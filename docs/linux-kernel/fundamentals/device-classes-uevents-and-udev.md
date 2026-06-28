---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Device Classes, Uevents, And udev

## What Problem Does This Solve?

Device classes and uevents let kernel devices appear as stable userspace nodes and metadata through `/dev` and sysfs. They connect kernel identity to userspace policy.

For a character driver, registering a `cdev` is not enough for normal userspace use. You also usually need a device object so userspace can create:

```text
/dev/demo0
```

and attach permissions, ownership, tags, and symlinks.

## Core Concepts

- device class
- class device
- device node
- `dev_t`
- major and minor numbers
- `class_create()`
- `device_create()`
- `device_destroy()`
- uevent
- environment variables
- modalias
- `udev`
- `udevadm info`
- `udevadm monitor`
- device permissions
- module autoloading
- sysfs path

## Mental Model

The kernel creates device identity and emits events. Userspace decides policy.

```text
kernel:
  creates struct device
  assigns dev_t
  emits uevent
  exposes sysfs attributes

udev/userspace:
  receives event
  creates /dev node
  applies permissions
  creates symlinks
  may trigger module loading
```

Do not hard-code product permissions in driver code when udev policy is the right layer.

## Class Creation For A Character Device

Typical module-level setup:

```c
static struct class *demo_class;
static dev_t demo_devt;

demo_class = class_create("demo");
if (IS_ERR(demo_class))
    return PTR_ERR(demo_class);
```

Create one device:

```c
device_create(demo_class, parent_dev, demo_devt, priv, "demo%d", 0);
```

Destroy:

```c
device_destroy(demo_class, demo_devt);
class_destroy(demo_class);
```

Arguments:

| Argument | Purpose |
| --- | --- |
| class | Class under `/sys/class/`. |
| parent | Parent device, if there is one. |
| devt | Major/minor for `/dev` node. |
| drvdata | Driver data attached to the created device. |
| format | Device name format. |

Use a real parent device when the cdev belongs to hardware discovered by a platform/I2C/SPI driver. That preserves device hierarchy.

## Sysfs And `/dev` Layout

After `device_create()`:

```text
/sys/class/demo/demo0/
/dev/demo0
```

Inspect:

```sh
ls -l /sys/class/demo/demo0
ls -l /dev/demo0
udevadm info --query=all --name=/dev/demo0
```

The `/dev` node contains the major/minor:

```sh
ls -l /dev/demo0
```

Example:

```text
crw-rw---- 1 root dialout 240, 0 /dev/demo0
```

`240, 0` is major 240, minor 0.

## Uevents

When the kernel adds, removes, or changes a device, it emits uevents.

Monitor:

```sh
udevadm monitor --kernel --property
```

Typical fields:

```text
ACTION=add
DEVPATH=/devices/platform/.../demo0
SUBSYSTEM=demo
DEVNAME=/dev/demo0
MAJOR=240
MINOR=0
```

These fields are inputs to udev rules.

## udev Rules

Example rule:

```text
SUBSYSTEM=="demo", KERNEL=="demo*", GROUP="dialout", MODE="0660"
```

Possible actions:

- set permissions
- set group
- create symlinks
- tag devices for systemd
- run helper scripts where policy allows

Reload rules:

```sh
sudo udevadm control --reload
sudo udevadm trigger --subsystem-match=demo
```

Product policy belongs in rules, image recipes, or system configuration, not ad hoc `chmod` in startup scripts.

## Device Naming

Names are ABI-adjacent. Choose them carefully.

Good:

```text
/dev/example-sensor0
/dev/demo0
```

Risky:

```text
/dev/test
/dev/newdriver
/dev/foo
```

For multiple devices, use stable minor allocation and names:

```c
device_create(demo_class, parent, MKDEV(MAJOR(base), id),
              priv, "demo%d", id);
```

If board topology matters, create udev symlinks based on parent path or attributes rather than encoding board location into kernel device names prematurely.

## Parent Devices

Bad for hardware-backed cdev:

```c
device_create(demo_class, NULL, devt, priv, "demo0");
```

Better inside `probe()`:

```c
device_create(demo_class, &pdev->dev, devt, priv, "demo%d", id);
```

The parent relationship helps:

- sysfs hierarchy
- power management relationships
- debugging
- udev rule matching
- lifetime reasoning

## Driver Data On Class Devices

`device_create()` can store driver data:

```c
dev = device_create(demo_class, parent, devt, priv, "demo%d", id);
if (IS_ERR(dev))
    return PTR_ERR(dev);
```

Later:

```c
priv = dev_get_drvdata(dev);
```

If you expose sysfs attributes on the class device, this links callbacks back to private state.

## Module Autoloading And Modalias

Uevents can also trigger module loading through modaliases.

For bus devices:

```sh
cat /sys/bus/platform/devices/<device>/modalias
modinfo demo.ko | grep alias
```

Userspace tools can translate modalias to module name using `modules.alias`.

If autoloading fails:

- check `MODULE_DEVICE_TABLE()`
- check `depmod`
- check module install path
- check `modalias`
- check udev/systemd module loading policy

## Debugging Device Node Creation

Check kernel side:

```sh
ls /sys/class/demo
cat /sys/class/demo/demo0/dev
```

Check userspace side:

```sh
ls -l /dev/demo0
udevadm info --query=all --path=/sys/class/demo/demo0
udevadm monitor --kernel --udev --property
```

Trigger events:

```sh
sudo udevadm trigger --path-match=/sys/class/demo/demo0
```

Check module logs:

```sh
dmesg | grep -i demo
```

## Cleanup Order

Destroy userspace-visible nodes before deleting the cdev and freeing state:

```c
device_destroy(demo_class, devt);
cdev_del(&priv->cdev);
```

Then release the device number when all minors are gone:

```c
unregister_chrdev_region(base_devt, count);
```

If files can remain open after `device_destroy()`, the node disappears but existing file descriptors may still call file operations. Your lifetime model must handle that.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `/dev/demo0` missing | `device_create()` failed or udev not running | return value, `/sys/class` |
| sysfs class exists but `/dev` missing | udev/devtmpfs policy issue | `udevadm monitor`, devtmpfs |
| wrong permissions | udev rule missing or wrong match | `udevadm info`, rules |
| stale `/dev` node | cleanup failure or manual node | major/minor, reload udev |
| autoload fails | missing modalias or `depmod` metadata | `modinfo`, `modalias` |
| symlink wrong | unstable rule match | parent path and attributes |

## Common Mistakes

- Ignoring errors from `device_create()`.
- Creating `/dev` nodes manually in product images when udev/devtmpfs should own them.
- Encoding permissions in random scripts instead of udev/system policy.
- Creating class devices without a real parent for hardware-backed devices.
- Destroying private state while `/dev` file descriptors still exist.
- Depending on dynamic major numbers in userspace scripts.
- Forgetting `depmod` when testing module autoloading.

## Practice Exercises

### Exercise 1: Inspect A Device Node

For any `/dev` node:

```sh
ls -l /dev/null
udevadm info --query=all --name=/dev/null
```

Identify:

- subsystem
- major/minor
- sysfs path

### Exercise 2: Add A udev Rule

Create a rule for a test device:

```text
SUBSYSTEM=="demo", KERNEL=="demo*", MODE="0660", GROUP="plugdev"
```

Reload and trigger. Confirm permissions.

### Exercise 3: Watch Events

In one terminal:

```sh
udevadm monitor --kernel --udev --property
```

In another, load/unload your module and observe add/remove events.

## Debugging Checklist

- Did `alloc_chrdev_region()` allocate the expected major/minor range?
- Did `cdev_add()` succeed?
- Did `class_create()` succeed?
- Did `device_create()` succeed?
- Does `/sys/class/<class>/<device>/dev` show the expected major/minor?
- Did udev receive an add event?
- Do udev rules match the actual subsystem and kernel name?
- Are permissions and ownership product policy, not driver hacks?
- Does cleanup emit remove events?

## Related Topics

- [Character Device Basics](character-device-basics.md)
- [Sysfs Attributes](sysfs-attributes.md)
- [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)

## Official References

- [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html)
- [Driver Model Classes](https://docs.kernel.org/driver-api/driver-model/class.html)
- [udev manual pages](https://www.freedesktop.org/software/systemd/man/latest/udev.html)
