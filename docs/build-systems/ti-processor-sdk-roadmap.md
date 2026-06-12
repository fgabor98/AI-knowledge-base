---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# TI Processor SDK Roadmap

## What Problem Does This Solve?

TI Processor SDK Linux is a vendor BSP and software platform for TI embedded processors. It provides prebuilt images, toolchains, documentation, board support, demos, and Yocto/OpenEmbedded-based source builds using TI and Arago layers.

This roadmap explains how to learn TI Processor SDK as a production embedded Linux workflow, not just as a prebuilt SD card image.

## Core Concepts

- TI Processor SDK Linux
- EVMs and board variants
- Arago Project
- Yocto/OpenEmbedded layers
- `oe-layersetup`
- `oe-layertool-setup.sh`
- SDK installer
- prebuilt images
- `tisdk-default-image`
- `tisdk-base-image`
- `deploy-ti`
- machine names
- RT kernel enablement
- TI kernel and U-Boot integration
- TI-specific demos, firmware, and libraries

## Mental Model

Think of TI Processor SDK as a vendor-supported Yocto/OE distribution plus board support:

```text
TI SDK release
-> documented host setup and supported machines
-> oe-layersetup config selects exact layer revisions
-> BitBake builds TI images and components
-> deploy-ti contains images, boot artifacts, packages, and SDK output
-> SD card or flashing workflow deploys to the EVM or product board
```

The release is the anchor. Always keep the documentation, layer configuration, machine name, kernel, U-Boot, firmware, and image target from the same SDK release unless you are deliberately porting.

## Syntax / API / Mechanism

### SDK Build Flow

TI Processor SDK Linux builds are based on Arago layers for OpenEmbedded and Yocto Project targeting TI platforms.

Typical flow:

```sh
git clone https://git.ti.com/git/arago-project/oe-layersetup.git tisdk
cd tisdk
./oe-layertool-setup.sh -f configs/processor-sdk/<oeconfig-file>
cd build
. conf/setenv
MACHINE=<machine> bitbake -k tisdk-default-image
```

The exact `<oeconfig-file>`, `<machine>`, and image targets are release-specific. Use the matching TI documentation for the selected processor family and SDK version.

### Layer Configuration

TI SDK releases use `oe-layersetup` configuration files to initialize the Yocto/OE build environment.

Important habit:

- identify the processor family
- identify the SDK version
- choose the matching layer configuration file
- use the documented machine name
- avoid mixing layers across unrelated releases

### Image Targets

Common image target names include:

- `tisdk-default-image`
- `tisdk-base-image`
- other release-specific TI images

Outputs are commonly placed relative to `deploy-ti`.

### Machine Names

Machine names are board and release specific. Examples in TI documentation include names such as:

```text
am62xx-evm
am64xx-evm
```

Do not guess the machine name. Read the release documentation for the exact supported machine list.

### RT Kernel Builds

Some Processor SDK releases support RT kernel builds for selected machines. TI documentation shows this as a build-time setting such as:

```sh
MACHINE=<machine> ARAGO_RT_ENABLE=1 bitbake <target>
```

Check the release documentation before assuming RT support exists for a board.

## Minimal Example

Build the default TI SDK image for a documented machine:

```sh
git clone https://git.ti.com/git/arago-project/oe-layersetup.git tisdk
cd tisdk
./oe-layertool-setup.sh -f configs/processor-sdk/<oeconfig-file>
cd build
. conf/setenv
MACHINE=<machine> bitbake -k tisdk-default-image
```

Then inspect generated artifacts:

```sh
find deploy-ti -maxdepth 3 -type f
```

## Real-World Example

A practical TI Processor SDK learning sequence:

1. Select one TI board family, such as AM62x or AM64x.
2. Download and boot the prebuilt SDK image first.
3. Create an SD card using the documented workflow.
4. Bring up serial console and verify boot logs.
5. Read the SDK release overview and directory structure.
6. Build the SDK image from source with the documented layer config.
7. Locate the generated root filesystem, kernel, U-Boot, DTB, and WIC artifacts.
8. Build a single recipe from the TI SDK build tree.
9. Add a simple application recipe in a custom layer.
10. Add the application to a TI image.
11. Patch the TI kernel or device tree with a `.bbappend`.
12. Patch U-Boot or adjust boot configuration.
13. Build an RT image if the selected SDK release and machine support it.
14. Recreate a release image in CI.
15. Document exactly which SDK release, layer config, machine, and image target produced the product build.

## Common Mistakes

- Mixing documentation from one SDK release with layers from another.
- Guessing the machine name instead of using the supported machine table.
- Treating prebuilt SDK contents and Yocto-built artifacts as interchangeable without checking versions.
- Editing generated build output instead of changing recipes, appends, or layer metadata.
- Rebuilding the full image before isolating a failing recipe.
- Ignoring TI-specific firmware, boot image, or flash layout requirements.
- Assuming RT kernel support applies to every board.
- Losing track of whether an issue is upstream Yocto/OE, Arago/TI metadata, kernel, U-Boot, firmware, or board hardware.

## Debugging Checklist

- Confirm processor family and SDK version.
- Confirm the exact TI documentation page matches that SDK version.
- Confirm the selected `oe-layersetup` config file.
- Confirm `MACHINE`.
- Confirm the image target, such as `tisdk-default-image`.
- Use `bitbake-layers show-layers` to inspect active layers.
- Use `bitbake -e <recipe>` to inspect final variables.
- Inspect task logs under the recipe work directory.
- Check `deploy-ti` for generated artifacts.
- Compare boot artifacts on the SD card or flash media with the artifacts from the current build.
- For RT builds, confirm the release and machine support RT before debugging build flags.

## Learning Path

### Beginner

1. TI Processor SDK purpose and supported processor families
2. SDK release documentation
3. EVM boot flow
4. SD card creation
5. Serial console boot validation
6. SDK directory structure
7. Prebuilt image vs source-built image

### Intermediate

1. Arago and TI Yocto/OE layers
2. `oe-layersetup`
3. Layer configuration files
4. Machine names
5. TI image targets
6. `deploy-ti`
7. Building individual recipes
8. Adding a custom application layer

### Advanced

1. Kernel customization
2. Device tree customization
3. U-Boot customization
4. Firmware and boot artifact handling
5. RT kernel builds where supported
6. Reproducible SDK release builds
7. CI build containers or pinned host setup
8. Product board migration from an EVM
9. Long-term maintenance across SDK releases

## Related Topics

- [Yocto and OpenEmbedded Roadmap](yocto-openembedded-roadmap.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)
- [Linux Kernel Build System Roadmap](linux-kernel-build-roadmap.md)
- [U-Boot Build System Roadmap](u-boot-build-roadmap.md)
- [Embedded Linux](../embedded-linux/index.md)

## References

- TI Processor SDK Linux documentation: <https://software-dl.ti.com/processor-sdk-linux/>
- TI Processor SDK Linux AM62x documentation: <https://software-dl.ti.com/processor-sdk-linux/esd/AM62X/latest/exports/docs/>
- TI Processor SDK Linux AM64x documentation: <https://software-dl.ti.com/processor-sdk-linux/esd/AM64X/>
- Arago Project oe-layersetup: <https://git.ti.com/cgit/arago-project/oe-layersetup/>
