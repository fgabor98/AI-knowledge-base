---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kobjects And Sysfs Groups

## What Problem Does This Solve?

Kobjects provide the object model behind sysfs. Attribute groups organize related sysfs files and simplify creation and teardown.

Most driver authors should not start by creating raw kobjects. If data belongs to a device, expose it through device attributes. If it belongs to an existing subsystem, follow that subsystem's ABI. Learn kobjects so you understand sysfs lifetime and can read kernel code, not because every driver needs custom kobjects.

## Core Concepts

- `struct kobject`
- kset overview
- sysfs directory
- sysfs attribute
- `struct attribute`
- `struct attribute_group`
- show callback
- store callback
- `kobject_create_and_add()`
- `kobject_put()`
- reference ownership
- release callback
- default groups
- device attributes versus raw kobjects

## Mental Model

Sysfs files are attached to kernel objects. The object must remain alive while sysfs can call its callbacks.

```text
kobject exists
-> sysfs directory exists
-> attributes expose callbacks
-> userspace reads/writes
-> attributes removed
-> kobject reference dropped
-> release frees object
```

If the file outlives the state used by callbacks, the driver has a use-after-free bug.

## Prefer Device Attributes First

If the attribute describes a device:

```text
/sys/bus/platform/devices/<device>/threshold
```

prefer:

```c
DEVICE_ATTR_RW(threshold);
devm_device_add_group(dev, &group);
```

Only consider custom kobjects when you need a separate object model not already represented by a device, bus, class, or subsystem.

## Attribute Groups

An attribute group collects related files:

```c
static struct attribute *demo_attrs[] = {
    &dev_attr_status.attr,
    &dev_attr_threshold.attr,
    NULL,
};

static const struct attribute_group demo_group = {
    .attrs = demo_attrs,
};
```

Create on a device:

```c
ret = sysfs_create_group(&dev->kobj, &demo_group);
if (ret)
    return ret;
```

Remove:

```c
sysfs_remove_group(&dev->kobj, &demo_group);
```

Managed device helper:

```c
ret = devm_device_add_group(dev, &demo_group);
```

Prefer the managed helper for normal device-owned attributes.

## Named Subdirectories

Groups can create a subdirectory:

```c
static const struct attribute_group demo_stats_group = {
    .name = "stats",
    .attrs = demo_stats_attrs,
};
```

Result:

```text
/sys/.../stats/errors
/sys/.../stats/resets
```

Use subdirectories sparingly and only when they make the ABI clearer.

## Raw Kobject Example

Simple global example:

```c
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/sysfs.h>

static struct kobject *demo_kobj;
static int demo_value;

static ssize_t value_show(struct kobject *kobj,
                          struct kobj_attribute *attr,
                          char *buf)
{
    return sysfs_emit(buf, "%d\n", demo_value);
}

static ssize_t value_store(struct kobject *kobj,
                           struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
    int ret;
    int val;

    ret = kstrtoint(buf, 0, &val);
    if (ret)
        return ret;

    demo_value = val;
    return count;
}

static struct kobj_attribute value_attr =
    __ATTR(value, 0644, value_show, value_store);

static struct attribute *demo_attrs[] = {
    &value_attr.attr,
    NULL,
};

static const struct attribute_group demo_group = {
    .attrs = demo_attrs,
};

static int __init demo_init(void)
{
    int ret;

    demo_kobj = kobject_create_and_add("demo", kernel_kobj);
    if (!demo_kobj)
        return -ENOMEM;

    ret = sysfs_create_group(demo_kobj, &demo_group);
    if (ret) {
        kobject_put(demo_kobj);
        return ret;
    }

    return 0;
}

static void __exit demo_exit(void)
{
    sysfs_remove_group(demo_kobj, &demo_group);
    kobject_put(demo_kobj);
}
```

This creates:

```text
/sys/kernel/demo/value
```

Again: this is for understanding and rare global objects. Device-specific state should usually be under the device.

## Custom Embedded Kobject

For a custom object, embed `struct kobject` and provide a release callback through `kobj_type`.

High-level shape:

```c
struct demo_obj {
    struct kobject kobj;
    int value;
};

static void demo_release(struct kobject *kobj)
{
    struct demo_obj *obj = container_of(kobj, struct demo_obj, kobj);

    kfree(obj);
}
```

The release callback is where memory is freed after the final reference goes away. If a kobject has no proper release path, lifetime is wrong.

This is an advanced pattern. Reach for device attributes first.

## Show And Store Callback Differences

Device attributes:

```c
static ssize_t foo_show(struct device *dev,
                        struct device_attribute *attr,
                        char *buf);
```

Kobject attributes:

```c
static ssize_t foo_show(struct kobject *kobj,
                        struct kobj_attribute *attr,
                        char *buf);
```

The callback type depends on the object type that owns the sysfs file.

## Lifetime Rules

Rules:

- create attributes only after backing state is initialized
- remove attributes before state becomes invalid
- use reference counting when sysfs can outlive local state
- provide a release callback for custom kobjects
- do not free embedded kobject memory directly while references exist
- keep callbacks short and locking simple

Bad:

```c
kfree(obj); /* while sysfs file can still call into obj */
```

Good:

```c
kobject_put(&obj->kobj); /* release frees when final ref drops */
```

## Default Attribute Groups

Many kernel object types support default groups that are created automatically with the object.

For drivers, you may see patterns like:

```c
static const struct attribute_group *demo_groups[] = {
    &demo_group,
    NULL,
};
```

attached to a class, device type, bus, or driver structure.

Use established subsystem patterns rather than manually creating/removing files when a default-group mechanism exists.

## Debugging Sysfs Object Problems

Inspect:

```sh
find /sys/kernel/demo -maxdepth 2 -type f -print
cat /sys/kernel/demo/value
```

Watch lifetime:

```sh
dmesg | grep -i demo
```

If removing a module crashes after sysfs access, suspect:

- callbacks use freed memory
- attributes not removed
- missing kobject release
- active work references object without refcount
- module unload while sysfs file is open

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| sysfs file missing | group creation failed | return values |
| module unload warning | kobject reference leak | `kobject_put`, release |
| crash on read after remove | backing object freed too early | lifetime and remove order |
| store corrupts state | missing validation/locking | parser, locks |
| duplicate file error | group created twice | probe/remove ordering |
| permanent ABI in wrong place | custom kobject used instead of subsystem/device | design review |

## Common Mistakes

- Creating raw kobjects for device data.
- Forgetting the empty `NULL` terminator in attribute arrays.
- Forgetting to remove groups.
- Freeing an object directly instead of dropping the kobject reference.
- Missing release callback for embedded kobjects.
- Exposing debug state in sysfs instead of debugfs.
- Creating many individual files manually instead of using groups.

## Practice Exercises

### Exercise 1: Convert To A Group

Take two existing device attributes and move them into one `attribute_group`.

### Exercise 2: Create A Global Teaching Kobject

Create `/sys/kernel/demo/value` using `kobject_create_and_add()`. Load/unload repeatedly and check for warnings.

### Exercise 3: Explain Lifetime

For a sysfs callback in an existing driver, identify:

- the object that owns the file
- where the file is created
- where the file is removed
- what keeps backing state alive

## Debugging Checklist

- Should this be a device attribute instead of a raw kobject?
- Does every attribute array end with `NULL`?
- Are groups created after state initialization?
- Are groups removed before state invalidation?
- Does every custom kobject have a release callback?
- Are references put exactly once?
- Are sysfs callbacks locked and validated?
- Is the ABI documented?

## Related Topics

- [Sysfs Attributes](sysfs-attributes.md)
- [Pollable Sysfs Attributes](pollable-sysfs-attributes.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)

## Official References

- [Kobjects](https://docs.kernel.org/core-api/kobject.html)
- [Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html)
- [ABI documentation](https://docs.kernel.org/admin-guide/abi.html)
