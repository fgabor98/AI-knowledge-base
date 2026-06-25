---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# BSP Build Integration

## What Problem Does This Solve?

BSP build integration explains how kernel, U-Boot, device tree, firmware, root filesystem, image layout, and update artifacts become one bootable embedded Linux system.

This is the bridge between component build knowledge and complete product builds.

A board support package is not just a kernel tree. In practice, a BSP is the coordinated set of sources, patches, configurations, firmware, tools, images, documentation, and deployment rules needed to boot Linux on a specific board or SoC family.

This topic teaches how to reason about that coordination.

This page is the overview for the BSP integration section. Use the focused modules for deeper practice:

1. [Artifact Flow and Provenance](bsp-integration/artifact-flow-and-provenance.md)
2. [Configuration and Patch Ownership](bsp-integration/configuration-and-patch-ownership.md)
3. [Boot Debugging and Runtime Validation](bsp-integration/boot-debugging-and-runtime-validation.md)
4. [Image Layout and Deployment](bsp-integration/image-layout-and-deployment.md)
5. [BSP Release Reproducibility](bsp-integration/release-reproducibility.md)

## Core Concepts

- BSP
- boot chain
- source baseline
- vendor patches
- product patches
- defconfigs
- kernel config fragments
- U-Boot environment
- device trees
- boot artifacts
- root filesystem
- image generation
- deployment artifacts
- firmware blobs
- update bundles
- source provenance
- release manifests
- flashing workflow
- runtime validation

## Mental Model

Track artifacts through the system:

```text
vendor/source baseline
-> product patches and config
-> U-Boot/SPL/TPL artifacts
-> kernel image and modules
-> DTBs and overlays
-> root filesystem
-> partitioned image or update bundle
-> board flash/storage
```

Each layer has different ownership. Put changes where future you can find, rebase, test, and release them.

The central discipline is artifact traceability:

```text
runtime behavior
-> boot log or file on target
-> deployed artifact
-> build output
-> build task
-> source tree, patch, config, or recipe
```

When a board fails to boot, avoid starting with guesses. First identify which artifact is wrong, stale, missing, incompatible, or loaded from an unexpected location.

## System Boundary

BSP integration sits between component build systems and product release engineering:

```text
component build systems:
  U-Boot, Linux kernel, device tree compiler, application builds

BSP integration:
  selects versions, patches, configs, artifacts, image layout

productization:
  release manifest, factory flashing, OTA, secure boot, HIL, diagnostics
```

The boundary is not perfectly sharp, but it is useful:

- if the question is "how is this driver object selected?", it belongs to kernel build knowledge
- if the question is "which DTB goes into the image?", it belongs to BSP integration
- if the question is "how is the signed update rolled out?", it belongs to productization

## Artifact Map

A typical embedded Linux BSP produces or consumes these artifacts:

```text
early boot:
  ROM-selected boot header
  SPL / TPL / tiboot3 / platform first-stage artifacts
  U-Boot proper
  U-Boot environment or default environment

kernel boot:
  kernel image: Image, zImage, uImage, fitImage, bzImage
  device tree blobs: *.dtb
  overlays: *.dtbo where used
  initramfs where used

runtime:
  root filesystem image or archive
  kernel modules
  firmware files
  systemd units or init scripts
  application binaries
  config files

image/deploy:
  WIC image
  SD card image
  eMMC flashing image
  UBI/UBIFS images
  update bundle
  SDK/toolchain
  debug symbols
  manifests and licenses
```

Every artifact should have an owner and a source.

Example ownership table:

```text
artifact                     source owner
--------------------------   --------------------------------
SPL                          U-Boot recipe/package/config
u-boot.img                   U-Boot recipe/package/config
Image                        kernel recipe/config/patches
board.dtb                    kernel DTS/DTSI patches
*.ko                         kernel config/modules install
rootfs.ext4                  image recipe or system builder
systemd service file         application recipe/package
firmware blob                BSP firmware package
sdcard.wic                   image layout metadata
RAUC/SWUpdate bundle         update system metadata
```

## Boot Chain View

Use the boot chain to localize failures:

```text
BootROM
-> first-stage loader: SPL/TPL/platform artifact
-> U-Boot proper
-> kernel image
-> device tree
-> initramfs or rootfs
-> init system
-> product services
```

Typical failure ownership:

```text
ROM cannot load first stage
  boot media layout, boot headers, signed image, wrong flashing offset

SPL starts but U-Boot does not
  DDR init, boot media driver, SPL size, wrong second-stage path

U-Boot starts but kernel does not
  boot command, kernel path, DTB path, filesystem support, load address

kernel starts but panics
  wrong DTB, missing rootfs, wrong root=, missing driver, bad initramfs

rootfs mounts but services fail
  package content, config files, permissions, systemd units, device nodes
```

This mapping prevents wasting time in Yocto metadata when the real problem is a stale boot partition, or debugging U-Boot when the kernel is loading the wrong DTB.

## Repository And Metadata Layers

Different systems express BSP integration differently.

### Yocto / OpenEmbedded

Common ownership:

```text
meta-vendor/
  vendor BSP machines, recipes, patches

meta-soc/
  SoC-family support

meta-board/
  board-specific support

meta-product/
  product policy, image content, app integration, final patches
```

Common files:

```text
conf/machine/*.conf
recipes-kernel/linux/*.bbappend
recipes-bsp/u-boot/*.bbappend
recipes-bsp/firmware/
recipes-core/images/
wic/*.wks
```

Rule of thumb: keep product changes in a product layer. Avoid editing vendor layers directly unless you are intentionally maintaining a fork.

### Buildroot

Common ownership:

```text
board/<vendor>/<board>/
configs/<board>_defconfig
package/<name>/
BR2_EXTERNAL/
```

Common integration points:

- Buildroot defconfig
- rootfs overlay
- post-build scripts
- post-image scripts
- kernel/U-Boot config and patches
- generated images through genimage or board scripts

Rule of thumb: keep board/product customizations in `BR2_EXTERNAL` where possible.

### TI Processor SDK Linux

TI Processor SDK Linux is Yocto/OE-based, but the release model matters.

Common ownership:

```text
oe-layersetup config
Arago/TI layers
selected MACHINE
TI image target
deploy-ti artifacts
custom product layer
```

Rule of thumb: pin the SDK release first, then keep all documentation, layer revisions, machine names, kernel/U-Boot providers, and image targets aligned with that release.

## Roadmap

### 1. Artifact Flow

Learn:

- which source repositories produce which artifacts
- where each artifact appears in the build output
- which artifacts are copied into boot partitions
- which artifacts are installed into the root filesystem
- which artifacts are signed, packed, or transformed

Practice:

- build an image
- list all generated artifacts
- map each artifact to its source and build step
- boot the image and confirm artifact versions at runtime

Useful commands:

```sh
find build -name '*.dtb' -o -name 'Image' -o -name 'zImage' -o -name 'u-boot*'
find deploy -maxdepth 3 -type f
```

In Yocto:

```sh
find tmp/deploy -maxdepth 4 -type f
```

In TI Processor SDK, inspect the documented deploy output, commonly under a `deploy-ti` area for the selected release.

### 2. Configuration Ownership

Learn where each kind of configuration belongs:

- kernel config: defconfig, fragments, recipe metadata
- U-Boot config: board defconfig, Kconfig, environment policy
- device tree: kernel tree, U-Boot tree, vendor overlays, product overlays
- rootfs content: image recipe, package groups, Buildroot config, package manifests
- services: recipes/packages, systemd units, init scripts
- image layout: WIC, genimage, vendor image tools, partition scripts

Bad ownership examples:

- changing final `.config` but not preserving the fragment or defconfig
- editing a generated DTB instead of the DTS source
- copying files manually into a built rootfs instead of packaging them
- editing a vendor layer for product application policy
- modifying U-Boot environment only at the prompt and never recording the default

Good ownership examples:

- product kernel options in a product-layer config fragment
- application service installed by the application package
- board pinmux change in board DTS/DTSI source
- image partition layout in WIC/genimage metadata
- temporary vendor workaround as a named patch with a removal condition

### 3. Patch Ownership

Separate patch classes:

- upstream fix
- vendor BSP patch
- board enablement patch
- product policy patch
- temporary workaround
- release-only patch

Every patch should have a reason, owner, expected lifetime, and upstream/vendor status.

Patch metadata to preserve:

```text
subject:
  short description

reason:
  bug fix, board enablement, product policy, temporary workaround

scope:
  upstream, vendor BSP, board, product

status:
  upstreamed, submitted, vendor pending, product-only, temporary

remove when:
  upstream release X, vendor SDK Y, hardware revision Z
```

This is not bureaucracy. It is what keeps BSP upgrades from becoming archaeology.

### 4. Boot Debug Trace

Learn to trace a failed boot by layer:

```text
ROM cannot load first stage
SPL cannot initialize RAM or load U-Boot
U-Boot cannot find kernel/DTB/rootfs
kernel cannot mount rootfs
init cannot start services
application cannot find devices or config
```

Each failure points to different build artifacts and configuration owners.

Practical workflow:

1. Capture the full serial log from reset.
2. Mark the first failure.
3. Identify which boot stage printed the failing line.
4. Identify which artifact implements that stage.
5. Confirm the board loaded the artifact you just built.
6. Only then inspect source, config, patches, or recipes.

Version strings help. Make sure U-Boot and Linux builds include enough version information to distinguish old and new artifacts.

### 5. Product Build Reproducibility

Learn to preserve:

- exact source revisions
- layer revisions
- patch series
- config files
- toolchain versions
- image generation inputs
- release manifests
- debug symbols
- source archives and license artifacts

Minimum release traceability:

```text
product version
Git revisions and tags
Yocto layers or Buildroot tree revisions
SDK version
MACHINE / board config
image target
kernel config source
U-Boot defconfig source
device tree source
patch list
toolchain version
build host/container identity
artifact checksums
```

## Build-System Specific Integration

### Kernel Integration

Kernel integration usually owns:

- kernel source revision
- vendor or product patches
- defconfig or config fragments
- module selection
- device tree sources
- firmware install dependencies
- module install into rootfs
- kernel image deploy into boot partition

Important outputs:

```text
vmlinux
Image / zImage / fitImage
*.dtb
*.dtbo
*.ko
System.map
Module.symvers
```

Common checks:

```sh
strings Image | head
find rootfs -name '*.ko'
find boot -name '*.dtb'
```

### U-Boot Integration

U-Boot integration usually owns:

- U-Boot source revision
- board defconfig
- SPL/TPL enablement
- default environment
- boot commands
- boot media assumptions
- FIT support and signing where used
- board-specific image packing

Important outputs:

```text
SPL
spl/u-boot-spl.bin
u-boot
u-boot.bin
u-boot.img
u-boot.itb
board-specific boot images
```

Common checks:

```sh
strings u-boot.bin | grep -i 'U-Boot'
find deploy -name 'u-boot*' -o -name 'SPL'
```

### Device Tree Integration

Device tree integration usually owns:

- board DTS/DTSI source
- SoC `.dtsi` includes
- pinmux
- regulators
- clocks
- buses and child devices
- Ethernet PHY descriptions
- aliases and chosen nodes
- overlays where used

Important distinction:

```text
kernel DTB: describes hardware to Linux
U-Boot DTB: describes hardware to U-Boot driver model
```

They may share source or diverge depending on platform and vendor workflow.

Runtime checks:

```sh
ls /proc/device-tree
tr -d '\0' < /proc/device-tree/model
find /proc/device-tree -name status
```

### Root Filesystem Integration

Rootfs integration usually owns:

- package selection
- service units
- config files
- firmware files
- users and groups
- permissions
- init system policy
- writable data directories
- package manifests

Common checks:

```sh
find rootfs -path '*systemd*' -type f
find rootfs/lib/modules -type f
find rootfs/lib/firmware -type f
```

### Image Layout Integration

Image layout integration owns how artifacts become storage:

```text
partition table
boot partition
rootfs partition
data partition
recovery partition
U-Boot environment storage
raw NAND UBI volumes
```

Common tools and metadata:

- WIC in Yocto
- genimage in Buildroot workflows
- vendor flashing scripts
- UBI/UBIFS tools
- update-system bundle generators

Common checks:

```sh
fdisk -l image.wic
lsblk
mount -o loop,ro ...
```

## Practical Scenario: Add A New Userspace Service

Integration questions:

- where is the source built?
- what package owns the binary?
- where is the systemd unit installed?
- what configuration file does it need?
- should it start by default?
- does it need users, groups, devices, or firmware?
- is it included in the image?
- does it need an update migration path?

Good ownership:

```text
application recipe/package
  -> builds binary
  -> installs service file
  -> installs default config
  -> declares runtime dependencies

image/packagegroup
  -> includes package

product layer
  -> owns product-specific service policy
```

Avoid manually copying the binary into a rootfs after image generation.

## Practical Scenario: Add A Board Device Tree Change

Integration questions:

- is this a kernel DTB change, U-Boot DTB change, or both?
- which source DTS/DTSI owns the node?
- does the kernel driver exist and is it enabled?
- does the device require clocks, regulators, pinmux, GPIOs, or firmware?
- which DTB is actually deployed?
- does U-Boot load that DTB?

Debug path:

```text
source DTS changed
-> DTB rebuilt
-> DTB deployed to boot partition/image
-> U-Boot loads expected DTB
-> kernel boot log shows node/driver behavior
-> /proc/device-tree reflects expected property
```

## Practical Scenario: Kernel Module Missing On Target

Integration questions:

- was the driver configured as `m`?
- did the module build?
- was `modules_install` run into the rootfs staging directory?
- does the target rootfs contain the module under the matching kernel version?
- did `depmod` run?
- does the module depend on firmware?

Checks:

```sh
find rootfs/lib/modules -name '*.ko'
uname -r
modinfo my_driver.ko
```

Module and kernel version mismatches usually indicate artifact mixing or install-step mistakes.

## Practical Scenario: Board Boots Old Artifacts

Symptoms:

- U-Boot version string did not change
- kernel command line is old
- DTB behavior does not match source
- services are old despite a new rootfs build

Likely causes:

- flashing wrong storage device
- boot ROM selects another boot source
- U-Boot loads from a different partition
- boot partition was not updated
- board uses cached TFTP/NFS artifacts
- eMMC and SD both contain bootable images
- update bundle updated only one slot

Debug steps:

```sh
printenv
version
bdinfo
```

from U-Boot, plus Linux-side checks:

```sh
cat /proc/cmdline
uname -a
tr -d '\0' < /proc/device-tree/model
```

## Practical Scenario: TI SDK EVM To Product Board

Typical flow:

```text
choose nearest TI EVM
-> reproduce documented TI SDK image
-> record MACHINE and image target
-> create product layer
-> add product DTS/DTSI changes
-> add kernel config and patches
-> add U-Boot board/config changes
-> add application packages and services
-> define image layout and flashing flow
-> validate boot and peripherals
```

Key rule: get the unmodified EVM baseline building and booting first. Product-board migration is much easier when you can compare against a known-good vendor baseline.

## Ownership Matrix

Use this matrix when deciding where a change belongs:

```text
change                                likely owner
-----------------------------------   ----------------------------------
enable kernel driver                  kernel config fragment/defconfig
fix kernel driver bug                  kernel patch
add I2C peripheral                     board DTS/DTSI
change U-Boot boot command            U-Boot environment/config metadata
enable U-Boot command                  U-Boot defconfig/Kconfig
add application binary                 application recipe/package
start application at boot             service file + package/image policy
add firmware file                      firmware package/rootfs package
add rootfs package                     image recipe/packagegroup/config
change partition layout                WIC/genimage/vendor image metadata
sign boot artifacts                    secure boot/update metadata
change factory serial provisioning     productization/provisioning workflow
```

## Release Artifact Checklist

A BSP-integrated release should identify:

- bootloader first-stage artifacts
- U-Boot artifact and version
- kernel image and version
- DTBs and overlays
- root filesystem image
- kernel modules
- firmware files
- partitioned image
- update bundle if applicable
- SDK/toolchain if released
- source manifest
- patch manifest
- config manifest
- license/SBOM artifacts
- debug symbols
- checksums
- flashing instructions

## Review Questions

Before accepting a BSP integration change, ask:

- Which artifact changes?
- Which source or metadata owns the change?
- How is the artifact deployed?
- How will runtime prove the change is active?
- Does this affect factory image, OTA image, or both?
- Does this affect secure boot signing?
- Does this affect rollback compatibility?
- Does this need a migration for existing devices?
- Is the change product-specific, board-specific, vendor-specific, or upstreamable?
- How will the next BSP upgrade carry or remove this change?

## Common Mistakes

- Treating BSP output as one opaque image.
- Modifying generated files instead of source metadata.
- Mixing kernel, U-Boot, DTB, and rootfs artifacts from different builds.
- Putting product patches directly into vendor imports.
- Losing track of which defconfig generated the release.
- Debugging a runtime boot failure without checking the artifact provenance.
- Assuming a successful image build means the board booted the new artifacts.
- Copying files manually into the rootfs instead of packaging them.
- Updating kernel image but not matching modules.
- Updating DTB source but flashing an old DTB.
- Updating U-Boot but booting from another storage device.
- Treating EVM assumptions as product-board assumptions.
- Mixing prebuilt vendor artifacts with locally built artifacts without recording it.

## Debugging Checklist

- Identify the first artifact that fails.
- Confirm the board is booting the artifact just built.
- Compare boot logs with expected artifact versions.
- Check kernel image, DTB, modules, rootfs, and U-Boot all come from the same build.
- Check partition and bootloader load paths.
- Check whether patches are applied in the expected order.
- Check whether config fragments actually affect the final `.config`.
- Capture serial logs from reset.
- Record artifact checksums before and after flashing.
- Inspect bootloader environment and boot command.
- Confirm the active boot slot if A/B updates are used.
- Confirm `/proc/cmdline`, `uname -a`, loaded modules, and `/proc/device-tree`.
- Confirm rootfs package manifest contains expected packages.
- Confirm the deployed image layout matches the flashing instructions.

## Related Topics

- [Advanced Build Systems](index.md)
- [Artifact Flow and Provenance](bsp-integration/artifact-flow-and-provenance.md)
- [Configuration and Patch Ownership](bsp-integration/configuration-and-patch-ownership.md)
- [Boot Debugging and Runtime Validation](bsp-integration/boot-debugging-and-runtime-validation.md)
- [Image Layout and Deployment](bsp-integration/image-layout-and-deployment.md)
- [BSP Release Reproducibility](bsp-integration/release-reproducibility.md)
- [Linux Kernel Build System](linux-kernel/index.md)
- [U-Boot Build System](u-boot/index.md)
- [Yocto and OpenEmbedded](yocto-openembedded/index.md)
- [TI Processor SDK Linux](ti-processor-sdk/index.md)

## References

- Linux kernel documentation
- U-Boot documentation
- Yocto Project documentation
- TI Processor SDK Linux documentation
- Buildroot manual
- Filesystem Hierarchy Standard
