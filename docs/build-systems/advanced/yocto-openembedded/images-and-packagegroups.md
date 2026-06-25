---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Images and Package Groups

## What Problem Does This Solve?

Image metadata defines what software appears in the target root filesystem and which filesystem or disk-image artifacts are produced. Package groups organize product capabilities without turning one image recipe into an unreviewable package list.

## Core Concepts

- image recipe
- package group
- `IMAGE_INSTALL`
- `CORE_IMAGE_EXTRA_INSTALL`
- `IMAGE_FEATURES`
- `DISTRO_FEATURES`
- rootfs construction
- package manager
- image formats
- WIC
- package manifest

## Mental Model

```text
recipes produce packages
-> image recipe selects packages/features
-> package manager resolves runtime dependencies
-> do_rootfs creates target filesystem
-> image tasks create filesystem/disk artifacts
```

An image consumes packages, not arbitrary recipe build directories.

## Minimal Image Recipe

```bitbake
SUMMARY = "Minimal product image"
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL:append = " packagegroup-product-base"
```

Keep image recipes declarative. Complex install logic belongs in component recipes, classes, or image infrastructure.

## `IMAGE_INSTALL`

`IMAGE_INSTALL` names binary packages to install in the rootfs.

```bitbake
IMAGE_INSTALL:append = " example-app"
```

Spacing matters with append operations. Prefer clear product image/package-group ownership over ad hoc additions in `local.conf`.

## Recipe Name Vs Package Name

A recipe can generate multiple packages. Images install package names.

If recipe `example` emits:

```text
example
example-tools
example-dev
```

then installing `example-tools` requires that exact package name.

Inspect package output through recipe metadata and package data rather than guessing.

## Package Groups

Package-group recipe:

```bitbake
SUMMARY = "Base product packages"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    busybox \
    example-app \
    product-config \
"
```

Use package groups to model capabilities:

```text
packagegroup-product-base
packagegroup-product-networking
packagegroup-product-diagnostics
packagegroup-product-factory
```

Do not create a separate package group for every two-package detail. Use them where they express meaningful product composition.

## Image Features

`IMAGE_FEATURES` enables image-level capabilities implemented by image classes and metadata.

Examples can include:

- SSH server choices
- package-management support
- debug tools
- development packages
- read-only rootfs behavior

Available feature names depend on the active metadata.

Inspect final value:

```sh
bitbake -e <image> | grep '^IMAGE_FEATURES='
```

## Distro Features

`DISTRO_FEATURES` describes distribution capabilities that influence recipe configuration and dependency selection.

Examples:

- systemd
- x11/Wayland
- Bluetooth
- PAM
- security features

An image feature asks for image behavior. A distro feature declares broader distribution capability/policy. They are related but not interchangeable.

## Rootfs Construction

`do_rootfs` uses the configured package backend to:

- resolve package dependencies
- install selected packages
- run permitted post-install behavior
- generate package database/manifest data
- apply rootfs processing hooks

Rootfs failures often mean:

- requested package does not exist
- package architecture is incompatible
- package dependencies conflict
- post-install cannot complete
- package feed metadata is inconsistent

## Image Formats

Configured through `IMAGE_FSTYPES`, examples may include:

```bitbake
IMAGE_FSTYPES += "ext4 tar.zst wic"
```

Possible formats vary by metadata and host tools.

Choose formats based on deployment:

- raw filesystem for flashing a partition
- archive for NFS/container/test extraction
- WIC disk image for SD/eMMC media
- CPIO for initramfs
- squashfs for read-only systems

## WIC

WIC creates partitioned disk images from a kickstart-style description.

It can assemble:

- boot partition
- rootfs partition
- data/recovery partitions
- raw bootloader regions

WIC configuration belongs to BSP/product deployment ownership, not application recipes.

Validate:

- partition offsets
- bootloader raw regions
- filesystem types
- labels/UUIDs
- artifact source paths
- final image size

## Image Manifest

Yocto emits package manifests for images. Use them to answer what actually entered the rootfs.

Do not rely only on image recipe text because runtime dependencies add packages transitively.

## Adding A Package Safely

1. Build recipe:

   ```sh
   bitbake example-app
   ```

2. Verify package output and installed files.
3. Add package to product package group or image.
4. Build image.
5. Inspect image manifest.
6. Inspect/extract rootfs or boot target.
7. Verify runtime dependencies and service state.

## Debug And Production Images

Prefer explicit image variants or feature policy:

```text
product-image
product-image-debug
product-image-factory
```

Ensure debug images cannot be mistaken for production releases. Differences may include:

- SSH access
- debug tools
- package manager
- symbols
- test services
- factory provisioning tools

## Read-Only Root Filesystems

Read-only systems require more than a filesystem flag.

Audit:

- writable application state
- logs
- machine ID
- SSH host keys
- package post-install actions
- update mechanism
- overlay/data partitions

Image policy and runtime service design must agree.

## Rootfs Inspection

Find deploy artifacts:

```sh
find tmp/deploy/images/${MACHINE} -maxdepth 1 -type f
```

Find rootfs task workdir:

```sh
bitbake -e <image> | grep '^WORKDIR='
```

Inspect image manifest and, where practical, mount or extract the generated image.

## Worked Example: Base, Debug, And Factory Composition

```bitbake
# packagegroup-product-base.bb
RDEPENDS:${PN} = "product-agent iproute2"

# packagegroup-product-debug.bb
RDEPENDS:${PN} = "strace tcpdump gdbserver"

# product-image.bb
IMAGE_INSTALL:append = " packagegroup-product-base"

# product-image-debug.bb
require product-image.bb
IMAGE_INSTALL:append = " packagegroup-product-debug"
```

Factory provisioning should use a separately named image/package group so production policy does not accidentally inherit manufacturing tools.

## Worked Example: Explain A Transitive Package

If `libfoo` appears in the image but is not listed directly:

1. Find its package in image manifest.
2. Query package/runtime dependency data with release-supported tools.
3. Identify which selected package depends on it.
4. Change the owning feature/package only if the dependency is genuinely optional.

## Common Mistakes

- Adding recipe names when a differently named output package is required.
- Putting long product package lists in `local.conf`.
- Assuming successful recipe build means image inclusion.
- Using development image features in production.
- Treating WIC layout as unrelated to bootloader offsets.
- Ignoring transitive runtime packages.
- Modifying the generated rootfs instead of metadata.

## Debugging Checklist

- Does the recipe build?
- Which package contains the required file?
- Is that package requested by image/package group?
- Does the image manifest include it?
- Did runtime dependency resolution add/remove/conflict?
- Which `IMAGE_FEATURES` and `DISTRO_FEATURES` are active?
- Which image formats were generated?
- Does WIC use the expected boot artifacts and layout?
- Is the board flashing the current image?

## Related Topics

- [Recipes](recipes.md)
- [Machine and Distro Configuration](machine-and-distro-configuration.md)
- [Kernel and Bootloader Integration](kernel-and-bootloader-integration.md)
- [BSP Image Layout and Deployment](../bsp-integration/image-layout-and-deployment.md)

## References

- Yocto Project Development Tasks Manual
- Yocto Project Reference Manual
- WIC documentation
