---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Sysfs Attributes

## What Problem Does This Solve?

Sysfs exposes device, driver, bus, class, and subsystem state to userspace through small text files.

Example:

```text
/sys/bus/platform/devices/10000000.demo/status
/sys/class/demo/demo0/value
```

Sysfs is useful for simple state and control knobs. It is not a byte stream, logging system, high-rate data path, or replacement for a real subsystem ABI.

## Core Concepts

- sysfs
- kobject
- device attribute
- `DEVICE_ATTR_RO()`
- `DEVICE_ATTR_WO()`
- `DEVICE_ATTR_RW()`
- show method
- store method
- `sysfs_emit()`
- `kstrto*()` parsers
- one-value-per-file convention
- file permissions
- lifetime
- ABI documentation
- `sysfs_notify()`

## Mental Model

Sysfs files are callbacks attached to kernel objects.

```text
userspace read file
-> show callback
-> driver formats current state as text

userspace write file
-> store callback
-> driver validates input and changes state
```

The file contents are generated on demand. Do not treat sysfs like a normal storage file.

## When To Use Sysfs

Use sysfs for:

- simple device state
- one value or small structured value per file
- human-readable control
- low-rate configuration
- discoverable metadata
- standardized subsystem attributes

Avoid sysfs for:

- streaming samples
- binary protocols
- large logs
- firmware blobs
- high-rate notifications
- private debug dumps
- complex command protocols

Alternatives:

| Need | Better Interface |
| --- | --- |
| high-rate sensor samples | IIO buffers |
| stream data | character device or subsystem |
| debug-only internals | debugfs |
| trace events | tracepoints/ftrace |
| network packets | netdev |
| input events | input subsystem |

## Read-Only Device Attribute

Example private state:

```c
struct demo_priv {
    struct device *dev;
    u32 status;
};
```

Show callback:

```c
static ssize_t status_show(struct device *dev,
                           struct device_attribute *attr,
                           char *buf)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    return sysfs_emit(buf, "%u\n", priv->status);
}
static DEVICE_ATTR_RO(status);
```

Create in probe:

```c
ret = device_create_file(&pdev->dev, &dev_attr_status);
if (ret)
    return ret;
```

Remove:

```c
device_remove_file(&pdev->dev, &dev_attr_status);
```

For multiple attributes, prefer attribute groups.

## Writable Device Attribute

```c
static ssize_t threshold_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    return sysfs_emit(buf, "%u\n", priv->threshold);
}

static ssize_t threshold_store(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    unsigned int val;
    int ret;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
        return ret;

    if (val > 1000)
        return -EINVAL;

    mutex_lock(&priv->lock);
    priv->threshold = val;
    mutex_unlock(&priv->lock);

    return count;
}
static DEVICE_ATTR_RW(threshold);
```

Rules:

- parse input with kernel helpers
- validate ranges
- protect shared state
- return `count` on success
- return negative errno on failure

## Permissions

Attribute macros define permissions.

```c
static DEVICE_ATTR_RO(status);     /* read-only */
static DEVICE_ATTR_WO(reset);      /* write-only */
static DEVICE_ATTR_RW(threshold);  /* read-write */
```

Custom:

```c
static DEVICE_ATTR(mode, 0644, mode_show, mode_store);
```

Avoid world-writable attributes unless there is a clear reason.

Permissions are not the whole security model. Product access policy may still belong in udev, systemd, capabilities, or subsystem-specific policy.

## Attribute Groups

Groups make creation/removal cleaner:

```c
static struct attribute *demo_attrs[] = {
    &dev_attr_status.attr,
    &dev_attr_threshold.attr,
    NULL,
};

static const struct attribute_group demo_attr_group = {
    .attrs = demo_attrs,
};
```

Create:

```c
ret = devm_device_add_group(&pdev->dev, &demo_attr_group);
if (ret)
    return ret;
```

This ties removal to device cleanup.

For static attributes on a driver/device type, many subsystems provide default groups. Prefer established subsystem patterns when available.

## One Value Per File

Preferred:

```text
status
threshold
enable
```

with contents:

```text
1
250
enabled
```

Avoid:

```text
config
```

with contents:

```text
enable=1 threshold=250 mode=fast
```

Sysfs convention favors simple attributes. If you need complex commands or structured transactions, sysfs is probably the wrong ABI.

## Formatting With `sysfs_emit()`

Use:

```c
return sysfs_emit(buf, "%u\n", value);
```

instead of open-coded `sprintf()`.

For multiple values where accepted by subsystem convention:

```c
return sysfs_emit(buf, "%u %u\n", x, y);
```

Keep output stable and documented.

## Input Parsing

Use helpers:

```c
kstrtoint(buf, 0, &val);
kstrtouint(buf, 0, &val);
kstrtobool(buf, &enabled);
sysfs_streq(buf, "reset");
```

Example:

```c
bool enabled;
int ret;

ret = kstrtobool(buf, &enabled);
if (ret)
    return ret;
```

Do not use unsafe parsing or assume input is null-terminated in every context.

## Lifetime Rules

Sysfs callbacks run while userspace reads or writes. The backing state must remain valid.

For device attributes:

- attach attributes to the device that owns the state
- remove attributes before freeing backing state
- use locking for mutable state
- handle device removal racing with userspace access

If using `devm_device_add_group()`, managed cleanup removes the group when the device is released. You still need a coherent remove path for active hardware and state changes.

## Avoid Long Work In Callbacks

Sysfs callbacks should be short.

Avoid:

- long sleeps
- firmware updates
- large hardware transfers
- waiting forever for hardware
- high-rate polling
- complex multi-step commands

If a write starts work, consider:

- validate and queue work
- return promptly
- expose separate state/progress
- use a more appropriate interface for complex operations

## ABI Documentation

Sysfs attributes that form a userspace ABI should be documented under:

```text
Documentation/ABI/
```

Typical entry:

```text
What:           /sys/bus/platform/devices/.../threshold
Date:           2026-06
KernelVersion:  6.x
Contact:        maintainer@example.com
Description:
                Read or write the threshold in millivolts.
Users:          product-daemon
```

In product repositories, maintain equivalent documentation even if the driver is not upstream.

## Example: Probe With Attribute Group

```c
struct demo_priv {
    struct device *dev;
    struct mutex lock;
    unsigned int threshold;
};

static ssize_t threshold_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    unsigned int threshold;

    mutex_lock(&priv->lock);
    threshold = priv->threshold;
    mutex_unlock(&priv->lock);

    return sysfs_emit(buf, "%u\n", threshold);
}

static ssize_t threshold_store(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    unsigned int val;
    int ret;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
        return ret;

    if (val > 1000)
        return -EINVAL;

    mutex_lock(&priv->lock);
    priv->threshold = val;
    mutex_unlock(&priv->lock);

    return count;
}
static DEVICE_ATTR_RW(threshold);

static struct attribute *demo_attrs[] = {
    &dev_attr_threshold.attr,
    NULL,
};

static const struct attribute_group demo_group = {
    .attrs = demo_attrs,
};

static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;
    mutex_init(&priv->lock);
    priv->threshold = 100;
    dev_set_drvdata(&pdev->dev, priv);

    return devm_device_add_group(&pdev->dev, &demo_group);
}
```

## Testing

Read:

```sh
cat /sys/bus/platform/devices/<device>/threshold
```

Write:

```sh
echo 250 | sudo tee /sys/bus/platform/devices/<device>/threshold
```

Invalid write:

```sh
echo 999999 | sudo tee /sys/bus/platform/devices/<device>/threshold
echo $?
```

Trace:

```sh
strace -e read,write cat /sys/bus/platform/devices/<device>/threshold
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Attribute missing | group not created or wrong object | probe return, sysfs path |
| Write returns `EINVAL` | parser/range rejected input | `dmesg`, input format |
| Attribute crashes on remove | backing state freed too early | lifetime and remove order |
| Output truncated/weird | formatting bug | `sysfs_emit`, buffer usage |
| Userspace misses changes | sysfs is not event stream | `sysfs_notify`, poll, better ABI |
| Permission denied | file mode or system policy | `ls -l`, udev/system policy |

## Common Mistakes

- Using sysfs for high-rate data.
- Returning something other than `count` on successful store.
- Parsing with unsafe string code.
- Forgetting locking around shared state.
- Exposing multiple unrelated values in one file.
- Adding debug-only internals to permanent sysfs ABI.
- Removing backing state while callbacks can run.
- Inventing attributes already provided by a subsystem.

## Practice Exercises

### Exercise 1: Add A Read-Only Attribute

Expose a `status` value:

```sh
cat /sys/.../status
```

Use `sysfs_emit()`.

### Exercise 2: Add Validation

Add a writable `threshold` and reject values outside a defined range.

### Exercise 3: Convert Files To A Group

Replace multiple `device_create_file()` calls with one attribute group.

## Debugging Checklist

- Is the attribute attached to the right device/kobject?
- Are permissions correct?
- Does show use `sysfs_emit()`?
- Does store parse and validate input?
- Does store return `count` on success?
- Is shared state locked?
- Is the attribute documented?
- Is sysfs the right ABI for this data?
- Does removal handle active callbacks safely?

## Related Topics

- [Character Device Basics](character-device-basics.md)
- [Kobjects And Sysfs Groups](kobjects-and-sysfs-groups.md)
- [Pollable Sysfs Attributes](pollable-sysfs-attributes.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)

## Official References

- [The Linux driver model](https://docs.kernel.org/driver-api/driver-model/index.html)
- [Everything you never wanted to know about kobjects, ksets, and ktypes](https://docs.kernel.org/core-api/kobject.html)
- [ABI testing and sysfs documentation](https://docs.kernel.org/admin-guide/abi.html)
