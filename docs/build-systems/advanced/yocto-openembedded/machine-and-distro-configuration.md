---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Machine and Distro Configuration

## What Problem Does This Solve?

Machine configuration describes hardware. Distro configuration describes operating-system policy. Keeping them separate makes a BSP reusable across products and prevents hardware definitions from becoming entangled with product package and security policy.

## Core Concepts

- `MACHINE`
- machine configuration
- `DISTRO`
- distro configuration
- tune
- machine features
- distro features
- providers
- image formats
- BSP ownership
- product policy

## Mental Model

```text
machine = what hardware is this?
distro = what operating-system policy do we want?
image = which packages/capabilities form this product image?
```

These scopes interact but should remain distinguishable.

## Machine Configuration

Typically:

```text
conf/machine/product-board.conf
```

It can define:

- tune/architecture
- SoC includes
- kernel provider and machine mapping
- U-Boot provider/configuration
- device tree names
- serial consoles
- machine features
- firmware
- image/WIC formats
- boot artifact expectations

Conceptual example:

```bitbake
require conf/machine/include/soc-family.inc

MACHINEOVERRIDES =. "product-board:"
KERNEL_DEVICETREE = "vendor/product-board.dtb"
UBOOT_CONFIG = "sd"
SERIAL_CONSOLES = "115200;ttyS2"
IMAGE_FSTYPES += "wic"
```

Use variables supported by the active BSP layer and release.

## Tune And Package Architecture

Tune metadata controls:

- CPU architecture
- instruction set
- ABI
- floating-point behavior
- compiler flags
- package architecture compatibility

Wrong tune selection can produce binaries that fail on target or unnecessarily restrict package reuse.

Prefer established SoC/vendor tune includes over hand-written compiler flags.

## Machine Features

`MACHINE_FEATURES` describes hardware capabilities such as:

- Bluetooth
- Wi-Fi
- touchscreen
- PCI
- USB host
- RTC
- display

Recipes and package groups can use these features to enable hardware-related behavior.

Do not use machine features as arbitrary product marketing flags.

## Distro Configuration

Typically:

```text
conf/distro/product.conf
```

It can define:

- distro identity/version
- init system
- package format
- security/compiler policy
- default providers
- feature policy
- release/debug behavior
- package feed policy

Conceptual example:

```bitbake
DISTRO_NAME = "Product Linux"
DISTRO_VERSION = "1.0"
DISTRO_FEATURES:append = " systemd"
VIRTUAL-RUNTIME_init_manager = "systemd"
```

## Provider Selection

Virtual capabilities decouple consumers from concrete recipes.

Common BSP providers:

```text
virtual/kernel
virtual/bootloader
```

Inspect:

```sh
bitbake-layers show-recipes virtual/kernel
bitbake-layers show-recipes virtual/bootloader
bitbake -e virtual/kernel | grep '^PN='
bitbake -e virtual/bootloader | grep '^PN='
```

Provider selection can be global, distro-specific, or machine-specific. Hardware-specific provider choices usually belong in BSP/machine policy.

## Version Selection

Recipes can have multiple available versions. Preferences should be explicit and scoped.

Avoid selecting a version only because a layer priority happens to win. Record why a product tracks a particular vendor branch or release.

## Custom Board From Vendor EVM

Recommended workflow:

1. Select closest vendor EVM machine.
2. Build unchanged vendor image.
3. Create product layer.
4. Add custom machine that includes SoC-family metadata.
5. Set product DTB/U-Boot configuration.
6. Add product kernel/U-Boot appends.
7. Add product WIC/deployment layout.
8. Validate boot chain and rootfs.

Do not copy the complete vendor machine file and then lose upstream relationships. Include shared SoC files where the vendor layer supports it.

## Hardware Vs Product Policy Examples

Machine/BSP:

- SoC tune
- DTB
- bootloader config
- kernel provider
- firmware
- eMMC/SD boot layout

Distro/product:

- systemd policy
- security hardening
- package format
- update framework
- logging policy
- release identity

Image:

- application package groups
- factory/debug package selection
- concrete product composition

## Overrides

Machine overrides allow scoped metadata:

```bitbake
SRC_URI:append:product-board = " file://board.patch"
```

Use overrides when a change truly belongs only to that machine. A generic driver fix should not be hidden under a product-machine override if it should apply broadly.

## Multiconfig Awareness

Some systems build multiple configurations, architectures, or firmware contexts together. Multiconfig can model separate BitBake configurations with dependencies between them.

This is relevant for SoCs with auxiliary cores or separate firmware builds, but should be introduced only when the build genuinely needs multiple configuration contexts.

## TI Processor SDK Perspective

For TI SDK-based work, inspect:

- selected `MACHINE`
- TI distro or distro includes
- kernel provider
- U-Boot provider
- TI firmware recipes
- image target
- deploy artifact naming
- machine overrides

Create product machine and distro metadata in your own layer rather than editing TI layer files.

## Inspection Commands

```sh
bitbake -e | grep '^MACHINE='
bitbake -e | grep '^DISTRO='
bitbake -e | grep '^MACHINE_FEATURES='
bitbake -e | grep '^DISTRO_FEATURES='
bitbake -e virtual/kernel | grep -E '^(PN|PV|SRCREV)='
bitbake -e virtual/bootloader | grep -E '^(PN|PV|SRCREV)='
```

## Worked Example: Shared SoC, Two Products

```text
conf/machine/product-a.conf
conf/machine/product-b.conf
conf/machine/include/company-soc.inc
```

Shared include owns SoC/tune/provider defaults. Each machine owns DTB, U-Boot config, and board features. A common distro owns systemd/security/package policy. Separate images own application composition.

This avoids copying one complete machine file for every carrier-board variant.

## Worked Example: Verify Provider Scope

```sh
MACHINE=product-a bitbake -e virtual/kernel | grep -E '^(PN|PV|SRCREV)='
MACHINE=product-b bitbake -e virtual/kernel | grep -E '^(PN|PV|SRCREV)='
```

Prefer separate initialized build directories for routine work; the command illustrates that provider selection must be checked in each machine context.

## Common Mistakes

- Putting application package lists in machine config.
- Putting DTB selection in an image recipe.
- Copying a vendor EVM machine without understanding includes.
- Setting raw compiler flags instead of using tune metadata.
- Selecting providers accidentally through priority.
- Using machine overrides for generic fixes.
- Editing vendor machine/distro files directly.

## Debugging Checklist

- What machine and distro are active?
- Which tune is selected?
- Which machine/distro features are active?
- Which recipes provide kernel and bootloader?
- Which versions/source revisions are selected?
- Which DTBs and U-Boot configs are selected?
- Which image types and WIC layout apply?
- Is each policy in the correct scope?
- Are vendor and product ownership separated?

## Related Topics

- [Build Directory and Configuration](build-directory-and-configuration.md)
- [Layers](layers.md)
- [Images and Package Groups](images-and-packagegroups.md)
- [Kernel and Bootloader Integration](kernel-and-bootloader-integration.md)

## References

- Yocto Project BSP Developer's Guide
- Yocto Project Reference Manual
- BitBake User Manual
