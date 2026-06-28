---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Character Device Basics

## What Problem Does This Solve?

Character devices expose byte-stream or command-style kernel interfaces to userspace through device nodes such as:

```text
/dev/demo0
```

They are useful when a driver needs a custom userspace interface that does not fit an existing kernel subsystem.

They are also dangerous to design casually because a character device is a userspace ABI. Once applications depend on it, changing behavior can break systems.

## Core Concepts

- character device
- major number
- minor number
- `dev_t`
- `alloc_chrdev_region()`
- `struct cdev`
- `cdev_init()`
- `cdev_add()`
- `struct file_operations`
- `open()`
- `release()`
- `read()`
- `write()`
- `unlocked_ioctl()`
- `poll()`
- `copy_to_user()`
- `copy_from_user()`
- `class_create()`
- `device_create()`
- `/dev` node
- udev
- ABI stability

## Mental Model

A character device connects a userspace file descriptor to driver callbacks:

```text
userspace open("/dev/demo0")
-> VFS finds major/minor
-> cdev file_operations
-> driver open/read/write/ioctl/release
```

The kernel does not know what your bytes or commands mean. Your driver defines the ABI, so you must define it deliberately.

## When To Use A Character Device

Use a character device when:

- userspace needs stream-like access
- userspace sends commands or receives data not covered by an existing subsystem
- a device is product-specific and no standard Linux ABI exists
- read/write/poll/ioctl semantics are natural

Prefer an existing subsystem when possible:

| Hardware/Need | Prefer |
| --- | --- |
| ADC, DAC, sensor channels | IIO |
| buttons, keys, touch | input subsystem |
| LEDs | LED subsystem |
| watchdog | watchdog subsystem |
| network devices | netdev |
| audio | ALSA/ASoC |
| video/camera | V4L2 |
| display | DRM |
| debug-only state | debugfs |
| simple device state | sysfs |

Do not create a private character device just because it is easy.

## Minimal Character Device Structure

Global/simple teaching example:

```c
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

#define DEMO_BUFSIZE 128

static dev_t demo_devt;
static struct cdev demo_cdev;
static struct class *demo_class;
static char demo_buf[DEMO_BUFSIZE];
static size_t demo_len;
static DEFINE_MUTEX(demo_lock);
```

File operations:

```c
static const struct file_operations demo_fops = {
    .owner = THIS_MODULE,
    .open = demo_open,
    .release = demo_release,
    .read = demo_read,
    .write = demo_write,
    .llseek = no_llseek,
};
```

Registration:

```c
static int __init demo_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&demo_devt, 0, 1, "demo");
    if (ret)
        return ret;

    cdev_init(&demo_cdev, &demo_fops);
    ret = cdev_add(&demo_cdev, demo_devt, 1);
    if (ret)
        goto err_unregister;

    demo_class = class_create("demo");
    if (IS_ERR(demo_class)) {
        ret = PTR_ERR(demo_class);
        goto err_cdev;
    }

    if (IS_ERR(device_create(demo_class, NULL, demo_devt, NULL, "demo0"))) {
        ret = -ENODEV;
        goto err_class;
    }

    return 0;

err_class:
    class_destroy(demo_class);
err_cdev:
    cdev_del(&demo_cdev);
err_unregister:
    unregister_chrdev_region(demo_devt, 1);
    return ret;
}

static void __exit demo_exit(void)
{
    device_destroy(demo_class, demo_devt);
    class_destroy(demo_class);
    cdev_del(&demo_cdev);
    unregister_chrdev_region(demo_devt, 1);
}

module_init(demo_init);
module_exit(demo_exit);
```

Some older kernel trees use a different `class_create()` signature. Always follow the API in the kernel tree you build against.

## Major And Minor Numbers

A device number is stored in `dev_t`.

Get parts:

```c
MAJOR(demo_devt)
MINOR(demo_devt)
```

Allocate dynamically:

```c
alloc_chrdev_region(&demo_devt, 0, 1, "demo");
```

Avoid hard-coding major numbers in new drivers. Let the kernel allocate and let udev create the `/dev` node.

For multiple devices:

```c
alloc_chrdev_region(&base_devt, 0, DEMO_MAX_MINORS, "demo");
```

Each instance can use:

```c
MKDEV(MAJOR(base_devt), minor)
```

## `open()` And `release()`

```c
static int demo_open(struct inode *inode, struct file *file)
{
    pr_debug("demo: open\n");
    return 0;
}

static int demo_release(struct inode *inode, struct file *file)
{
    pr_debug("demo: release\n");
    return 0;
}
```

For real drivers, `open()` commonly:

- finds the containing private structure
- stores it in `file->private_data`
- powers up hardware
- prevents removal while in use
- initializes per-open state

Example using `container_of()`:

```c
struct demo_dev {
    struct cdev cdev;
    struct mutex lock;
};

static int demo_open(struct inode *inode, struct file *file)
{
    struct demo_dev *demo;

    demo = container_of(inode->i_cdev, struct demo_dev, cdev);
    file->private_data = demo;
    return 0;
}
```

## `read()`

Prototype:

```c
static ssize_t demo_read(struct file *file, char __user *buf,
                         size_t count, loff_t *ppos)
```

Simple implementation:

```c
static ssize_t demo_read(struct file *file, char __user *buf,
                         size_t count, loff_t *ppos)
{
    ssize_t ret;

    mutex_lock(&demo_lock);

    if (*ppos >= demo_len) {
        ret = 0;
        goto out;
    }

    if (count > demo_len - *ppos)
        count = demo_len - *ppos;

    if (copy_to_user(buf, demo_buf + *ppos, count)) {
        ret = -EFAULT;
        goto out;
    }

    *ppos += count;
    ret = count;

out:
    mutex_unlock(&demo_lock);
    return ret;
}
```

Important rules:

- return number of bytes copied on success
- return `0` for EOF where appropriate
- return negative errno on failure
- support partial reads
- never dereference userspace pointers directly
- update `*ppos` if file position matters

## `write()`

```c
static ssize_t demo_write(struct file *file, const char __user *buf,
                          size_t count, loff_t *ppos)
{
    ssize_t ret;

    if (count > DEMO_BUFSIZE)
        count = DEMO_BUFSIZE;

    mutex_lock(&demo_lock);

    if (copy_from_user(demo_buf, buf, count)) {
        ret = -EFAULT;
        goto out;
    }

    demo_len = count;
    ret = count;

out:
    mutex_unlock(&demo_lock);
    return ret;
}
```

Important rules:

- validate size
- copy from userspace with `copy_from_user()`
- define whether partial writes are accepted
- protect shared state
- avoid unbounded allocations from userspace-controlled sizes

## Userspace Copy Helpers

Never do this:

```c
memcpy(kernel_buf, user_ptr, len); /* wrong */
```

Use:

```c
copy_from_user(kernel_buf, user_ptr, len);
copy_to_user(user_ptr, kernel_buf, len);
```

These helpers validate access and handle page faults safely for userspace memory. They can sleep, so do not use them in atomic context or interrupt handlers.

## `ioctl()` Basics

Use `ioctl` for structured commands that do not fit read/write.

Define commands in a UAPI header:

```c
#define DEMO_IOC_MAGIC      'd'
#define DEMO_IOC_RESET      _IO(DEMO_IOC_MAGIC, 0)
#define DEMO_IOC_GET_STATUS _IOR(DEMO_IOC_MAGIC, 1, struct demo_status)
#define DEMO_IOC_SET_MODE   _IOW(DEMO_IOC_MAGIC, 2, struct demo_mode)
```

Handler:

```c
static long demo_ioctl(struct file *file, unsigned int cmd,
                       unsigned long arg)
{
    switch (cmd) {
    case DEMO_IOC_RESET:
        return demo_reset(file->private_data);
    default:
        return -ENOTTY;
    }
}
```

ABI discipline:

- use fixed-width types in structures
- avoid raw pointers inside ioctl structs
- define padding for future extension
- validate all fields
- preserve compatibility once shipped

Detailed ioctl ABI design belongs in [Userspace Copy And ioctl ABI](../memory-and-io/userspace-copy-and-ioctl-abi.md).

## `poll()`

Use `poll()` when userspace should wait for readiness:

```c
static __poll_t demo_poll(struct file *file, poll_table *wait)
{
    struct demo_dev *demo = file->private_data;
    __poll_t mask = 0;

    poll_wait(file, &demo->readq, wait);

    if (demo_data_available(demo))
        mask |= EPOLLIN | EPOLLRDNORM;

    return mask;
}
```

Wake users:

```c
wake_up_interruptible(&demo->readq);
```

Use poll for character devices that produce asynchronous data. Do not fake readiness if reads will block unexpectedly.

## Blocking And Nonblocking I/O

Check:

```c
if (file->f_flags & O_NONBLOCK)
    return -EAGAIN;
```

Blocking read pattern:

```c
ret = wait_event_interruptible(demo->readq, demo_data_available(demo));
if (ret)
    return ret;
```

Never sleep while holding a spinlock. Be careful when sleeping with a mutex if wakeup paths need the same lock.

## Lifetime With Open Files

A device can be removed while userspace still holds a file descriptor.

Plan for:

- `remove()` marks device as gone
- new opens fail
- existing operations return `-ENODEV` or finish safely
- reference counts keep memory alive until last close
- asynchronous work stops before memory is freed

This is where simple teaching examples stop being enough. Real hot-unplug-capable drivers need explicit lifetime design.

## Character Device In A Platform Driver

For one cdev per platform device:

```text
probe()
  allocate priv
  allocate minor or ID
  cdev_add()
  device_create()

remove()
  device_destroy()
  cdev_del()
  free minor/ID
```

Use `file->private_data` to connect operations to the right device instance. Avoid one global buffer for all instances unless that is the intended ABI.

## Testing From Userspace

Create and inspect:

```sh
ls -l /dev/demo0
udevadm info --query=all --name=/dev/demo0
```

Write:

```sh
printf 'hello' > /dev/demo0
```

Read:

```sh
dd if=/dev/demo0 bs=1 count=5 2>/dev/null
```

Trace errors:

```sh
strace -e openat,read,write,ioctl cat /dev/demo0
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `/dev/demo0` missing | no class device or udev issue | `device_create`, udev monitor |
| open returns `ENODEV` | driver marked device gone or no backing device | logs, remove path |
| read returns `EFAULT` | bad userspace pointer or copy failure | copy helpers, test program |
| write truncates unexpectedly | driver-defined max size | ABI docs |
| module cannot unload | open file descriptor or reference held | `lsmod`, `fuser /dev/demo0` |
| crash after remove | file ops use freed private data | lifetime/refcounting |
| userspace app breaks after update | ABI changed | UAPI compatibility |

## Common Mistakes

- Dereferencing userspace pointers directly.
- Returning positive values for errors.
- Ignoring partial reads and writes.
- Forgetting `.owner = THIS_MODULE`.
- Exposing a character device when a standard subsystem exists.
- Designing ioctl structs with native C types whose size changes across architectures.
- Freeing private data while file descriptors still exist.
- Not documenting the ABI.
- Using world-writable device permissions to avoid proper policy.

## Practice Exercises

### Exercise 1: Minimal Echo Device

Implement a device where `write()` stores a short buffer and `read()` returns it.

Test:

```sh
printf abc > /dev/demo0
cat /dev/demo0
```

### Exercise 2: Add Nonblocking Behavior

Make `read()` return `-EAGAIN` when no data is available and `O_NONBLOCK` is set.

Test with a small C program or `strace`.

### Exercise 3: Add A Simple ioctl

Add `DEMO_IOC_RESET` that clears the buffer. Keep the command definition in a UAPI-style header.

## Debugging Checklist

- Was a device number allocated?
- Was `cdev_add()` successful?
- Did `device_create()` create a class device?
- Did udev create the `/dev` node?
- Are file operations connected to the right private state?
- Are userspace pointers accessed only with copy helpers?
- Are read/write return values correct?
- Are locks appropriate for shared state?
- Can the module unload while no file is open?
- Is the ABI documented and version-safe?

## Related Topics

- [Device Classes, Uevents, And udev](device-classes-uevents-and-udev.md)
- [Userspace Copy And ioctl ABI](../memory-and-io/userspace-copy-and-ioctl-abi.md)
- [Sysfs Attributes](sysfs-attributes.md)
- [Kernel Module Lifecycle](kernel-module-lifecycle.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)

## Official References

- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [The Linux driver model](https://docs.kernel.org/driver-api/driver-model/index.html)
- [Adding a New System Call and ioctl Alternatives](https://docs.kernel.org/process/adding-syscalls.html)
