---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# End-to-End Product Layer Lab

## Goal

Build a small but structurally correct product layer that demonstrates:

- layer creation
- application recipe
- systemd service
- configuration package
- package group
- image recipe
- custom machine extension
- kernel config/DTB append
- U-Boot append
- WIC layout
- build inspection
- release artifacts

Adapt names and vendor interfaces to the active Yocto release and BSP.

## Prerequisites

- working reference image for a supported machine
- initialized build environment
- vendor BSP layers active
- known kernel/U-Boot providers
- enough disk space
- test board or emulator

Record baseline:

```sh
bitbake-layers show-layers
bitbake -e | grep -E '^(MACHINE|DISTRO)='
bitbake -e virtual/kernel | grep -E '^(PN|PV|SRCREV)='
bitbake -e virtual/bootloader | grep -E '^(PN|PV|SRCREV)='
```

## Step 1: Create Layer

```sh
bitbake-layers create-layer ../meta-product
bitbake-layers add-layer ../meta-product
bitbake-layers show-layers
```

Target layout:

```text
meta-product/
  conf/
    layer.conf
    machine/product-board.conf
    distro/product.conf
  recipes-apps/product-agent/
  recipes-core/images/
  recipes-core/packagegroups/
  recipes-kernel/linux/
  recipes-bsp/u-boot/
  wic/product.wks.in
```

Review layer dependencies and release compatibility.

## Step 2: Application Source

Example C source:

```c
#include <stdio.h>

int main(void)
{
    puts("product-agent 1.0");
    return 0;
}
```

Store source in a separate Git repository for realistic development, or use local recipe files for the exercise.

## Step 3: Application Recipe

```bitbake
SUMMARY = "Product agent"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=<checksum>"

SRC_URI = "file://product-agent.c \
           file://product-agent.service \
           file://product-agent.conf \
"

S = "${WORKDIR}"

inherit systemd

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} ${S}/product-agent.c -o product-agent
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 product-agent ${D}${bindir}/product-agent

    install -d ${D}${sysconfdir}/product
    install -m 0644 ${WORKDIR}/product-agent.conf \
        ${D}${sysconfdir}/product/agent.conf

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/product-agent.service \
        ${D}${systemd_system_unitdir}/product-agent.service
}

SYSTEMD_SERVICE:${PN} = "product-agent.service"
FILES:${PN} += "${sysconfdir}/product"
```

Service:

```ini
[Unit]
Description=Product Agent
After=network.target

[Service]
ExecStart=/usr/bin/product-agent
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

Build and inspect:

```sh
bitbake product-agent
bitbake -e product-agent | grep -E '^(WORKDIR|D)='
```

## Step 4: Split Configuration Package

Extend recipe:

```bitbake
PACKAGES =+ "${PN}-config"
FILES:${PN}-config = "${sysconfdir}/product"
RDEPENDS:${PN} += "${PN}-config"
```

Rebuild and inspect `packages-split` to prove ownership.

## Step 5: Package Group

`packagegroup-product-base.bb`:

```bitbake
SUMMARY = "Product base packages"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    product-agent \
    iproute2 \
"
```

Add only capabilities required by the exercise/product.

## Step 6: Image Recipe

`product-image.bb`:

```bitbake
SUMMARY = "Product Linux image"
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL:append = " packagegroup-product-base"
IMAGE_FSTYPES += "wic"
```

Build package before image, then:

```sh
bitbake product-image
```

Inspect image manifest and rootfs/deploy artifacts.

## Step 7: Product Distro

`conf/distro/product.conf` should include a supported baseline distro or define required policy deliberately.

Conceptual additions:

```bitbake
DISTRO_NAME = "Product Linux"
DISTRO_VERSION = "1.0"
DISTRO_FEATURES:append = " systemd"
VIRTUAL-RUNTIME_init_manager = "systemd"
```

Do not create a distro by copying a large reference file without understanding inherited policy.

## Step 8: Product Machine

Start from the closest vendor machine include structure.

Conceptual:

```bitbake
require conf/machine/include/vendor-soc.inc

MACHINEOVERRIDES =. "product-board:"
KERNEL_DEVICETREE = "vendor/product-board.dtb"
WKS_FILE = "product.wks.in"
```

Real vendor machine files can require many additional variables. Build the vendor EVM unchanged before introducing this step.

## Step 9: Kernel Append

Layout:

```text
recipes-kernel/linux/
  linux-vendor_%.bbappend
  linux-vendor/
    product.cfg
    0001-arm64-dts-add-product-board.patch
```

Append:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI:append:product-board = " \
    file://product.cfg \
    file://0001-arm64-dts-add-product-board.patch \
"
```

Fragment example:

```text
CONFIG_CAN=y
CONFIG_CAN_RAW=y
```

Validate final `.config`, deployed DTB, and runtime DTB.

## Step 10: U-Boot Append

Layout:

```text
recipes-bsp/u-boot/
  u-boot-vendor_%.bbappend
  u-boot-vendor/
    0001-add-product-board.patch
```

Append:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI:append:product-board = " file://0001-add-product-board.patch"
```

Validate:

- selected provider/config
- SPL and U-Boot proper artifacts
- U-Boot DTB
- default vs saved environment
- serial version from reset

## Step 11: WIC Layout

`wic/product.wks.in`:

```text
part /boot --source bootimg-partition --fstype=vfat \
    --label boot --active --align 4 --size 128

part / --source rootfs --fstype=ext4 \
    --label rootfs --align 4 --size 1536

part /data --fstype=ext4 --label data \
    --align 4 --size 512

bootloader --ptable gpt
```

Adapt source plugin and raw boot regions to the vendor BSP.

## Step 12: Build And Inspect

```sh
bitbake-layers show-appends
bitbake -e virtual/kernel | grep -E '^(PN|PV|SRCREV|KERNEL_DEVICETREE)='
bitbake -e virtual/bootloader | grep -E '^(PN|PV|SRCREV)='
bitbake product-image
```

Inspect:

- task logs
- final kernel config
- package split
- image manifest
- machine deploy directory
- WIC partition table
- boot partition files/checksums

## Step 13: Flash And Runtime Validate

Before flashing, know recovery path.

Capture:

- serial from reset
- SPL/U-Boot versions
- selected boot artifacts
- kernel `uname -a`
- `/proc/cmdline`
- `/proc/device-tree/model`
- `/lib/modules` release
- service status/output
- data partition mount

## Step 14: Release Bundle

Create:

```text
release/
  manifest.json
  images/product-image-*.wic
  boot-artifacts/
  sdk/
  compliance/
  logs/build.log
  logs/serial.log
  checksums.sha256
```

Manifest records layer/source revisions, providers, machine/distro/image, artifact checksums, and validation job.

## Step 15: Clean Rebuild Test

1. Reset devtool workspace.
2. Use clean build directory.
3. Reuse approved downloads/sstate.
4. Build from pinned layer manifest.
5. Compare artifacts/manifests.
6. Run hardware smoke test.

## Extension Exercises

- add `product-image-debug` with diagnostics package group
- add external kernel module recipe
- add initramfs image
- add a signed FIT flow
- add remoteproc firmware through multiconfig
- generate standard SDK and build an application with it
- enable SBOM/CVE output
- add CI artifact promotion

## Completion Criteria

You can explain and demonstrate:

- ownership of every metadata file
- recipe-to-package-to-image flow
- selected kernel/U-Boot providers
- final kernel/U-Boot configuration
- deploy-to-WIC artifact flow
- flashed runtime identity
- compliance/release outputs
- reproducible clean build process

## Common Mistakes

- Starting custom board work before vendor EVM builds.
- Keeping changes in `local.conf` or `tmp/work`.
- Adding recipe rather than package to image.
- Mixing kernel/U-Boot artifacts across builds.
- Copying an EVM WKS without boot-ROM review.
- Validating a devtool-deployed target instead of clean image.
- Releasing without manifests/checksums/logs.

## Related Topics

- [Layers](layers.md)
- [Recipes](recipes.md)
- [Kernel and Bootloader Integration](kernel-and-bootloader-integration.md)
- [WIC and Partition Layouts](wic-and-partition-layouts.md)
- [CI, Hash Equivalence, and Shared State](ci-hash-equivalence-and-sstate.md)

## References

- Yocto Project Development Tasks Manual
- Yocto Project BSP Developer's Guide
- Yocto Project Reference Manual
