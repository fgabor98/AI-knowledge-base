---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Device Tree in U-Boot

## What Problem Does This Solve?

U-Boot uses device tree to describe hardware for driver model, board selection, pre-relocation devices, and Linux handoff. The U-Boot DTB may be shared with the kernel, derived from it, or maintained separately.

For embedded Linux work, this matters because a peripheral may need to work in both U-Boot and Linux, but each stage can use a different DTB.

## Core Concepts

- U-Boot device tree
- `CONFIG_DEFAULT_DEVICE_TREE`
- driver model
- pre-relocation
- `u-boot.dtb`
- embedded DTB
- external DTB
- Linux handoff DTB
- U-Boot overlays/fixups

## Mental Model

There may be two related but distinct device trees:

```text
U-Boot DTB
  used by U-Boot drivers before Linux

Linux DTB
  passed to kernel during boot
```

Sometimes they come from the same source. Sometimes they do not.

## Finding The Selected U-Boot DTB

Check config:

```sh
grep '^CONFIG_DEFAULT_DEVICE_TREE' build/.config
```

Find matching source:

```sh
find arch -name '*board*.dts'
find dts -name '*board*.dts'
```

U-Boot source layout varies by version and architecture.

## Generated DTB

Common generated files:

```text
u-boot.dtb
u-boot-nodtb.bin
u-boot-dtb.bin
u-boot.itb
spl/u-boot-spl.dtb
```

The DTB may be appended to U-Boot, embedded in a FIT, or loaded separately.

## Driver Model

Modern U-Boot uses driver model. A driver usually needs:

- config symbol enabled
- matching compatible string
- DT node enabled
- required clocks/resets/regulators
- pre-relocation property if needed before relocation

If a driver is built but not active, check the DTB.

## Pre-Relocation Constraints

Some devices must work before U-Boot relocates itself to DRAM.

Examples:

- serial console
- clocks
- pinctrl
- MMC boot device
- SPI boot flash

U-Boot uses special properties for pre-relocation behavior, depending on version and driver model expectations.

If a device is needed early, verify both config and DT properties.

## U-Boot-Specific DTS Properties

U-Boot may use properties that Linux does not use, and Linux may use properties U-Boot ignores.

Examples:

- pre-relocation markers
- boot phase markers
- U-Boot-specific aliases
- environment or boot device policy

Keep U-Boot-specific changes clearly separated when possible.

## Shared Vs Separate DTS

### Shared DTS

Advantages:

- less duplication
- kernel and U-Boot describe same board
- easier hardware review

Disadvantages:

- U-Boot may not understand all kernel bindings
- early boot constraints differ
- U-Boot-specific properties can clutter kernel DTS

### Separate DTS

Advantages:

- stage-specific control
- easier to minimize SPL DTB
- avoids unsupported Linux-only binding complexity

Disadvantages:

- drift between U-Boot and Linux
- duplicated board descriptions
- more upgrade work

## Linux Handoff DTB

U-Boot may pass:

- the same DTB it used internally
- a loaded Linux DTB from boot partition
- a DTB inside a FIT image
- a DTB modified by overlays or fixups

Always verify what Linux received:

```sh
cat /proc/device-tree/model
cat /proc/device-tree/compatible
```

## FIT And Multiple DTBs

A FIT image can contain multiple DTBs and configurations. U-Boot selects one based on boot command, environment, or board logic.

If Linux boots with the wrong board description, inspect FIT configurations and U-Boot environment.

## Common Mistakes

- Editing Linux DTS when U-Boot uses a separate DTS.
- Enabling a U-Boot driver but missing its DT node.
- Forgetting pre-relocation requirements.
- Assuming U-Boot passes its own DTB to Linux.
- Updating DTB on boot partition but booting a FIT-contained DTB.
- Letting U-Boot and Linux DTS files drift without review.

## Debugging Checklist

- Check `CONFIG_DEFAULT_DEVICE_TREE`.
- Locate U-Boot DTS source.
- Build and inspect `u-boot.dtb`.
- Check whether SPL has its own DTB.
- Confirm driver config symbols.
- Confirm compatible strings.
- Confirm pre-relocation needs.
- Confirm Linux handoff DTB.
- Compare runtime `/proc/device-tree` with expected source.

## Related Topics

- [Kconfig and Generated Config](kconfig-and-generated-config.md)
- [FIT Images and Boot Artifacts](fit-images-and-boot-artifacts.md)
- [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)
- [Linux Kernel Device Tree Builds](../linux-kernel/device-tree-builds.md)

## References

- U-Boot driver model documentation
- U-Boot device tree documentation
- Linux devicetree documentation
