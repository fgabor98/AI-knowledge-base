---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# TI Processor SDK Linux

## What Problem Does This Solve?

TI Processor SDK Linux is a vendor BSP and distribution build workflow for TI processors. It combines TI documentation, prebuilt images, SDK installers, Arago/OpenEmbedded layers, TI kernel and U-Boot trees, firmware, image targets, and EVM deployment workflows.

This section is the detailed roadmap for learning TI Processor SDK Linux as a build and product integration environment.

## Core Concepts

- TI Processor SDK Linux release
- EVM
- Arago layers
- `oe-layersetup`
- `oe-layertool-setup.sh`
- machine names
- image targets
- `tisdk-default-image`
- `tisdk-base-image`
- `deploy-ti`
- TI kernel
- TI U-Boot
- device tree customization
- RT Linux enablement
- EVM to product board migration

## Mental Model

TI Processor SDK flow:

```text
TI SDK release documentation
-> oe-layersetup config
-> Arago/OE layers at pinned revisions
-> MACHINE and image target
-> BitBake build
-> deploy-ti artifacts
-> SD/eMMC/flash deployment
-> EVM or product board validation
```

Treat the SDK release as the anchor. Documentation, layer revisions, machine names, image targets, kernel, U-Boot, firmware, and deployment instructions must match the selected SDK release.

## Roadmap Pages

Planned pages:

1. `sdk-overview-and-release-model.md`
2. `installed-sdk-layout.md`
3. `yocto-arago-layer-setup.md`
4. `machines-and-image-targets.md`
5. `building-tisdk-images.md`
6. `deploy-ti-and-artifacts.md`
7. `kernel-customization.md`
8. `u-boot-customization.md`
9. `device-tree-customization.md`
10. `adding-applications-and-services.md`
11. `rt-linux-builds.md`
12. `debugging-ti-sdk-builds.md`
13. `evm-to-product-board-workflow.md`

## Detailed Roadmap

### 1. SDK Overview And Release Model

Learn:

- SDK release naming
- processor family documentation
- supported EVMs
- prebuilt image vs source build
- release notes
- host requirements
- known issues

Practice:

- pick one processor family
- identify current SDK docs for that family
- boot a prebuilt image before modifying source builds

### 2. Installed SDK Layout

Learn:

- SDK installer output
- example applications
- filesystem images
- toolchains and sysroots
- documentation locations
- scripts and setup helpers

Practice:

- inspect installed SDK directories
- identify boot artifacts, rootfs, and toolchain pieces

### 3. Yocto Arago Layer Setup

Learn:

- Arago Project
- `oe-layersetup`
- processor SDK config files
- layer revisions
- `oe-layertool-setup.sh`
- build environment setup

Practice:

- clone `oe-layersetup`
- initialize layers using a documented config
- inspect active layers and revisions

### 4. Machines And Image Targets

Learn:

- TI machine names
- EVM vs custom board naming
- `tisdk-default-image`
- `tisdk-base-image`
- image feature differences
- RT build switches where supported

Practice:

- build the documented default image for one EVM
- compare image targets
- identify generated files

### 5. Building TISDK Images

Learn:

- build command structure
- `MACHINE`
- image targets
- `bitbake -k`
- build logs
- downloads and sstate

Practice:

- build one full image
- rebuild one recipe
- clean and rebuild a failed component

### 6. deploy-ti And Artifacts

Learn:

- `deploy-ti` layout
- kernel image
- DTBs
- U-Boot artifacts
- WIC/SD card images
- rootfs archives
- firmware
- manifests

Practice:

- map every deploy artifact to its purpose
- flash or write the documented image
- confirm runtime versions on the board

### 7. Kernel Customization

Learn:

- TI kernel recipe/provider
- kernel patches
- config fragments
- external modules where relevant
- kernel deploy artifacts

Practice:

- add a kernel config fragment
- apply a small patch through a layer
- confirm final `.config`

### 8. U-Boot Customization

Learn:

- TI U-Boot recipe/provider
- board defconfig
- environment policy
- SPL/U-Boot artifacts
- boot media assumptions

Practice:

- change a U-Boot config option
- rebuild U-Boot only
- confirm serial boot log shows the expected version

### 9. Device Tree Customization

Learn:

- TI kernel DTS layout
- board DTS/DTSI layering
- pinmux
- regulators/clocks
- overlays where used
- DTB deployment

Practice:

- patch a board DTS
- rebuild DTBs
- confirm deployed DTB matches runtime `/proc/device-tree`

### 10. Adding Applications And Services

Learn:

- product layer
- application recipe
- systemd service recipe
- package install into TI image
- SDK app development vs image-integrated app

Practice:

- add a simple app recipe
- add a service unit
- include package in the image
- verify service starts on EVM

### 11. RT Linux Builds

Learn:

- release-specific RT support
- documented RT enablement variables
- machine support constraints
- latency test artifacts

Practice:

- confirm selected machine supports RT
- build the documented RT image
- verify kernel version and preemption model on target

### 12. Debugging TI SDK Builds

Learn:

- layer mismatch failures
- fetch failures
- patch failures
- provider conflicts
- image/rootfs failures
- stale build dirs
- wrong machine names

Practice:

- use BitBake logs
- inspect final variables
- confirm layer revisions
- isolate a failed recipe before rebuilding the full image

### 13. EVM To Product Board Workflow

Learn:

- starting from the nearest EVM
- custom machine config
- custom DTS
- boot media changes
- product image content
- manufacturing and update implications

Practice:

- document EVM baseline
- list product board differences
- move changes into a product layer
- produce a reproducible product image

## Common Mistakes

- Mixing SDK documentation and layer revisions from different releases.
- Guessing `MACHINE` instead of using the documented one.
- Treating `deploy-ti` artifacts as interchangeable across builds.
- Editing generated output under `tmp/work`.
- Making product changes directly in vendor layers without a maintenance plan.
- Assuming RT support exists for every board.
- Debugging TI-specific failures as generic Yocto failures without checking TI metadata.

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [Yocto and OpenEmbedded](../yocto-openembedded/index.md)
- [Embedded Linux](../../../embedded-linux/index.md)

## References

- TI Processor SDK Linux documentation
- TI Arago Project documentation and repositories
- Yocto Project documentation
- TI E2E forum and release notes
