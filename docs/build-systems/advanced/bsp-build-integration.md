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

### 2. Configuration Ownership

Learn where each kind of configuration belongs:

- kernel config: defconfig, fragments, recipe metadata
- U-Boot config: board defconfig, Kconfig, environment policy
- device tree: kernel tree, U-Boot tree, vendor overlays, product overlays
- rootfs content: image recipe, package groups, Buildroot config, package manifests
- services: recipes/packages, systemd units, init scripts
- image layout: WIC, genimage, vendor image tools, partition scripts

### 3. Patch Ownership

Separate patch classes:

- upstream fix
- vendor BSP patch
- board enablement patch
- product policy patch
- temporary workaround
- release-only patch

Every patch should have a reason, owner, expected lifetime, and upstream/vendor status.

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

## Common Mistakes

- Treating BSP output as one opaque image.
- Modifying generated files instead of source metadata.
- Mixing kernel, U-Boot, DTB, and rootfs artifacts from different builds.
- Putting product patches directly into vendor imports.
- Losing track of which defconfig generated the release.
- Debugging a runtime boot failure without checking the artifact provenance.

## Debugging Checklist

- Identify the first artifact that fails.
- Confirm the board is booting the artifact just built.
- Compare boot logs with expected artifact versions.
- Check kernel image, DTB, modules, rootfs, and U-Boot all come from the same build.
- Check partition and bootloader load paths.
- Check whether patches are applied in the expected order.
- Check whether config fragments actually affect the final `.config`.

## Related Topics

- [Advanced Build Systems](index.md)
- [Linux Kernel Build System](linux-kernel/index.md)
- [U-Boot Build System](u-boot/index.md)
- [Yocto and OpenEmbedded](yocto-openembedded/index.md)
- [TI Processor SDK Linux](ti-processor-sdk/index.md)

## References

- Linux kernel documentation
- U-Boot documentation
- Yocto Project documentation
- TI Processor SDK Linux documentation
