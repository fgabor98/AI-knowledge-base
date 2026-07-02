---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Module Lifecycle

## What Problem Does This Solve?

Kernel modules let driver code be compiled, loaded, inspected, and removed independently from the base kernel image. That makes them ideal for learning, bring-up, optional hardware, and development loops where rebooting for every change would be too slow.

The lifecycle lesson is simple but strict:

```text
module init registers things
module exit unregisters those same things
```

If the module creates devices, registers callbacks, starts timers, schedules work, enables interrupts, or exposes userspace files, the exit path must leave no active user, callback, or hardware path behind.

## Core Concepts

- loadable kernel module
- external module
- module init function
- module exit function
- `module_init()`
- `module_exit()`
- module metadata
- `MODULE_LICENSE()`
- `MODULE_AUTHOR()`
- `MODULE_DESCRIPTION()`
- `insmod`
- `modprobe`
- `rmmod`
- `lsmod`
- `modinfo`
- init failure cleanup
- module reference ownership
- symbol dependencies
- taint flags
- module autoloading

## Mental Model

Think of module init as registration, not hardware ownership:

```text
module loaded
-> init function runs once
-> driver/subsystem/cdev/sysfs registrations happen
-> kernel calls your registered callbacks later
-> exit function unregisters and stops all callbacks
-> module memory can be freed
```

For a driver module, `module_init()` often registers a driver, but the real per-device setup happens later in `probe()`:

```text
module_init()
  registers platform_driver

platform bus match
  calls demo_probe() for each matching device

module_exit()
  unregisters platform_driver
  which removes bound devices
```

Do not allocate per-device hardware resources globally in module init unless there is truly only one global software object.

## Minimal External Module

Source file:

```c
// hello.c
#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
    pr_info("hello: module loaded\n");
    return 0;
}

static void __exit hello_exit(void)
{
    pr_info("hello: module unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Minimal module lifecycle example");
```

Makefile:

```make
obj-m += hello.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Build and inspect:

```sh
make
modinfo hello.ko
modinfo hello.ko | grep vermagic
```

Load and remove:

```sh
sudo insmod hello.ko
dmesg | tail -n 20
lsmod | grep hello
sudo rmmod hello
dmesg | tail -n 20
```

## `insmod` Versus `modprobe`

`insmod` loads one exact `.ko` file:

```sh
sudo insmod ./hello.ko
```

It does not resolve module dependencies.

`modprobe` loads by module name and uses metadata under `/lib/modules/$(uname -r)/`:

```sh
sudo modprobe hello
sudo modprobe -r hello
```

`modprobe` can load dependencies and follow aliases. It requires the module to be installed in the module tree and `depmod` metadata to exist.

Use `insmod` for quick local experiments. Use `modprobe` to test product-style module installation and dependency handling.

## Module Metadata

Common metadata:

```c
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Demo module");
MODULE_VERSION("1.0");
```

`MODULE_LICENSE("GPL")` matters because some exported kernel symbols are available only to GPL-compatible modules. A missing or non-GPL-compatible license can also taint the kernel.

Inspect metadata:

```sh
modinfo hello.ko
```

Typical fields:

```text
filename:
license:
description:
author:
depends:
vermagic:
```

`vermagic` helps diagnose whether the module was built for the running kernel.

## Return Values From Init

Module init returns `0` on success and a negative errno on failure:

```c
static int __init demo_init(void)
{
    int ret;

    ret = register_something();
    if (ret)
        return ret;

    ret = register_second_thing();
    if (ret) {
        unregister_something();
        return ret;
    }

    return 0;
}
```

Use standard errno values:

| Error | Meaning |
| --- | --- |
| `-ENOMEM` | Allocation failed. |
| `-EINVAL` | Invalid parameter or configuration. |
| `-ENODEV` | Expected device or hardware not present. |
| `-EBUSY` | Resource already in use. |
| `-EPROBE_DEFER` | Provider not ready; mainly from probe paths. |

For module init, `-EPROBE_DEFER` is usually not the right top-level answer unless you are inside a device probe path. Deferral is mainly part of the driver model.

## Failure Cleanup Pattern

Manual registration needs reverse-order cleanup:

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
        goto err_unregister_chrdev;

    demo_class = class_create("demo");
    if (IS_ERR(demo_class)) {
        ret = PTR_ERR(demo_class);
        goto err_del_cdev;
    }

    return 0;

err_del_cdev:
    cdev_del(&demo_cdev);
err_unregister_chrdev:
    unregister_chrdev_region(demo_devt, 1);
    return ret;
}

static void __exit demo_exit(void)
{
    class_destroy(demo_class);
    cdev_del(&demo_cdev);
    unregister_chrdev_region(demo_devt, 1);
}
```

The failure path mirrors the successful registrations. This pattern appears everywhere in kernel code.

Where possible in real device drivers, use `devm_*` helpers so per-device resources are tied to the device lifecycle. Module-global registrations still often require explicit cleanup.

## `__init` And `__exit`

The annotations:

```c
static int __init demo_init(void)
static void __exit demo_exit(void)
```

place code in special sections.

For built-in code, init sections can be freed after boot. For modules, the annotations are still conventional and useful. `__exit` code may be discarded for built-in drivers because built-in code is not unloaded.

Do not call `__init` functions from normal runtime paths. They may no longer exist after init memory is freed.

## Module Reference Ownership

A module cannot be safely removed while code inside it may still be called.

Common references come from:

- open character device files
- registered bus drivers
- sysfs callbacks
- active timers
- queued work
- IRQ handlers
- subsystem callbacks
- exported symbols used by another module

The kernel module loader tracks many references automatically through registered structures, file operations, and symbol dependencies, but your code must still stop asynchronous paths before unload completes.

Check references:

```sh
lsmod | grep demo
```

The `Used by` count is a clue, not a complete debugging strategy. A stuck unload usually means some registered interface or active reference is still alive.

## Unload Safety

Before module exit returns:

- stop hardware from generating interrupts
- disable or free IRQs
- cancel timers
- flush or cancel workqueues
- unregister devices and subsystem objects
- remove sysfs/debugfs files
- prevent new userspace opens
- wait for active callbacks where needed

Example work cleanup:

```c
static void demo_exit(void)
{
    cancel_work_sync(&demo_work);
    unregister_chrdev_region(demo_devt, 1);
}
```

Use the synchronous cleanup variant when the work function lives in the module being unloaded.

## Module Loading Errors

Common failures:

| Symptom | Likely Cause | Check |
| --- | --- | --- |
| `Invalid module format` | Wrong kernel release, arch, or module versioning | `modinfo`, `uname -r`, `dmesg` |
| `Unknown symbol` | Missing dependency or wrong `Module.symvers` | `dmesg`, `modinfo depends` |
| `Operation not permitted` | Lockdown, Secure Boot, unsigned module, policy | `dmesg`, module signing config |
| `No such device` | Init or probe returned `-ENODEV` | driver logs |
| `File exists` | Resource or name already registered | cleanup previous load |
| `Device or resource busy` | Resource already owned or module still in use | `lsmod`, `fuser`, driver logs |

Always read `dmesg` after a load failure:

```sh
dmesg | tail -n 80
```

The user-facing error often hides the exact kernel reason.

## External Module Compatibility

Check the running kernel:

```sh
uname -r
cat /proc/version
```

Check the module:

```sh
modinfo hello.ko | grep vermagic
```

Check the build tree:

```sh
make -C /lib/modules/$(uname -r)/build kernelrelease
```

For embedded targets, do not accidentally build against host headers:

```sh
make -C /path/to/linux-source \
  O=/path/to/build-arm64 \
  ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  M=$PWD modules
```

## Practice Exercises

### Exercise 1: Load And Unload A Minimal Module

Build the `hello.c` module, load it, inspect logs, and remove it.

Expected checks:

```sh
modinfo hello.ko
sudo insmod hello.ko
lsmod | grep hello
dmesg | tail -n 20
sudo rmmod hello
```

### Exercise 2: Add A Failure Path

Add a module parameter that forces init to fail:

```c
static bool fail_init;
module_param(fail_init, bool, 0444);

static int __init hello_init(void)
{
    if (fail_init)
        return -EINVAL;

    pr_info("hello loaded\n");
    return 0;
}
```

Load:

```sh
sudo insmod hello.ko fail_init=1
dmesg | tail -n 20
```

Confirm the module is not left loaded.

### Exercise 3: Break Compatibility Deliberately

Build a module against one kernel tree and try to load it on a different kernel. Observe:

```sh
modinfo hello.ko | grep vermagic
uname -r
dmesg | tail -n 50
```

This teaches why matching source/build/runtime identity matters.

## Debugging Checklist

- Did the module build against the running kernel?
- Does `modinfo` show the expected license, dependencies, and `vermagic`?
- Does `dmesg` show init, probe, or symbol errors?
- Did `insmod` fail before module init completed?
- If init failed halfway, did every earlier registration get unwound?
- Does `lsmod` show unexpected users?
- Does exit unregister every object registered by init?
- Are timers, work, IRQs, and callbacks stopped before unload?

## Related Topics

- [Modules And External Modules](../../build-systems/advanced/linux-kernel/modules-and-external-modules.md)
- [Kernel Build And Install Overview](../source-build-and-tailoring/kernel-build-and-install-overview.md)
- [Module Parameters And Driver Logging](module-parameters-and-logging.md)
- [Built-In Drivers Vs Loadable Modules](built-in-vs-loadable-modules.md)
- [Driver Binding, Probe, And Remove](driver-binding-probe-remove.md)

## Official References

- [Building External Modules](https://docs.kernel.org/kbuild/modules.html)
- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [The Kernel's Command-Line Parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
