---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Build Directory and Configuration

## What Problem Does This Solve?

The Yocto build directory contains the active build configuration and all generated output for one build context. Understanding its ownership boundaries prevents product policy from becoming trapped in local developer files and prevents accidental reuse of incompatible output.

For embedded BSP work, the build directory determines the selected machine, distro, layers, caches, package formats, image features, providers, and deployment output.

## Core Concepts

- build environment initialization
- build directory
- `conf/local.conf`
- `conf/bblayers.conf`
- `MACHINE`
- `DISTRO`
- `TMPDIR`
- `DL_DIR`
- `SSTATE_DIR`
- build directory lifecycle
- product metadata ownership

## Mental Model

```text
checked-in layers and product configuration
+ local build-directory selection
-> active BitBake configuration
-> generated tmp output, caches, and deploy artifacts
```

The build directory is an execution workspace. It should not be the sole home of product knowledge.

## Initializing The Environment

A common Poky/OE workflow is:

```sh
source oe-init-build-env build
```

This normally:

- changes the current directory to the build directory
- places BitBake and related tools on `PATH`
- creates initial `conf/local.conf` and `conf/bblayers.conf` if absent
- exports environment values needed by the build tools

Vendor SDKs can wrap this command or provide their own setup scripts. Read the wrapper before assuming it only calls `oe-init-build-env`.

## Build Directory Layout

Typical layout:

```text
build/
  conf/
    local.conf
    bblayers.conf
  tmp/
  cache/
  downloads/        if DL_DIR points here
  sstate-cache/     if SSTATE_DIR points here
```

Only `conf/` is configuration. `tmp/`, parse caches, workdirs, and deploy output are generated.

## `bblayers.conf`

`bblayers.conf` identifies active metadata layers.

Conceptual example:

```bitbake
BBLAYERS = " \
  ${TOPDIR}/../poky/meta \
  ${TOPDIR}/../poky/meta-poky \
  ${TOPDIR}/../meta-openembedded/meta-oe \
  ${TOPDIR}/../meta-vendor \
  ${TOPDIR}/../meta-product \
"
```

It answers:

- which layers participate in parsing?
- where do recipes and appends come from?
- which product and BSP metadata is active?

Inspect:

```sh
bitbake-layers show-layers
```

## `local.conf`

`local.conf` contains build-instance settings.

Common values:

```bitbake
MACHINE = "example-machine"
DISTRO = "example-distro"
DL_DIR = "/srv/yocto/downloads"
SSTATE_DIR = "/srv/yocto/sstate-cache"
```

Appropriate local uses:

- developer parallelism
- local cache locations
- temporary diagnostics
- machine selection during development

Poor long-term uses:

- product package list
- kernel patch ownership
- production security policy
- permanent image features
- release provider selection

Product policy belongs in product layers, distro configuration, machine configuration, image recipes, or package groups.

## `MACHINE`

`MACHINE` selects the hardware configuration.

It can influence:

- CPU tune and ABI
- kernel provider and configuration
- U-Boot provider and configuration
- device trees
- machine-specific packages
- image formats
- WIC layout
- firmware
- deploy directory

Changing `MACHINE` is not a cosmetic switch. Use a separate build directory or deliberately audit all reused output.

Inspect:

```sh
bitbake -e | grep '^MACHINE='
bitbake -e virtual/kernel | grep '^MACHINE='
```

## `DISTRO`

`DISTRO` selects distribution policy.

It can control:

- init system
- package format
- compiler/security policy
- default features
- provider preferences
- versioning
- package feed behavior
- release identification

Hardware policy belongs primarily in machine/BSP metadata. Product operating-system policy belongs primarily in distro/image metadata.

## `TOPDIR` And `TMPDIR`

`TOPDIR` normally identifies the active build directory.

`TMPDIR` identifies the generated build-output root, commonly `${TOPDIR}/tmp`.

Under `TMPDIR` you commonly find:

```text
tmp/work/
tmp/work-shared/
tmp/sysroots-components/
tmp/deploy/
tmp/log/
tmp/stamps/
```

Do not commit or manually maintain files under `TMPDIR`.

## Downloads And Shared State

`DL_DIR` holds fetched sources.

`SSTATE_DIR` holds reusable task output.

For multiple builds, use shared locations:

```bitbake
DL_DIR = "/srv/yocto/downloads"
SSTATE_DIR = "/srv/yocto/sstate-cache"
```

Benefits:

- fewer network fetches
- faster clean build directories
- easier CI scaling

Risks:

- permissions
- cache growth
- untrusted cache sharing
- accidental cleanup

## Parallelism

Common controls:

```bitbake
BB_NUMBER_THREADS = "8"
PARALLEL_MAKE = "-j 8"
```

`BB_NUMBER_THREADS` limits BitBake task concurrency. `PARALLEL_MAKE` influences parallelism inside compatible compile tasks.

More parallelism can increase memory and I/O pressure. Tune based on the actual build host and CI contention.

## Disk Monitoring

Yocto builds consume significant disk space. Configure monitoring and keep caches/output on suitable filesystems.

Track:

- build `tmp`
- downloads
- sstate
- deploy artifacts
- package feeds
- build history

A disk-full failure can corrupt generated output or produce misleading task errors.

## Multiple Build Directories

Use separate directories for materially different contexts:

```text
build-am62x-debug/
build-am62x-release/
build-am64x-release/
build-ci/
```

This makes `MACHINE`, `DISTRO`, cache policy, and release intent explicit.

Caches can still be shared while `TMPDIR` remains isolated.

## Product Configuration Pattern

A maintainable product repository may contain:

```text
meta-product/
  conf/distro/product.conf
  conf/machine/product-board.conf
  recipes-core/images/product-image.bb
  recipes-core/packagegroups/packagegroup-product.bb
  recipes-kernel/linux/
  recipes-bsp/u-boot/
```

Then `local.conf` only selects the product configuration:

```bitbake
MACHINE = "product-board"
DISTRO = "product"
```

## Inspecting Active Configuration

```sh
bitbake-layers show-layers
bitbake -e | grep '^MACHINE='
bitbake -e | grep '^DISTRO='
bitbake -e | grep '^DL_DIR='
bitbake -e | grep '^SSTATE_DIR='
bitbake -e | grep '^TMPDIR='
```

For a recipe-specific value:

```sh
bitbake -e virtual/kernel | grep '^PREFERRED_PROVIDER_virtual/kernel='
```

## Switching Machine Safely

Recommended:

1. Create another build directory.
2. Reuse shared `DL_DIR` and `SSTATE_DIR` if appropriate.
3. Set the new `MACHINE`.
4. Verify active layers.
5. Verify kernel/U-Boot providers.
6. Build a minimal image.
7. Inspect the new machine deploy directory.

Do not copy deploy artifacts between machine directories manually.

## Worked Example: Reproducible Developer Build Directories

Shared site configuration:

```bitbake
DL_DIR = "/srv/yocto/downloads"
SSTATE_DIR = "/srv/yocto/sstate"
```

Create isolated contexts:

```sh
source oe-init-build-env build-product-debug
# Set MACHINE/DISTRO through the project's supported configuration mechanism.

source oe-init-build-env build-product-release
```

Both builds reuse sources and valid task output, but their `TMPDIR`, local configuration, and deploy trees remain isolated.

## Worked Example: Configuration Audit Script Inputs

Capture release context:

```sh
bitbake-layers show-layers > active-layers.txt
bitbake -e | grep -E '^(MACHINE|DISTRO|TMPDIR|DL_DIR|SSTATE_DIR)=' \
    > build-configuration.txt
```

Archive these files with the build manifest; do not use them as substitutes for pinned layer revisions.

## Common Mistakes

- Keeping product behavior only in `local.conf`.
- Committing absolute developer paths in shared configuration.
- Reusing one `TMPDIR` for unrelated release contexts.
- Confusing `DL_DIR` with `SSTATE_DIR`.
- Editing files under `tmp/work`.
- Switching `MACHINE` without checking providers and deploy output.
- Assuming vendor setup scripts do not change configuration.

## Debugging Checklist

- Was the correct environment script sourced?
- What is `TOPDIR`?
- Which layers are active?
- What are `MACHINE` and `DISTRO`?
- Where are `DL_DIR`, `SSTATE_DIR`, and `TMPDIR`?
- Is product policy checked into a layer?
- Is disk space sufficient?
- Are kernel and bootloader providers correct?
- Is the expected machine deploy directory being inspected?

## Related Topics

- [Mental Model](mental-model.md)
- [Layers](layers.md)
- [Machine and Distro Configuration](machine-and-distro-configuration.md)
- [Reproducibility, Caches, and Mirrors](reproducibility-caches-and-mirrors.md)

## References

- Yocto Project Reference Manual
- Yocto Project Development Tasks Manual
- BitBake User Manual
