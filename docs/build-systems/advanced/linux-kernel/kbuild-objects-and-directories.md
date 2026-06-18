---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kbuild Objects and Directories

## What Problem Does This Solve?

Kbuild decides which source files and directories become part of the kernel image or modules. It connects `CONFIG_*` choices from Kconfig to actual object files.

If a driver source file exists but no object is built, Kbuild is usually where you find the reason.

## Core Concepts

- Kbuild
- `Makefile`
- `obj-y`
- `obj-m`
- `obj-$(CONFIG_FOO)`
- built-in object
- module object
- directory recursion
- composite objects
- generated dependencies
- `built-in.a`

## Mental Model

Kconfig answers:

```text
Should this feature be enabled?
```

Kbuild answers:

```text
Which object files should be built because of that choice?
```

Example:

```make
obj-$(CONFIG_MY_DRIVER) += my_driver.o
```

If `CONFIG_MY_DRIVER=y`, the object is built into the kernel. If `CONFIG_MY_DRIVER=m`, it is built as a module. If unset, it is not built.

## Syntax / API / Mechanism

Basic object selection:

```make
obj-y += core.o
obj-m += test_module.o
obj-$(CONFIG_FOO) += foo.o
```

Directory selection:

```make
obj-$(CONFIG_I2C) += i2c/
```

Composite object:

```make
obj-$(CONFIG_MY_DRIVER) += my_driver.o
my_driver-y := core.o bus.o sysfs.o
my_driver-$(CONFIG_MY_DRIVER_DEBUG) += debug.o
```

## Minimal Example

Kconfig:

```kconfig
config MY_DRIVER
    tristate "My driver"
    depends on I2C
```

Kbuild Makefile:

```make
obj-$(CONFIG_MY_DRIVER) += my_driver.o
```

Source:

```text
drivers/misc/my_driver.c
```

Final behavior:

```text
CONFIG_MY_DRIVER=y -> built into vmlinux
CONFIG_MY_DRIVER=m -> my_driver.ko
unset              -> not built
```

## Directory Structure

Kernel directories typically have their own Makefiles:

```text
drivers/
  Makefile
  i2c/
    Makefile
  net/
    Makefile
```

Parent directories include child directories conditionally:

```make
obj-$(CONFIG_I2C) += i2c/
```

Child directories select their own objects:

```make
obj-$(CONFIG_I2C_CHARDEV) += i2c-dev.o
```

Both levels must be reachable.

## Common Scenarios

### Source File Exists But Is Not Built

Check:

1. Is the Kconfig symbol enabled?
2. Does a Makefile reference the object?
3. Is the parent directory entered?
4. Is the object behind another config symbol?
5. Are you inspecting the correct output directory?

Commands:

```sh
grep CONFIG_MY_DRIVER build/.config
grep -R "my_driver.o" drivers/
make O=build V=1 drivers/misc/
```

### Driver Built Into Kernel Instead Of Module

If final config says:

```text
CONFIG_MY_DRIVER=y
```

then expect no `.ko`; it is built into `vmlinux`.

If you need a module:

```text
CONFIG_MY_DRIVER=m
```

but only if the symbol is `tristate`.

### Module Object Has Multiple Source Files

Composite module:

```make
obj-$(CONFIG_FOO) += foo.o
foo-y := foo_core.o foo_bus.o
foo-$(CONFIG_FOO_DEBUG) += foo_debug.o
```

The final module is `foo.ko`, not one module per source file.

### Generated Source Or Headers

Some kernel build steps generate headers or source. Missing dependencies can show up as intermittent parallel build failures. Kbuild has conventions for generated files; follow existing nearby patterns instead of inventing ad hoc shell commands.

## Built-In vs Module

Built-in:

- linked into `vmlinux`
- available during early boot
- cannot be unloaded
- needed for rootfs storage, early console, or mandatory platform support

Module:

- built as `*.ko`
- installed into `/lib/modules/<kernel-version>/`
- can be loaded later
- useful for optional peripherals or development

Embedded products often build core board support in and optional drivers as modules.

## Common Mistakes

- Adding a source file but not adding it to Kbuild.
- Enabling Kconfig but forgetting parent directory selection.
- Looking for a `.ko` when the driver is built in.
- Setting `CONFIG_FOO=m` for boot-critical storage support.
- Editing generated Kbuild output instead of source Makefiles.
- Adding broad unconditional `obj-y` entries for optional product drivers.
- Ignoring composite object naming.

## Debugging Checklist

- Find the Kconfig symbol.
- Check final `.config`.
- Search for the object in Kbuild files.
- Check parent directory `obj-*` selection.
- Build with `V=1`.
- Check whether expected output is built-in or module.
- Search output tree for object files.
- Confirm module install path if built as module.

## Related Topics

- [Kconfig and Defconfig](kconfig-and-defconfig.md)
- [Modules and External Modules](modules-and-external-modules.md)
- [Debugging Kernel Builds](debugging-kernel-builds.md)
- [Configuration and Patch Ownership](../bsp-integration/configuration-and-patch-ownership.md)

## References

- Linux kernel Kbuild documentation
- Linux kernel Makefiles documentation
- Linux kernel module documentation
