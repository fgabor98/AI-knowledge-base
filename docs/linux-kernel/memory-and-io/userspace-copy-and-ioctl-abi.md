---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Userspace Copy And ioctl ABI

## What Problem Does This Solve?

Drivers that expose character devices must safely move data between kernel and userspace and preserve ABI compatibility.

Userspace pointers are untrusted. They may be invalid, unmapped, changed by another thread, or point to memory that faults while the kernel is copying. Kernel drivers must use user access helpers and must treat any interface structure as a long-term ABI contract.

## Core Concepts

- userspace pointers
- `copy_to_user`
- `copy_from_user`
- `get_user`
- `put_user`
- `ioctl`
- fixed-width types
- ABI versioning
- compatibility

## Mental Model

Userspace memory is untrusted and may fault. ABI structures are contracts, not internal implementation details.

```text
kernel internal state:
  private implementation

userspace ABI:
  stable contract
  fixed-size fields
  explicit flags
  no kernel pointers
  no uninitialized padding
```

Do not expose internal kernel structures directly. Design a UAPI structure for the interface.

## `__user` Pointers

Userspace pointers should be annotated:

```c
static ssize_t demo_read(struct file *filp, char __user *buf,
                         size_t count, loff_t *ppos)
```

Do not directly dereference:

```c
*buf = value; /* wrong */
```

Use usercopy helpers:

```c
if (copy_to_user(buf, &value, sizeof(value)))
    return -EFAULT;
```

The `__user` annotation helps static analysis catch address-space mistakes.

## Copying To Userspace

Example read path:

```c
static ssize_t demo_read(struct file *filp, char __user *buf,
                         size_t count, loff_t *ppos)
{
    struct demo_priv *priv = filp->private_data;
    struct demo_status status = { };

    if (count < sizeof(status))
        return -EINVAL;

    mutex_lock(&priv->lock);
    status.value = priv->value;
    status.flags = priv->flags;
    mutex_unlock(&priv->lock);

    if (copy_to_user(buf, &status, sizeof(status)))
        return -EFAULT;

    return sizeof(status);
}
```

Important details:

- the ABI struct is zero-initialized
- internal state is snapshotted under a lock
- `copy_to_user()` happens after unlocking if the lock must not be held across faulting operations
- partial copy failure returns `-EFAULT`

`copy_to_user()` returns the number of bytes not copied, not zero or a negative errno. In simple drivers, any nonzero value becomes `-EFAULT`.

## Copying From Userspace

Example write path:

```c
static ssize_t demo_write(struct file *filp, const char __user *buf,
                          size_t count, loff_t *ppos)
{
    struct demo_priv *priv = filp->private_data;
    struct demo_config cfg;
    int ret;

    if (count != sizeof(cfg))
        return -EINVAL;

    if (copy_from_user(&cfg, buf, sizeof(cfg)))
        return -EFAULT;

    ret = demo_validate_config(&cfg);
    if (ret)
        return ret;

    mutex_lock(&priv->lock);
    ret = demo_apply_config_locked(priv, &cfg);
    mutex_unlock(&priv->lock);

    return ret ? ret : count;
}
```

Validate all userspace-controlled fields before using them to allocate memory, index arrays, program hardware, or select command behavior.

## `get_user()` And `put_user()`

Use scalar helpers for simple values:

```c
u32 value;

if (get_user(value, user_argp))
    return -EFAULT;
```

Write scalar:

```c
if (put_user(value, user_argp))
    return -EFAULT;
```

Use `copy_from_user()` and `copy_to_user()` for structures and buffers.

## Usercopy Context Rules

Usercopy can fault and sleep. Do not call it from:

- hard IRQ handlers
- timer callbacks
- hrtimer callbacks
- spinlock-held sections
- RCU read-side sections that prohibit sleeping
- other atomic contexts

Wrong:

```c
spin_lock_irqsave(&priv->lock, flags);
ret = copy_to_user(buf, &priv->status, sizeof(priv->status));
spin_unlock_irqrestore(&priv->lock, flags);
```

Better:

```c
spin_lock_irqsave(&priv->lock, flags);
status = priv->status;
spin_unlock_irqrestore(&priv->lock, flags);

if (copy_to_user(buf, &status, sizeof(status)))
    return -EFAULT;
```

Snapshot under the atomic lock, then copy outside it.

## Avoid TOCTOU With Userspace Data

Userspace memory can change between copies. Copy once into kernel memory, validate the kernel copy, and use the kernel copy.

Wrong:

```c
if (get_user(len, &u->len))
    return -EFAULT;

if (len <= DEMO_MAX)
    copy_from_user(buf, u->data, len); /* userspace may have changed len */
```

Better:

```c
struct demo_req req;

if (copy_from_user(&req, argp, sizeof(req)))
    return -EFAULT;

if (req.len > DEMO_MAX)
    return -EINVAL;
```

For variable-length data, copy a fixed header first, validate the length, allocate a bounded buffer, then copy the payload.

## ABI Structures

Use fixed-width types in UAPI structures:

```c
#include <linux/types.h>

struct demo_status {
    __u32 version;
    __u32 flags;
    __u64 sample_count;
    __u32 value;
    __u32 reserved;
};
```

Avoid:

- `long`
- pointers as native pointer types
- kernel-only types such as `size_t`, `bool`, `enum` when size matters
- internal structs
- implicit padding with uninitialized bytes

If userspace needs to pass a pointer through ioctl, use `__u64` for the address and convert carefully in the kernel.

## Reserved Fields And Versioning

Reserve fields for future expansion:

```c
struct demo_config {
    __u32 version;
    __u32 flags;
    __u32 rate_hz;
    __u32 reserved[5];
};
```

Validate them:

```c
if (cfg.version != DEMO_ABI_VERSION)
    return -EINVAL;

if (cfg.flags & ~DEMO_ALLOWED_FLAGS)
    return -EINVAL;

if (cfg.reserved[0] || cfg.reserved[1] || cfg.reserved[2] ||
    cfg.reserved[3] || cfg.reserved[4])
    return -EINVAL;
```

Requiring reserved fields to be zero lets future kernels assign meaning without breaking old userspace.

## ioctl Command Numbers

Define ioctl commands in a UAPI header:

```c
#define DEMO_IOC_MAGIC      'd'

struct demo_status {
    __u32 version;
    __u32 flags;
    __u64 sample_count;
};

#define DEMO_IOC_GET_STATUS _IOR(DEMO_IOC_MAGIC, 0x00, struct demo_status)
#define DEMO_IOC_RESET      _IO(DEMO_IOC_MAGIC, 0x01)
#define DEMO_IOC_SET_CONFIG _IOW(DEMO_IOC_MAGIC, 0x02, struct demo_config)
```

Common macros:

| Macro | Meaning |
| --- | --- |
| `_IO()` | no data argument |
| `_IOR()` | kernel writes data to userspace |
| `_IOW()` | kernel reads data from userspace |
| `_IOWR()` | bidirectional structure |

The direction is from the userspace perspective in the encoded command naming convention: read means userspace reads data from the kernel, write means userspace writes data to the kernel.

## ioctl Handler Shape

```c
static long demo_ioctl(struct file *filp, unsigned int cmd,
                       unsigned long arg)
{
    struct demo_priv *priv = filp->private_data;
    void __user *argp = (void __user *)arg;

    switch (cmd) {
    case DEMO_IOC_GET_STATUS:
        return demo_ioctl_get_status(priv, argp);
    case DEMO_IOC_SET_CONFIG:
        return demo_ioctl_set_config(priv, argp);
    case DEMO_IOC_RESET:
        return demo_reset(priv);
    default:
        return -ENOTTY;
    }
}
```

File operations:

```c
static const struct file_operations demo_fops = {
    .owner = THIS_MODULE,
    .open = demo_open,
    .release = demo_release,
    .read = demo_read,
    .write = demo_write,
    .unlocked_ioctl = demo_ioctl,
};
```

Return `-ENOTTY` for unknown ioctl commands.

## ioctl Copy Helpers

GET:

```c
static long demo_ioctl_get_status(struct demo_priv *priv,
                                  void __user *argp)
{
    struct demo_status status = { };

    mutex_lock(&priv->lock);
    status.version = DEMO_ABI_VERSION;
    status.flags = priv->flags;
    status.sample_count = priv->sample_count;
    mutex_unlock(&priv->lock);

    if (copy_to_user(argp, &status, sizeof(status)))
        return -EFAULT;

    return 0;
}
```

SET:

```c
static long demo_ioctl_set_config(struct demo_priv *priv,
                                  void __user *argp)
{
    struct demo_config cfg;
    int ret;

    if (copy_from_user(&cfg, argp, sizeof(cfg)))
        return -EFAULT;

    ret = demo_validate_config(&cfg);
    if (ret)
        return ret;

    mutex_lock(&priv->lock);
    ret = demo_apply_config_locked(priv, &cfg);
    mutex_unlock(&priv->lock);

    return ret;
}
```

## 32-Bit Compatibility

On 64-bit kernels supporting 32-bit userspace, ioctl structures must be compatible across word sizes.

Avoid ABI fields whose size changes:

```text
long
unsigned long
size_t
pointers
time_t
```

Use fixed-width integer types:

```c
__u32
__u64
__s64
```

For pointer values passed through ioctl, define the field as `__u64` and convert with care:

```c
uintptr = u64_to_user_ptr(req.user_addr);
```

If all commands use compatible structures, the driver may be able to use a generic compat ioctl path. If not, implement `.compat_ioctl` explicitly and translate the 32-bit structure.

## Time Values In ABI

Do not expose kernel-internal time types directly.

For timestamps, prefer explicit units:

```c
struct demo_sample {
    __u64 timestamp_ns;
    __s32 value;
    __u32 flags;
};
```

Document which clock the timestamp uses:

```text
timestamp_ns is CLOCK_MONOTONIC nanoseconds
```

Avoid ABI ambiguity such as "milliseconds since now" unless it is carefully specified.

## Variable-Length Payloads

Pattern:

```c
struct demo_blob_req {
    __u32 len;
    __u32 flags;
    __u64 user_ptr;
};
```

Handler:

```c
if (copy_from_user(&req, argp, sizeof(req)))
    return -EFAULT;

if (req.len == 0 || req.len > DEMO_MAX_BLOB)
    return -EINVAL;

data = memdup_user(u64_to_user_ptr(req.user_ptr), req.len);
if (IS_ERR(data))
    return PTR_ERR(data);

ret = demo_process_blob(priv, data, req.len);
kfree(data);
return ret;
```

Use helpers such as `memdup_user()` when they match the job. They combine allocation and copying with normal error conventions.

## Security Rules

Do not expose:

- kernel pointers
- physical addresses unless the ABI explicitly requires and secures them
- uninitialized stack or heap padding
- stale data from previously used buffers
- hardware access that bypasses permission checks
- raw MMIO register access as a convenience ABI

Validate:

- lengths
- flags
- reserved fields
- alignment
- indexes
- enum/range values
- permissions
- device state

Every userspace-controlled value is hostile until checked.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| kernel oops | direct userspace pointer dereference | sparse and code review |
| sleep warning | usercopy under spinlock or in IRQ | stack trace |
| ABI breaks after refactor | internal struct exposed | UAPI header |
| data leak | uninitialized padding copied out | zero ABI structs |
| 32-bit app fails on 64-bit kernel | non-compatible ioctl struct | field sizes |
| random `-EFAULT` | invalid or racing user pointer | usercopy return handling |
| privilege issue | ioctl lacks permission/device-state checks | handler validation |

## Practice Exercises

### Exercise 1: Safe Read Snapshot

Implement a read path that snapshots driver state under a lock, releases the lock, and copies a zeroed ABI struct to userspace.

### Exercise 2: Config ioctl

Define `DEMO_IOC_SET_CONFIG` with:

```text
fixed-width fields
version
flags
reserved fields
validation
```

Return `-ENOTTY` for unknown commands.

### Exercise 3: Compat Audit

Inspect an ioctl struct and identify every field that changes size between 32-bit and 64-bit userspace.

## Debugging Checklist

- Check partial copy return values.
- Validate lengths and reserved fields.
- Avoid exposing kernel pointers or padding.
- Keep ABI structures independent from internal structs.
- Do not copy while holding spinlocks.
- Zero structures before copying to userspace.
- Use fixed-width UAPI types.
- Return `-EFAULT` for failed usercopy and `-ENOTTY` for unknown ioctls.
- Treat every userspace-provided pointer, length, and flag as untrusted.

## Related Topics

- [Character Device Basics](../fundamentals/character-device-basics.md)
- [Wait Queues And Completions](../execution-and-concurrency/wait-queues-and-completions.md)
- [Kernel Debugging Basics](../debugging/index.md)
- [Kernel Memory Allocation](kernel-memory-allocation.md)

## Official References

- [Ioctl based interfaces](https://docs.kernel.org/driver-api/ioctl.html)
- [Ioctl Numbers](https://docs.kernel.org/userspace-api/ioctl/ioctl-number.html)
- [Botching up ioctls](https://docs.kernel.org/process/botching-up-ioctls.html)
