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

## Roadmap Pages

Planned pages:

1. `mental-model.md`
2. `build-directory-and-configuration.md`
3. `layers.md`
4. `recipes.md`
5. `tasks-and-workdirs.md`
6. `images-and-packagegroups.md`
7. `machine-and-distro-configuration.md`
8. `kernel-and-bootloader-integration.md`
9. `sdk-generation.md`
10. `devtool-and-recipe-development.md`
11. `debugging-bitbake-builds.md`
12. `reproducibility-caches-and-mirrors.md`

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
