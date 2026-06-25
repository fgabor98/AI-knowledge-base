---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kernel Integration

## Goal

Learn how the TI SDK builds, configures, patches, and deploys the Linux kernel.

## What The Kernel Recipe Wraps

The TI SDK kernel recipe wraps the normal Linux kernel build system. Yocto does not replace Kbuild. It fetches source, applies patches, configures the kernel, runs the kernel build, installs modules, deploys images and DTBs, and packages outputs.

```text
BitBake recipe
-> fetch TI kernel source
-> apply patches
-> apply config policy
-> run Kbuild
-> install modules
-> deploy kernel image and DTBs
-> package modules and headers
```

## Provider Selection

Inspect the kernel provider:

```bash
bitbake-layers show-recipes virtual/kernel
bitbake -e virtual/kernel | grep '^PREFERRED_PROVIDER_virtual/kernel'
bitbake -e virtual/kernel | grep '^PN='
```

Provider selection can come from distro, machine, or layer policy. Do not assume the recipe name from memory.

## Kernel Configuration

Kernel configuration may come from:

- in-tree defconfig
- recipe variables
- config fragments
- machine-specific metadata
- product `.bbappend`
- interactive changes saved back into fragments

For product work, prefer product-owned config fragments over manual edits to generated `.config`.

Example pattern:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://product.cfg \
"
```

Then `product.cfg` might contain:

```text
CONFIG_IKCONFIG=y
CONFIG_IKCONFIG_PROC=y
CONFIG_CAN=y
```

Always verify the final config:

```bash
bitbake virtual/kernel -c menuconfig
bitbake virtual/kernel -c diffconfig
bitbake virtual/kernel -c savedefconfig
```

Availability of helper tasks depends on the kernel recipe/classes in the selected release.

## Device Trees

DTBs are usually built by the kernel recipe. Product board hardware changes should be represented in DTS/DTSI files and selected by machine metadata.

Common product changes:

- pinmux
- regulators
- GPIO polarity
- I2C/SPI device nodes
- Ethernet PHY configuration
- MMC/eMMC/SD settings
- display interface
- reserved memory
- remoteproc nodes

Rebuild:

```bash
bitbake virtual/kernel -c compile -f
bitbake virtual/kernel
```

Then inspect `tmp/deploy/images/<machine>/` for updated DTBs.

## Kernel Patches

Use a `.bbappend` in your product layer:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-driver-product-change.patch \
"
```

Patch rules:

- keep patches small and reviewable
- include upstream status where appropriate
- separate vendor backports from product hacks
- avoid editing TI kernel source directly in `tmp/work`
- record why the patch exists

## Modules

Kernel modules may be built in-tree or externally. In-tree modules follow kernel config and packaging. External modules usually need separate recipes depending on `virtual/kernel`.

Runtime mismatch symptoms:

- `invalid module format`
- unresolved symbols
- module loads but device never probes
- module missing from rootfs

Check:

```bash
uname -r
modinfo <module>
cat /lib/modules/$(uname -r)/modules.dep
```

## Common Mistakes

- Editing `.config` under `tmp/work` and losing changes.
- Updating a DTB in deploy but booting an old DTB from another partition.
- Building modules against one kernel and booting another.
- Putting board DTS changes in an application layer with unclear ownership.
- Assuming TI kernel branch names map cleanly to mainline releases.

## Related Topics

- [Linux Kernel Build System](../linux-kernel/index.md)
- [Device Tree Builds](../linux-kernel/device-tree-builds.md)
- [Custom Sitara Board Bring-Up](custom-sitara-board-bring-up.md)
