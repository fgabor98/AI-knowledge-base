---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Kernel and Bootloader Integration

## What Problem Does This Solve?

Yocto integrates the Linux kernel and U-Boot into the complete BSP build. It selects providers, fetches vendor sources, applies patches and configuration, invokes component build systems, packages modules, deploys boot artifacts, and assembles images.

Understanding this boundary is essential: Yocto orchestrates kernel and U-Boot builds, but Kconfig, Kbuild, SPL, FIT, and board boot-flow rules still apply inside those components.

## Core Concepts

- `virtual/kernel`
- `virtual/bootloader`
- kernel recipe
- U-Boot recipe
- `.bbappend`
- config fragment
- defconfig
- device tree
- kernel modules
- deploy artifacts
- WIC integration
- firmware
- provider selection

## Integration Mental Model

```text
machine/provider policy
-> kernel and U-Boot recipes
-> source revisions + product patches/config
-> component build systems
-> packages and deploy artifacts
-> rootfs/WIC/boot partition
-> board boot chain
```

## Identify Providers First

```sh
bitbake-layers show-recipes virtual/kernel
bitbake-layers show-recipes virtual/bootloader
bitbake -e virtual/kernel | grep -E '^(PN|PV|SRCREV)='
bitbake -e virtual/bootloader | grep -E '^(PN|PV|SRCREV)='
```

Before patching anything, know the selected recipe, source branch, source revision, and workdir.

## Kernel Recipe Append

Typical product layout:

```text
meta-product/
  recipes-kernel/linux/
    linux-vendor_%.bbappend
    linux-vendor/
      product.cfg
      0001-arm64-dts-add-product-board.patch
      0002-driver-fix.patch
```

Conceptual append:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://product.cfg"
SRC_URI += "file://0001-arm64-dts-add-product-board.patch"
```

Exact kernel fragment mechanisms depend on the kernel recipe/class and vendor layer.

## Kernel Configuration

Separate:

- vendor defconfig/base policy
- custom board requirements
- product features
- debug configuration
- production hardening

Audit final configuration in the kernel build workdir. A fragment file existing in metadata does not prove its settings survived Kconfig dependency resolution.

Useful tasks can include:

```sh
bitbake virtual/kernel -c menuconfig
bitbake virtual/kernel -c diffconfig
```

Availability and exact behavior depend on inherited kernel classes.

## Kernel Device Trees

Device tree changes can be carried as:

- patches to the kernel source DTS
- additional DTS files supplied by metadata, where recipe support exists
- vendor-specific mechanisms

Machine metadata commonly selects deployed DTBs through variables such as `KERNEL_DEVICETREE`.

Validate three identities:

```text
source DTS changed
-> expected DTB deployed
-> expected DTB included/flashed and observed at runtime
```

## Kernel Modules

Kernel modules are packaged into binary packages. Building modules does not automatically install every module in the image.

Check:

- module was built (`CONFIG_*=m`)
- module package exists
- package is included through image/package-group policy
- `/lib/modules/<kernelrelease>` matches deployed kernel

Avoid blanket inclusion of all modules in production unless that is an explicit product decision.

## External Module Recipe

External modules normally use kernel module class support.

Conceptual recipe:

```bitbake
SUMMARY = "Product kernel module"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=<checksum>"

inherit module

SRC_URI = "git://example.invalid/module.git;branch=main;protocol=https"
SRCREV = "<fixed-commit>"
S = "${WORKDIR}/git"
```

The class supplies integration with the selected kernel build. Do not build against host `/lib/modules`.

## U-Boot Recipe Append

Typical layout:

```text
recipes-bsp/u-boot/
  u-boot-vendor_%.bbappend
  u-boot-vendor/
    0001-add-product-board.patch
    product-board.cfg
```

Changes may include:

- board defconfig
- U-Boot DTS
- SPL config
- board code
- environment defaults
- FIT packaging

Keep persistent environment behavior in mind: rebuilding defaults does not overwrite saved environment on deployed boards.

## U-Boot Configurations

Some recipes support multiple U-Boot configurations through metadata such as `UBOOT_CONFIG`.

Machine configuration can select:

- board defconfig
- boot media variant
- SPL/U-Boot artifact combinations
- deployment suffixes

Use the exact vendor recipe interface. Do not assume all U-Boot recipes expose identical variables.

## Deploy Artifacts

Inspect:

```sh
find tmp/deploy/images/${MACHINE} -maxdepth 1 -type f
```

Possible artifacts:

- kernel image
- DTBs/overlays
- kernel modules archive or packages
- U-Boot proper
- SPL/TPL
- TI boot-chain artifacts
- firmware
- initramfs
- FIT images
- WIC images

Symlinks may point to versioned files. Release manifests should capture the resolved artifact identities and checksums.

## WIC And Boot Partition Integration

WIC or image classes select deployed artifacts for disk images.

A correct kernel deploy output can still be absent from the final media if:

- WIC references another filename
- boot partition plugin selects another artifact
- old symlink points unexpectedly
- bootloader raw offset is wrong
- image task was not rerun after artifact changes

Trace deploy output into final disk image, then verify the flashed board.

## Firmware Integration

SoCs may require:

- system firmware
- DDR firmware
- remoteproc firmware
- PRU firmware
- GPU/media firmware
- Wi-Fi firmware

Firmware can enter through packages, boot partitions, or bootloader-stage packaging. Record ownership and version for each artifact.

## Development Workflow

Kernel change:

1. Identify selected provider/source revision.
2. Use devtool/devshell or a separate source clone.
3. Implement and test change.
4. Export clean patch or config fragment.
5. Add to product layer append.
6. Rebuild provider.
7. Inspect final config/deploy output.
8. Rebuild image if integration requires it.
9. Deploy and verify runtime identity.

Use the same discipline for U-Boot, including SPL and saved environment checks.

## TI Processor SDK Perspective

For TI systems, align:

- SDK release
- `MACHINE`
- kernel provider/commit
- U-Boot provider/commit
- system firmware
- security variant
- `tiboot3.bin`, `tispl.bin`, and `u-boot.img`-style artifacts where applicable
- kernel/DTB/modules/rootfs

Do not mix artifacts from different SDK builds even when filenames match.

## Worked Example: Trace A Kernel DTB Into WIC

```sh
bitbake -e virtual/kernel | grep '^KERNEL_DEVICETREE='
bitbake virtual/kernel
find tmp/deploy/images/${MACHINE} -name '*product*.dtb' -type f
bitbake product-image
wic ls tmp/deploy/images/${MACHINE}/product-image-*.wic:1
```

Then compare the boot partition DTB checksum with deploy output and runtime `/proc/device-tree/model`.

## Worked Example: U-Boot Patch Does Not Affect Board

Check in order:

1. `bitbake-layers show-appends` confirms append.
2. `virtual/bootloader` provider is expected recipe.
3. Patch appears in patched `${S}`.
4. New SPL/U-Boot artifacts appear in deploy.
5. WIC/media includes those artifacts.
6. Serial banner proves board runs them.
7. Saved environment does not override new default boot flow.

## Common Mistakes

- Patching a recipe that is not the selected provider.
- Assuming config fragments applied without checking final `.config`.
- Building a module but not including its package.
- Deploying kernel without matching DTB/modules.
- Updating U-Boot proper but not SPL/firmware.
- Editing vendor layer directly.
- Inspecting deploy output but not final WIC image.
- Ignoring saved U-Boot environment.

## Debugging Checklist

- Which kernel and bootloader providers are selected?
- Which source revisions are used?
- Did appends match?
- Did patches apply in expected order?
- What is final kernel/U-Boot config?
- Which DTBs are deployed?
- Which module packages exist and enter rootfs?
- Which boot artifacts enter WIC/media?
- Are firmware and security variants aligned?
- Does serial/runtime output prove the new artifacts run?

## Related Topics

- [Machine and Distro Configuration](machine-and-distro-configuration.md)
- [Tasks and Workdirs](tasks-and-workdirs.md)
- [Linux Kernel Build System](../linux-kernel/index.md)
- [U-Boot Build System](../u-boot/index.md)
- [BSP Artifact Flow](../bsp-integration/artifact-flow-and-provenance.md)

## References

- Yocto Project Linux Kernel Development Manual
- Yocto Project BSP Developer's Guide
- Yocto Project Reference Manual
