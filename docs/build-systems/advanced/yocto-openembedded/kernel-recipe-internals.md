---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kernel Recipe Internals

## What Problem Does This Solve?

Kernel recipes combine BitBake tasks with Kconfig, Kbuild, source management, module packaging, DTB deployment, and machine policy. Understanding their internals is necessary when configuration fragments disappear, modules do not enter images, or deploy artifacts do not match runtime.

## Two Build Systems

```text
BitBake/OE layer:
  fetch, patch, configure policy, tasks, package, deploy

Linux kernel layer:
  Kconfig, Kbuild, Image, modules, DTBs
```

A failure must be assigned to the correct layer.

## Provider And Recipe Identity

```sh
bitbake-layers show-recipes virtual/kernel
bitbake -e virtual/kernel | grep -E '^(PN|PV|SRCREV|S|B|WORKDIR)='
```

Record provider, source revision, branch, recipe version, and machine before debugging.

## Typical Kernel Tasks

Kernel classes add tasks beyond a generic recipe. Names vary by release/provider, but common responsibilities include:

- source fetch/unpack/patch
- kernel metadata/configuration processing
- `do_configure`
- `do_compile`
- module installation
- package generation
- deploy kernel images/DTBs
- shared workdir/sysroot preparation

List actual tasks:

```sh
bitbake -c listtasks virtual/kernel
```

## Source And Build Trees

```sh
bitbake -e virtual/kernel | grep -E '^(WORKDIR|S|B)='
```

Distinguish:

- patched source `${S}`
- kernel output `${B}`
- shared kernel areas
- recipe workdir
- deploy output

Do not edit generated kernel source/output as the final change.

## Configuration Inputs

Possible inputs include:

- vendor defconfig
- machine-selected defconfig
- config fragments
- kernel metadata features
- recipe-specific configuration tasks
- interactive `menuconfig` output during development

The selected vendor recipe determines the exact supported mechanism.

## Worked Example: Config Fragment

Append layout:

```text
recipes-kernel/linux/
  linux-vendor_%.bbappend
  linux-vendor/product.cfg
```

Append:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI:append:product-board = " file://product.cfg"
```

Fragment:

```text
CONFIG_CAN=y
CONFIG_CAN_M_CAN=y
CONFIG_CAN_M_CAN_PLATFORM=y
```

Audit:

```sh
bitbake virtual/kernel -c configure -f
bitbake -e virtual/kernel | grep '^B='
grep '^CONFIG_CAN' <kernel-build>/.config
```

If requested symbols disappear, inspect Kconfig dependencies and fragment processing logs.

## `menuconfig` And `diffconfig`

Development flow can include:

```sh
bitbake virtual/kernel -c menuconfig
bitbake virtual/kernel -c diffconfig
```

Treat generated config differences as input for a maintained fragment/defconfig. Interactive workdir state is not durable.

## Kernel Patches

```bitbake
SRC_URI += "file://0001-arm64-dts-add-product-board.patch"
SRC_URI += "file://0002-can-fix-product-controller.patch"
```

Keep categories separate:

- board DTS
- generic driver fix
- product-only driver behavior
- temporary debug patch

Rebase against the exact selected source revision.

## DTB Selection And Deployment

Machine metadata can select DTBs:

```bitbake
KERNEL_DEVICETREE = "vendor/product-board.dtb"
```

Worked trace:

1. Find DTS in patched `${S}`.
2. Build `virtual/kernel` deploy task/image.
3. Find DTB under machine deploy directory.
4. Check WIC/boot partition contents.
5. Compare runtime `/proc/device-tree`.

Do not stop after finding a correct DTB in `${B}`.

## Module Packaging

Kernel module output is split into packages by kernel packaging infrastructure.

Worked trace for `example.ko`:

1. Final `.config` has `CONFIG_EXAMPLE=m`.
2. Kernel compile produces `example.ko`.
3. Packaging emits module package.
4. Product image/package group requests that package or appropriate aggregate.
5. Rootfs contains module under matching kernel release.
6. Target `modinfo` and `uname -r` agree.

Inspect package data and image manifest rather than assuming package name.

## Kernel Headers And External Modules

External module recipes inherit module support and depend on selected kernel build interfaces.

Matching requires:

- kernel version/config
- generated headers
- symbol versions
- compiler/toolchain
- packaging/deploy timing

Do not use host kernel headers.

## Kernel Version And Release Identity

Kernel recipe metadata can alter kernel version suffixes and package identity. Record:

- source commit
- recipe `PV/PR`
- kernel release string
- final config
- compiler
- deploy checksums

Module package paths depend on kernel release identity.

## Deploy Vs Rootfs Artifacts

Deploy:

- kernel image
- DTBs
- optional fitImage
- module archives/metadata depending on classes

Rootfs:

- `/lib/modules/<kernelrelease>` packages
- firmware packages
- userspace tools

Both must come from compatible build state.

## Worked Failure: New Kernel, Old Modules

Symptom:

```text
Invalid module format
```

Investigation:

```sh
uname -r
modinfo /lib/modules/.../example.ko
```

Then compare image manifest, boot partition kernel checksum, and module package build. Often the kernel was copied manually without rebuilding/flashing matching rootfs.

## Shared Workdir Consumers

Some recipes consume kernel shared work output, for example external modules or tools. Correct task dependencies must ensure configured/generated kernel data is available.

Use established classes instead of manually copying `${B}`.

## Common Mistakes

- Patching non-selected provider.
- Assuming fragment presence means final config value.
- Editing kernel workdir directly.
- Building module but not including its package.
- Deploying kernel without matching modules/DTB.
- Selecting DTB in recipe but not machine/image policy.
- Copying kernel artifacts manually around WIC integration.

## Debugging Checklist

- Which provider/version/revision?
- What are `${S}`, `${B}`, `${WORKDIR}`?
- Which config inputs apply and in what order?
- What is final `.config`?
- Did patches apply?
- Which image/DTB targets compile?
- Which DTBs deploy?
- Which module packages exist?
- Which packages enter image?
- Do WIC and runtime artifacts match deploy checksums?

## Related Topics

- [Kernel and Bootloader Integration](kernel-and-bootloader-integration.md)
- [Linux Kernel Build System](../linux-kernel/index.md)
- [Configuration Fragments and Auditing](../linux-kernel/configuration-fragments-and-auditing.md)
- [WIC and Partition Layouts](wic-and-partition-layouts.md)

## References

- Yocto Project Linux Kernel Development Manual
- Yocto Project kernel classes documentation
- Linux kernel Kbuild/Kconfig documentation
