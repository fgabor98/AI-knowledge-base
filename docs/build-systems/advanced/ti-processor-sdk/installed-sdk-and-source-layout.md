---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Installed SDK and Source Layout

## Goal

Learn how to inspect a Processor SDK installation and a source build tree without confusing prebuilt artifacts, build metadata, generated output, and product-owned changes.

## Common Layout Categories

A Processor SDK environment usually contains several categories of files:

- documentation and release notes
- prebuilt boot artifacts
- prebuilt root filesystems or SD-card images
- toolchains and sysroots
- example applications
- setup scripts
- Yocto/OpenEmbedded layers
- BitBake build directories
- generated deploy artifacts

The exact directory names vary by SDK and processor family. Learn the role of each directory instead of memorizing only one release layout.

## Installed SDK Vs Yocto Build Directory

An installed SDK directory and a Yocto build directory are different things.

The installed SDK may contain:

- prebuilt binaries
- scripts for setting environment variables
- example code
- host-side tools
- target sysroots
- documentation snapshots

The Yocto build directory contains:

- `conf/local.conf`
- `conf/bblayers.conf`
- `tmp/`
- `downloads/` or a configured `DL_DIR`
- `sstate-cache/` or a configured `SSTATE_DIR`
- generated task logs
- generated workdirs
- deploy output

Do not patch generated files under `tmp/work`. Fix the recipe, append, source patch, config fragment, or product layer that produced them.

## Source Build Layout

A typical source-based setup looks like:

```text
sdk-work/
  oe-layersetup/
  sources/
    bitbake/
    oe-core/
    meta-openembedded/
    meta-ti/
    meta-arago/
    ...
  build/
    conf/
    tmp/
    downloads/
    sstate-cache/
```

Some releases or local setups use different names, but the same ownership model applies:

- setup repository describes what to clone
- `sources/` contains metadata layers and source checkouts
- `build/conf/` contains local build configuration
- `build/tmp/` contains generated output
- `tmp/deploy/` contains deployable artifacts

## What To Inspect First

After setup, inspect:

```bash
bitbake-layers show-layers
bitbake-layers show-recipes virtual/kernel
bitbake-layers show-recipes virtual/bootloader
bitbake -e <image> | grep '^MACHINE='
bitbake -e <image> | grep '^DISTRO='
```

These commands tell you whether your build directory matches your mental model.

## Artifact Locations

Important generated artifacts usually live under:

```text
tmp/deploy/images/<machine>/
tmp/deploy/licenses/
tmp/deploy/sdk/
tmp/deploy/rpm/
tmp/deploy/ipk/
tmp/deploy/deb/
```

For TI SDK work, always inspect the machine-specific deploy directory. It is where bootloader binaries, kernel images, DTBs, rootfs archives, WIC images, firmware, and manifests usually appear.

## Layout Inspection Checklist

For a new SDK tree, record:

- path to SDK docs
- path to `oe-layersetup`
- path to sources
- active build directory
- active `MACHINE`
- active `DISTRO`
- layer list and priorities
- deploy directory
- image target being built
- kernel and U-Boot recipe providers

This record is useful during support, CI migration, and SDK upgrades.

## Common Mistakes

- Treating the installed SDK sysroot as the same thing as a Yocto build sysroot.
- Copying files out of `tmp/work` and calling them source changes.
- Losing track of which build directory produced a deploy artifact.
- Flashing a prebuilt artifact while debugging a source-built artifact.
- Comparing two images without comparing their manifests and bootloader versions.

## Related Topics

- [TI Yocto and Arago Build Flow](ti-yocto-arago-build-flow.md)
- [Boot Artifact Pipeline](boot-artifact-pipeline.md)
- [Debugging TI SDK Builds and Boots](debugging-ti-sdk-builds-and-boots.md)
