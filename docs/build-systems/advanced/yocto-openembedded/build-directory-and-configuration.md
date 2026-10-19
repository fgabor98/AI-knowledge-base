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

How a build directory is created depends on the release.

**scarthgap (5.0) and older**, and most vendor SDKs based on them, use the combined `poky` repository:

```sh
git clone -b scarthgap https://git.yoctoproject.org/poky
source poky/oe-init-build-env build
```

Sourcing the script changes into the build directory, puts BitBake on `PATH`, and creates `conf/local.conf` and `conf/bblayers.conf` from templates if they do not exist. Layers are then added with `bitbake-layers add-layer`, and `MACHINE`/`DISTRO` set in `local.conf`. The layer list and revisions live only in the developer's checkout unless the project adds a manifest tool (`repo`, `kas`, TI's `oe-layersetup`).

**wrynose (6.0)** has no `poky` repository. **`bitbake-setup`** reads a JSON configuration that pins every source repository, lists the layers, and selects configuration fragments, then creates the whole setup in one step. That makes the manifest a first-class, reviewable file.

Vendor SDKs can wrap either mechanism. Read the wrapper before assuming what it does.

## A Product Configuration File

The reference build for these pages is described by `examples/build-systems/yocto/kb-wrynose.conf.json`:

```json
--8<-- "examples/build-systems/yocto/kb-wrynose.conf.json"
```

- `sources` pins each repository to a branch **and** a commit (`rev`). Updating a layer is a reviewed one-line change to this file.
- `bb-layers` lists layers inside the fetched repositories; `bb-layers-file-relative` adds a layer by a path relative to the JSON file, so the product layer can live in the same repository as the configuration.
- `oe-fragments` selects `MACHINE` and `DISTRO` through built-in fragments instead of `local.conf` lines.

`bitbake-setup init` turns it into a setup directory. Every setup under the same top directory shares one downloads directory and one shared-state cache, configured in the top directory's `site.conf`:

```text
$ ~/yocto/.kb-bitbake-setup/bin/bitbake-setup --setting default top-dir-prefix ~ --setting default top-dir-name yocto init --non-interactive --no-init-vscode ~/AI-knowledge-base/examples/build-systems/yocto/kb-wrynose.conf.json am62xx-evm 2>&1 | grep -Ev '^ *[0-9]+%|^ *$'
NOTE: Bitbake-setup is using ~/yocto as top directory.
NOTE: A site.conf file already exists. Please remove it if you would like to replace it with a default one
NOTE: Reading configuration from local file
    ~/AI-knowledge-base/examples/build-systems/yocto/kb-wrynose.conf.json
NOTE: Run 'bitbake-setup init --non-interactive ~/AI-knowledge-base/examples/build-systems/yocto/kb-wrynose.conf.json am62xx-evm' to select this configuration non-interactively.
Initializing a setup directory in
    ~/yocto/kb-am62xx-wrynose
Fetching layer/tool repositories into ~/yocto/kb-am62xx-wrynose/layers
    bitbake
    openembedded-core
    meta-yocto
    meta-arm
    meta-ti
Setting up bitbake configuration in
    ~/yocto/kb-am62xx-wrynose/build
This bitbake configuration provides:
    Poky distro, TI AM62x EVM machine, example product layer
Usage instructions and additional information are in
     ~/yocto/kb-am62xx-wrynose/build/README
To run builds, source the environment using
    . ~/yocto/kb-am62xx-wrynose/build/init-build-env
Run 'bitbake-config-build enable-fragment <fragment-name>' to enable additional fragments or replace built-in ones (e.g. machine/<name> or distro/<name> to change MACHINE or DISTRO).
The bitbake configuration files (local.conf, bblayers.conf and more) can be found in
    ~/yocto/kb-am62xx-wrynose/build/conf
```

The two `--setting` options choose the top directory (`~/yocto`) for this one invocation. The repositories are cloned at the pinned commits, and the `NOTE` about `site.conf` shows that this top directory already had shared downloads and shared state from an earlier setup, which the new setup reuses.

## Build Directory Layout

What `bitbake-setup` generated (all transcripts on this page come from `examples/build-systems/yocto/run.sh`; paths are shown relative to `~/yocto`):

```text
$ ls ~/yocto/kb-am62xx-wrynose ~/yocto/kb-am62xx-wrynose/build/conf
~/yocto/kb-am62xx-wrynose:
build
config
layers

~/yocto/kb-am62xx-wrynose/build/conf:
bblayers.conf
conf-notes.txt
conf-summary.txt
local.conf
site.conf
templateconf.cfg
toolcfg.conf
```

| Path | Owner | Contents |
| --- | --- | --- |
| `layers/` | `bitbake-setup` | the pinned repository checkouts |
| `config/` | `bitbake-setup` | the configuration used, and `sources-fixed-revisions.json` (the exact commits checked out) |
| `build/conf/bblayers.conf` | `bitbake-setup` | active layers |
| `build/conf/toolcfg.conf` | tools (`bitbake-config-build`) | enabled fragments; do not edit by hand |
| `build/conf/site.conf` | `bitbake-setup` | shared `DL_DIR`, `SSTATE_DIR`, hash equivalence database |
| `build/conf/local.conf` | you | host-specific tweaks only |
| `build/tmp/`, `build/cache/` | BitBake | generated output; never edit |

Only `conf/` is configuration. Everything under `tmp/` is generated and disposable.

## `bblayers.conf` And Fragments

```text
$ grep -v '^#' conf/bblayers.conf conf/toolcfg.conf
conf/bblayers.conf:BBLAYERS ?= " \
conf/bblayers.conf:  ~/yocto/kb-am62xx-wrynose/layers/openembedded-core/meta \
conf/bblayers.conf:  ~/yocto/kb-am62xx-wrynose/layers/meta-yocto/meta-poky \
conf/bblayers.conf:  ~/yocto/kb-am62xx-wrynose/layers/meta-arm/meta-arm-toolchain \
conf/bblayers.conf:  ~/yocto/kb-am62xx-wrynose/layers/meta-arm/meta-arm \
conf/bblayers.conf:  ~/yocto/kb-am62xx-wrynose/layers/meta-ti/meta-ti-bsp \
conf/bblayers.conf:  ~/AI-knowledge-base/examples/build-systems/yocto/meta-kb-example \
conf/bblayers.conf:  "
conf/toolcfg.conf:
conf/toolcfg.conf:OE_FRAGMENTS += "distro/poky machine/am62xx-evm"
```

```text
$ . ~/yocto/kb-am62xx-wrynose/build/init-build-env >/dev/null && bitbake-layers show-layers 2>&1 | sed 's/  */ /g'
layer path priority
========================================================================================================
core ~/yocto/kb-am62xx-wrynose/layers/openembedded-core/meta 5
yocto ~/yocto/kb-am62xx-wrynose/layers/meta-yocto/meta-poky 5
arm-toolchain ~/yocto/kb-am62xx-wrynose/layers/meta-arm/meta-arm-toolchain 5
meta-arm ~/yocto/kb-am62xx-wrynose/layers/meta-arm/meta-arm 5
meta-ti-bsp ~/yocto/kb-am62xx-wrynose/layers/meta-ti/meta-ti-bsp 6
kb-example ~/AI-knowledge-base/examples/build-systems/yocto/meta-kb-example 10
```

The first column is the layer's collection name, used in `LAYERDEPENDS` and error messages; the last is `BBFILE_PRIORITY`.

A **configuration fragment** is a small named `.conf` file (`conf/fragments/<layer>/<name>.conf`) that a build can enable or disable without editing `local.conf`. Two are built in: `machine/<name>` sets `MACHINE`, `distro/<name>` sets `DISTRO`. Layers can ship their own:

```text
$ . ~/yocto/kb-am62xx-wrynose/build/init-build-env >/dev/null && bitbake-config-build list-fragments 2>&1 | grep -E 'machine/am62xx-evm|core/yocto/sstate-mirror-cdn|Available|Enabled' | head -n 8
Available built-in fragments:
Enabled built-in fragments:
machine/am62xx-evm	Sets MACHINE = "am62xx-evm"
Available fragments in core layer located in ~/yocto/kb-am62xx-wrynose/layers/openembedded-core/meta:
core/yocto/sstate-mirror-cdn	Use prebuilt sstate artifacts for standard Yocto build configurations.
```

`bitbake-getvar` shows that `MACHINE` came from the fragment, not from `local.conf`:

```text
$ . ~/yocto/kb-am62xx-wrynose/build/init-build-env >/dev/null && bitbake-getvar --quiet MACHINE | grep -v '^#$'
# $MACHINE [2 operations]
#   set ~/yocto/kb-am62xx-wrynose/build/conf/toolcfg.conf:6
#     "am62xx-evm (OE_FRAGMENTS contains "machine/am62xx-evm")"
#   set ~/yocto/kb-am62xx-wrynose/layers/openembedded-core/meta/conf/documentation.conf:272
#     [doc] "Specifies the target device for which the image is built. You define MACHINE in the conf/local.conf file in the Build Directory."
# pre-expansion value:
#   "am62xx-evm"
MACHINE="am62xx-evm"
```

Switch machines with `bitbake-config-build enable-fragment machine/<other>`; the tool replaces the previous `machine/` fragment. The old way, `MACHINE = "..."` in `local.conf`, still works on wrynose, but then two places can set it and `bitbake-getvar` is the only reliable way to know which one won.

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

Inspect with `bitbake-getvar --quiet MACHINE` (history included), as shown above. Changing `MACHINE` on TI K3 also changes the R5 machine of the `k3r5` multiconfig; see [Multiconfig and Firmware Builds](multiconfig-and-firmware-builds.md#real-example-ti-k3-k3r5).

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
