---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Layers

## What Problem Does This Solve?

Layers separate reusable core metadata, vendor BSP support, distribution policy, product features, and local customizations. A good layer architecture makes ownership clear and allows vendor or Yocto upgrades without copying entire metadata trees.

## Core Concepts

- metadata layer
- `conf/layer.conf`
- collection
- layer priority
- layer dependency
- compatibility declaration
- BSP layer
- distro layer
- product layer
- recipe ownership
- append matching

## Mental Model

```text
OE-Core
+ community layers
+ SoC vendor BSP layer
+ organization distro layer
+ product layer
-> one final parsed metadata universe
```

Layers are not executed in sequence. BitBake parses all active metadata and resolves the final result through priorities, overrides, providers, and append application.

## Typical Layer Layout

```text
meta-product/
  conf/
    layer.conf
    machine/
    distro/
  recipes-bsp/
  recipes-core/
  recipes-kernel/
  recipes-support/
  classes/
  wic/
```

The `meta-` prefix is conventional, not magical.

## `conf/layer.conf`

A layer declares its metadata paths and identity.

Conceptual example:

```bitbake
BBPATH .= ":${LAYERDIR}"

BBFILES += "${LAYERDIR}/recipes-*/*/*.bb \
            ${LAYERDIR}/recipes-*/*/*.bbappend"

BBFILE_COLLECTIONS += "product"
BBFILE_PATTERN_product = "^${LAYERDIR}/"
BBFILE_PRIORITY_product = "10"

LAYERDEPENDS_product = "core vendor-bsp"
LAYERSERIES_COMPAT_product = "<supported-release>"
```

Use the actual release series names supported by the layer.

## Collections

`BBFILE_COLLECTIONS` gives the layer a collection name used by priority, dependencies, and compatibility metadata.

Collection names should be stable and unique enough to avoid confusion.

## Layer Priority

Priority helps resolve recipe preference when multiple layers provide recipes with the same name/version context.

Do not use high priority as the primary product customization mechanism. Prefer:

- `.bbappend`
- explicit provider/version preferences
- machine/distro overrides
- clearly owned product recipes

Priority can hide unintended duplicates.

## Layer Dependencies

Declare required layers through `LAYERDEPENDS`.

Examples:

- product BSP metadata depends on a vendor BSP collection
- application metadata depends on `meta-python`
- graphical product metadata depends on a graphics layer

Undeclared dependencies make a layer appear reusable when it is not.

## Release Compatibility

`LAYERSERIES_COMPAT` states which Yocto/OE release series a layer supports.

This is a compatibility declaration, not proof. CI should still parse and build against each declared series.

## Layer Types

### Core Layer

Provides baseline toolchain, libraries, classes, packaging, and image infrastructure.

### BSP Layer

Owns hardware policy:

- machines
- kernel/U-Boot providers
- firmware
- device trees
- boot layouts
- SoC tuning

### Distro Layer

Owns operating-system policy:

- init system
- security defaults
- package formats
- provider/version policy
- release identity

### Product Layer

Owns product composition and product-specific changes:

- image recipes
- package groups
- application recipes
- kernel/U-Boot appends
- product services
- product update policy

## Creating A Layer

```sh
bitbake-layers create-layer ../meta-product
bitbake-layers add-layer ../meta-product
```

Then inspect:

```sh
bitbake-layers show-layers
```

Review generated `layer.conf` rather than treating it as final product architecture.

## Recipe Ownership

Find recipe providers:

```sh
bitbake-layers show-recipes <recipe>
```

This reveals:

- available recipe versions
- layers providing them
- selected/preferred version where shown

For virtual providers:

```sh
bitbake-layers show-recipes virtual/kernel
bitbake-layers show-recipes virtual/bootloader
```

## Appends

Show appends:

```sh
bitbake-layers show-appends
```

An append applies only when its filename matches an available recipe according to BitBake filename/version matching rules.

Common patterns:

```text
linux-vendor_%.bbappend
u-boot-vendor_2024.01.bbappend
```

Use wildcard matching carefully. A broad append can silently apply to a future version that has different assumptions.

## File Search Paths

Appends often add files through `FILESEXTRAPATHS`:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://product.cfg"
SRC_URI += "file://0001-product-change.patch"
```

Typical layout:

```text
recipes-kernel/linux/
  linux-vendor_%.bbappend
  linux-vendor/
    product.cfg
    0001-product-change.patch
```

## Layer Flattening

`bitbake-layers flatten` can produce a flattened view for inspection, but the result is not a maintainable replacement for layered metadata.

Use it to understand interactions, not to destroy ownership boundaries.

## Layer Architecture For A TI Product

Conceptual structure:

```text
OE-Core/Poky
meta-openembedded
TI vendor layers
meta-company-distro
meta-product
```

Ownership:

- TI layers: SoC/EVM enablement and SDK integration
- company distro layer: organization-wide OS policy
- product layer: custom board, image, applications, and product BSP deltas

Avoid editing TI layers directly. Carry product changes in your own layer.

## Upgrade Workflow

1. Record current layer revisions.
2. Update core/vendor layers to compatible branches.
3. Update `LAYERSERIES_COMPAT` only after testing.
4. Parse metadata.
5. Check unmatched appends.
6. Check provider/version selection.
7. Rebuild critical recipes.
8. Build and boot product image.
9. Review product patches for upstream/vendor integration.

## Common Mistakes

- Editing vendor layers directly.
- Creating one layer that mixes BSP, distro, and unrelated application ownership.
- Depending on a layer without declaring `LAYERDEPENDS`.
- Using priority to mask duplicate recipes.
- Using overly broad `%` appends.
- Ignoring unmatched appends during upgrades.
- Claiming release compatibility without CI validation.

## Debugging Checklist

- Is the layer in `BBLAYERS`?
- Does `bitbake-layers show-layers` list it?
- Does `BBFILES` match the recipe path?
- Is the collection name correct?
- Are dependencies declared and present?
- Does the append match the selected recipe?
- Which layer provides the selected recipe?
- Is priority affecting selection unexpectedly?
- Does the layer declare the current release series?

## Related Topics

- [Build Directory and Configuration](build-directory-and-configuration.md)
- [Recipes](recipes.md)
- [Machine and Distro Configuration](machine-and-distro-configuration.md)
- [Kernel and Bootloader Integration](kernel-and-bootloader-integration.md)

## References

- Yocto Project Layer Model documentation
- BitBake User Manual
- OpenEmbedded Layer Index
