---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Yocto and OpenEmbedded Roadmap

## What Problem Does This Solve?

Yocto and OpenEmbedded are used to build complete embedded Linux distributions: bootloader, kernel, root filesystem, packages, SDKs, images, and update artifacts. They are more complex than Buildroot because they model software as metadata, layers, recipes, packages, tasks, machines, distros, and images.

This roadmap explains what to learn after basic cross-compilation, Make, CMake, kernel builds, and U-Boot builds.

## Core Concepts

- BitBake
- OpenEmbedded metadata
- Poky
- layers
- recipes: `.bb`
- append files: `.bbappend`
- classes: `.bbclass`
- configuration: `local.conf` and `bblayers.conf`
- machines
- distros
- images
- packages and package groups
- tasks
- sysroots
- SDK and extensible SDK
- kernel and U-Boot integration

## Mental Model

Think of Yocto/OE as a metadata-driven distribution factory:

```text
layers
-> recipes, classes, configuration, patches, machine metadata
-> BitBake parses metadata and schedules tasks
-> tasks fetch, configure, compile, install, package, and assemble images
-> deploy artifacts: images, packages, SDKs, bootloader, kernel, DTBs
```

The central skill is learning where a decision belongs:

- machine-specific hardware settings belong in machine configuration or BSP layers.
- distribution policy belongs in distro configuration.
- application build instructions belong in recipes.
- local developer overrides belong in `local.conf`.
- long-term product changes belong in a maintained product layer.

## Syntax / API / Mechanism

### Build Setup

Typical build trees contain:

```text
poky/
meta-openembedded/
meta-vendor/
meta-product/
build/
```

Important files:

```text
build/conf/local.conf
build/conf/bblayers.conf
```

Common commands:

```sh
source oe-init-build-env build
bitbake core-image-minimal
bitbake-layers show-layers
bitbake-layers show-recipes
```

### Recipes

A minimal recipe describes source, license, dependencies, and install behavior:

```bitbake
SUMMARY = "Small example application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://main.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} main.c -o hello
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 hello ${D}${bindir}/hello
}
```

### Tasks

Common task names:

- `do_fetch`
- `do_unpack`
- `do_patch`
- `do_configure`
- `do_compile`
- `do_install`
- `do_package`
- `do_rootfs`
- `do_image`

Useful commands:

```sh
bitbake -c cleanall hello
bitbake -c devshell hello
bitbake -e hello
bitbake -g core-image-minimal
```

### Layers

Layers are how Yocto/OE projects stay organized:

```text
meta-vendor/
meta-product/
meta-product/recipes-apps/
meta-product/recipes-kernel/
meta-product/recipes-bsp/
meta-product/conf/layer.conf
```

Use product layers for persistent changes. Avoid carrying production changes only in `local.conf`.

### Images

Images define what goes into the root filesystem:

```bitbake
require recipes-core/images/core-image-minimal.bb

IMAGE_INSTALL:append = " hello packagegroup-product"
```

Package groups collect sets of packages:

```bitbake
SUMMARY = "Product package group"
LICENSE = "MIT"

inherit packagegroup

RDEPENDS:${PN} = " \
    hello \
    openssh \
"
```

### Kernel and U-Boot

Yocto/OE can manage kernel and bootloader customization through recipes, append files, patches, fragments, and machine configuration.

Common areas:

- `recipes-kernel/linux/`
- `recipes-bsp/u-boot/`
- kernel config fragments
- device tree patches
- machine-specific boot files
- image and WIC layout

## Minimal Example

Add a layer, inspect it, and build an image:

```sh
source oe-init-build-env build
bitbake-layers add-layer ../meta-product
bitbake-layers show-layers
bitbake core-image-minimal
```

Then build one recipe:

```sh
bitbake hello
```

## Real-World Example

A practical Yocto/OE learning sequence:

1. Build `core-image-minimal` for a known machine.
2. Inspect `local.conf` and `bblayers.conf`.
3. Add `meta-openembedded`.
4. Create a product layer.
5. Add a simple C application recipe.
6. Add that application to an image.
7. Create a package group for product packages.
8. Add a systemd service recipe.
9. Patch an existing recipe with `.bbappend`.
10. Add a kernel config fragment.
11. Patch a device tree.
12. Build U-Boot through the BSP layer.
13. Generate an SDK for application developers.
14. Use `devtool` for recipe modification.
15. Add CI builds for image and SDK artifacts.

## Common Mistakes

- Treating `local.conf` as the product configuration source of truth.
- Editing files under `tmp/work` and losing the changes on rebuild.
- Adding packages to an image before confirming the recipe actually builds.
- Confusing a recipe name with a package name.
- Using `.bbappend` without matching the recipe name and version pattern.
- Forgetting that layer priority can change which recipe or append wins.
- Debugging a task without reading its task log.
- Changing machine or distro settings without rebuilding from a clean enough state.

## Debugging Checklist

- Use `bitbake-layers show-layers` to confirm active layers and priorities.
- Use `bitbake-layers show-recipes` to find recipe providers.
- Use `bitbake -e <recipe>` to inspect final variable values.
- Inspect task logs under `tmp/work/.../temp/`.
- Use `bitbake -c devshell <recipe>` for build-context debugging.
- Check whether the failure is in fetch, patch, configure, compile, install, package, rootfs, or image generation.
- Confirm the active `MACHINE` and distro.
- Confirm the package name being installed into the image.
- Rebuild the smallest failing recipe or task before rebuilding the full image.

## Learning Path

### Beginner

1. Yocto vs OpenEmbedded vs Poky
2. BitBake basics
3. Build directory structure
4. `local.conf` and `bblayers.conf`
5. Layers
6. Recipes
7. Images

### Intermediate

1. Task flow
2. Variable syntax and overrides
3. `.bbappend` files
4. Classes
5. Package names vs recipe names
6. Package groups
7. `devtool` and `recipetool`
8. SDK generation

### Advanced

1. Machine configuration
2. Distro configuration
3. Kernel recipes and fragments
4. U-Boot recipes
5. WIC image layouts
6. Reproducible builds and shared state
7. License and source compliance
8. CI integration
9. Product layer maintenance

## Related Topics

- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)
- [Linux Kernel Build System Roadmap](linux-kernel-build-roadmap.md)
- [U-Boot Build System Roadmap](u-boot-build-roadmap.md)
- [TI Processor SDK Roadmap](ti-processor-sdk-roadmap.md)
- [Embedded Linux](../embedded-linux/index.md)

## References

- Yocto Project documentation: <https://docs.yoctoproject.org/>
- Yocto Project Development Tasks Manual: <https://docs.yoctoproject.org/dev-manual/>
- Yocto Project Reference Manual: <https://docs.yoctoproject.org/ref-manual/>
- Yocto Project technical overview: <https://www.yoctoproject.org/development/technical-overview/>
