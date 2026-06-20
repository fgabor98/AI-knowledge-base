---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Yocto and OpenEmbedded

## What Problem Does This Solve?

Yocto and OpenEmbedded build complete embedded Linux distributions from metadata. They manage source fetching, patching, cross-compilation, packaging, image generation, SDKs, kernel and bootloader integration, reproducibility, and license artifacts.

This section provides the detailed roadmap for understanding Yocto/OE as a build system, not only as a set of commands.

## Core Concepts

- BitBake
- recipes
- classes
- layers
- machines
- distros
- images
- package groups
- tasks
- work directories
- sysroots
- shared state
- deploy artifacts
- SDKs

## Mental Model

Yocto/OE build flow:

```text
layers and configuration
-> parsed metadata
-> task graph
-> fetch/patch/configure/compile/install/package
-> rootfs and image generation
-> deploy artifacts and SDKs
```

BitBake executes tasks based on metadata. The main skill is learning where metadata belongs and how to inspect the final expanded state.

## Learning Materials

1. [Yocto, OpenEmbedded, and BitBake Mental Model](mental-model.md)
2. [Build Directory and Configuration](build-directory-and-configuration.md)
3. [Layers](layers.md)
4. [Recipes](recipes.md)
5. [Tasks and Workdirs](tasks-and-workdirs.md)
6. [Images and Package Groups](images-and-packagegroups.md)
7. [Machine and Distro Configuration](machine-and-distro-configuration.md)
8. [Kernel and Bootloader Integration](kernel-and-bootloader-integration.md)
9. [SDK Generation](sdk-generation.md)
10. [Devtool and Recipe Development](devtool-and-recipe-development.md)
11. [Debugging BitBake Builds](debugging-bitbake-builds.md)
12. [Reproducibility, Caches, and Mirrors](reproducibility-caches-and-mirrors.md)
13. [BitBake Metadata, Overrides, and Python](bitbake-metadata-overrides-and-python.md)
14. [Packaging, QA, and Package Feeds](packaging-qa-and-feeds.md)
15. [WIC and Partition Layouts](wic-and-partition-layouts.md)
16. [Kernel Recipe Internals](kernel-recipe-internals.md)
17. [Multiconfig and Firmware Builds](multiconfig-and-firmware-builds.md)
18. [Licensing, CVE, and SBOM Workflows](licensing-cve-and-sbom.md)
19. [CI, Hash Equivalence, and Shared State](ci-hash-equivalence-and-sstate.md)
20. [End-to-End Product Layer Lab](end-to-end-product-layer-lab.md)

## Detailed Roadmap

### 1. Mental Model

Learn:

- Yocto Project vs OpenEmbedded vs Poky vs BitBake
- metadata-driven builds
- task graph execution
- source to package to image flow
- where generated output lives

Practice:

- build a minimal image
- inspect layers, recipes, tasks, workdirs, deploy output

### 2. Build Directory And Configuration

Learn:

- `oe-init-build-env`
- `local.conf`
- `bblayers.conf`
- `MACHINE`
- `DISTRO`
- `DL_DIR`
- `SSTATE_DIR`
- build directory hygiene

Practice:

- create a build directory
- add a layer
- switch machine deliberately
- inspect active configuration

### 3. Layers

Learn:

- `conf/layer.conf`
- layer priority
- layer dependencies
- BSP layers
- distro layers
- product layers
- `bitbake-layers`

Practice:

- create a product layer
- list active layers
- inspect recipe ownership and append application

### 4. Recipes

Learn:

- `.bb`
- `.bbappend`
- `SRC_URI`
- `LICENSE`
- `S`
- `do_compile`
- `do_install`
- package variables
- inheritance

Practice:

- write a simple application recipe
- add a patch
- install files into `${D}`
- package a systemd service

### 5. Tasks And Workdirs

Learn:

- task lifecycle
- `do_fetch`
- `do_unpack`
- `do_patch`
- `do_configure`
- `do_compile`
- `do_install`
- `do_package`
- `${WORKDIR}`
- `${S}`
- task logs

Practice:

- run one task
- inspect logs
- enter `devshell`
- clean and rebuild one recipe

### 6. Images And Packagegroups

Learn:

- image recipes
- `IMAGE_INSTALL`
- package groups
- rootfs generation
- package names vs recipe names
- image features

Practice:

- add a package to an image
- create a package group
- inspect rootfs contents

### 7. Machine And Distro Configuration

Learn:

- machine config
- distro config
- tune files
- kernel/provider selection
- bootloader/provider selection
- product policy vs hardware policy

Practice:

- inspect a machine config
- identify selected kernel and U-Boot providers
- separate product policy from board support

### 8. Kernel And Bootloader Integration

Learn:

- kernel recipes
- U-Boot recipes
- config fragments
- patches
- device tree changes
- deploy artifacts
- WIC/image integration

Practice:

- apply a kernel patch through a layer
- add a kernel config fragment
- patch a DTB
- inspect deployed kernel/U-Boot artifacts

### 9. SDK Generation

Learn:

- standard SDK
- extensible SDK
- toolchain environment
- target sysroot
- application development workflow

Practice:

- generate an SDK
- build an application outside BitBake
- verify SDK sysroot matches image contents

### 10. Devtool And Recipe Development

Learn:

- `devtool modify`
- `devtool add`
- `devtool finish`
- workspace layer
- patch extraction

Practice:

- modify an existing recipe
- turn a source edit into a layer patch
- cleanly finish the change into a product layer

### 11. Debugging BitBake Builds

Learn:

- `bitbake -e`
- task logs
- task signatures
- dependency graphs
- provider conflicts
- fetch failures
- patch failures
- rootfs failures

Practice:

- debug a failed task
- inspect final variable values
- identify why a package is or is not in an image

### 12. Reproducibility, Caches, And Mirrors

Learn:

- downloads cache
- sstate cache
- mirrors
- locked revisions
- source archiving
- license manifests
- build history

Practice:

- configure shared `DL_DIR`
- configure shared `SSTATE_DIR`
- produce release source and license artifacts

### 13. BitBake Metadata, Overrides, And Python

Learn:

- variable expansion timing and assignment operators
- overrides and package/machine scoping
- variable flags
- inline and task Python
- anonymous Python and metadata inspection

Practice:

- trace a variable through multiple layers
- implement machine-specific metadata without copying recipes
- add a deterministic Python task

### 14. Packaging, QA, And Package Feeds

Learn:

- package splitting
- `FILES`, `PACKAGES`, dependencies, and alternatives
- QA checks
- package backends and feeds

Practice:

- split an application into runtime/tools/config packages
- diagnose an installed-vs-shipped error
- publish and consume a development package feed

### 15. WIC And Partition Layouts

Learn:

- kickstart files
- source plugins
- raw bootloader regions
- partition sizing and alignment
- custom layouts and verification

Practice:

- create a boot/rootfs/data image
- inspect partition offsets
- verify boot artifacts inside a WIC image

### 16. Kernel Recipe Internals

Learn:

- kernel classes and tasks
- configuration metadata
- source/shared workdirs
- module packaging
- DTB and deploy flow

Practice:

- audit final kernel config
- trace one module into the image
- trace one DTB into deploy and WIC output

### 17. Multiconfig And Firmware Builds

Learn:

- multiple BitBake configurations
- cross-config dependencies
- auxiliary-core firmware
- artifact handoff

Practice:

- model a Linux image depending on a firmware build
- avoid copying firmware manually between build trees

### 18. Licensing, CVE, And SBOM Workflows

Learn:

- license checks and manifests
- source compliance output
- CVE scanning
- SPDX/SBOM generation
- policy exceptions

Practice:

- inspect image license output
- review a CVE report
- archive an SBOM with a release

### 19. CI, Hash Equivalence, And Shared State

Learn:

- clean CI builds
- shared downloads/sstate
- hash equivalence
- cache trust and publishing
- artifact promotion

Practice:

- design a two-stage build/validation pipeline
- publish manifests and checksums
- diagnose unexpected cache misses

### 20. End-To-End Product Layer Lab

Practice:

- create a product layer
- add an application and service
- create package groups and image
- add machine, kernel, U-Boot, and WIC metadata
- build, inspect, deploy, validate, and release the system

## Common Mistakes

- Editing files under `tmp/work` and losing changes.
- Putting product policy in `local.conf`.
- Confusing recipe names with package names.
- Adding a package to an image before proving the recipe builds.
- Using `.bbappend` patterns that do not match the target recipe.
- Ignoring layer priority and provider selection.
- Debugging without reading task logs.

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [TI Processor SDK Linux](../ti-processor-sdk/index.md)
- [Build Caching and Mirrors](../../build-caching-and-mirrors.md)

## References

- Yocto Project documentation
- Yocto Project Development Tasks Manual
- Yocto Project Reference Manual
- BitBake User Manual
