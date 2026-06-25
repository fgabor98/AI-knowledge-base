---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Configuration and Patch Ownership

## What Problem Does This Solve?

BSPs accumulate configuration and patches over time. Without clear ownership, product changes end up mixed with vendor changes, generated files are edited by hand, and upgrades become risky.

This topic explains where configuration and patches belong across kernel, U-Boot, device tree, rootfs, image generation, Yocto, Buildroot, and TI Processor SDK workflows.

## Core Concepts

- source of truth
- generated file
- product layer
- vendor layer
- board configuration
- product policy
- patch series
- config fragment
- defconfig
- device tree source
- rootfs overlay
- image layout metadata

## Mental Model

Every change should answer:

```text
What behavior changes?
Which artifact changes?
Which source/config owns that artifact?
Is this board-specific, product-specific, vendor-specific, or upstreamable?
How will this survive a BSP upgrade?
```

Bad BSP integration hides answers. Good BSP integration makes them obvious.

## Ownership Categories

### Upstream Fix

Belongs as close to upstream as possible.

Examples:

- kernel driver bug fix
- U-Boot generic driver fix
- build-system compatibility fix

Preferred handling:

- submit upstream where realistic
- carry as a named patch while waiting
- remove when upstream/vendor release includes it

### Vendor BSP Patch

Belongs in vendor BSP metadata or a clearly separated vendor import.

Examples:

- TI kernel patch for SoC support
- vendor U-Boot patch for boot flow
- vendor firmware integration

Preferred handling:

- do not rewrite vendor history casually
- keep product changes separate
- record vendor release baseline

### Board Enablement Patch

Belongs in board-support metadata.

Examples:

- product board DTS
- pinmux changes
- regulator definitions
- U-Boot board config
- storage layout for a board

Preferred handling:

- keep near machine/board metadata
- document hardware revision assumptions
- validate against schematic and boot logs

### Product Policy Patch

Belongs in the product layer or product configuration.

Examples:

- which services start by default
- image package selection
- logging policy
- default application config
- update client configuration

Preferred handling:

- keep out of vendor layers
- make product ownership explicit
- review as product behavior, not BSP plumbing

### Temporary Workaround

Must be labeled clearly.

Metadata to preserve:

```text
reason:
owner:
date added:
remove when:
upstream/vendor link:
affected hardware:
```

Temporary patches without removal conditions become permanent technical debt.

## Configuration Ownership By Artifact

### Kernel Configuration

Common sources:

```text
arch/<arch>/configs/*_defconfig
Yocto config fragments
Buildroot kernel config
vendor SDK kernel metadata
```

Do not treat final `.config` as the long-term source of truth unless the workflow explicitly does.

Good workflow:

```text
desired option
-> config fragment or defconfig source
-> build final .config
-> verify CONFIG_* value
```

Checks:

```sh
grep CONFIG_FOO build/.config
```

### U-Boot Configuration

Common sources:

```text
configs/<board>_defconfig
Kconfig symbols
default environment source
board header/config files in older flows
Yocto/Buildroot U-Boot metadata
```

Good workflow:

```text
change defconfig/Kconfig source
-> rebuild U-Boot
-> verify generated .config
-> verify serial boot log version
```

### Device Tree Configuration

Common sources:

```text
SoC .dtsi
board .dts
carrier-board .dtsi
overlay .dtso
U-Boot DTS copy where separate
```

Good workflow:

```text
edit DTS/DTSI source
-> rebuild DTB
-> deploy DTB
-> verify /proc/device-tree
```

Do not edit decompiled DTBs as the source of truth.

### Rootfs Configuration

Common sources:

```text
package recipes
rootfs overlays
systemd unit files
init scripts
image recipes
package groups
Buildroot package selections
```

Good workflow:

```text
package owns file
image owns package selection
product layer owns product policy
```

Avoid manually copying files into a completed rootfs image.

### Image Layout Configuration

Common sources:

```text
WIC .wks files
genimage config
vendor flashing scripts
partition layout documentation
update system slot metadata
```

Good workflow:

```text
storage layout metadata
-> generated image
-> inspect partition table
-> flash using documented method
```

## Patch Workflow

### Patch Naming

Use descriptive names:

```text
0001-arm64-dts-add-product-board-uart2.patch
0002-net-cpsw-workaround-link-reset-on-revA.patch
0003-u-boot-set-default-bootcmd-for-emmc.patch
```

Avoid:

```text
fix.patch
changes.patch
new.patch
```

### Patch Header Content

Good patches explain why, not only what:

```text
Subject: arm64: dts: add product board UART2 pinmux

The revB product board routes UART2 to the debug connector.
Enable the pinmux and node so the factory test image can use it.

Applies to hardware: product revB and later.
```

### Patch Layering

Preferred order:

```text
upstream baseline
-> vendor BSP patches
-> board enablement patches
-> product policy patches
-> temporary workarounds
```

In Yocto, this usually maps to layer ordering and `.bbappend` ownership.

## Build-System Examples

### Yocto / OpenEmbedded

Kernel patch:

```text
meta-product/
  recipes-kernel/linux/linux-ti_%.bbappend
  recipes-kernel/linux/files/0001-arm64-dts-product-board.patch
```

Recipe snippet:

```bitbake
SRC_URI += "file://0001-arm64-dts-product-board.patch"
```

Application policy:

```text
meta-product/
  recipes-core/images/product-image.bb
  recipes-core/packagegroups/packagegroup-product.bb
```

### Buildroot

Keep customizations in `BR2_EXTERNAL`:

```text
br2-product/
  configs/product_defconfig
  board/product/
  package/product-app/
```

Rootfs overlay:

```text
board/product/rootfs-overlay/etc/product.conf
```

Use overlays sparingly for static files. Prefer packages for built software.

### TI Processor SDK Linux

Keep TI release metadata separate from product metadata:

```text
tisdk/
  meta-ti/
  meta-arago/
  meta-product/
```

Product changes should normally live in `meta-product`, not directly in `meta-ti` or `meta-arago`, unless you intentionally maintain a fork.

## Common Scenarios

### Kernel Option Does Not Appear In Final `.config`

Likely causes:

- dependency not met
- fragment not applied
- wrong kernel provider
- wrong machine
- option selected differently by another fragment

Debug:

```sh
grep CONFIG_FOO .config
```

In Yocto:

```sh
bitbake -e virtual/kernel | grep -E 'MACHINE|PREFERRED_PROVIDER|SRC_URI'
```

### Patch Applies Locally But Not In CI

Likely causes:

- CI uses different source revision
- patch order differs
- layer version differs
- patch context is too weak
- generated patch was not added to metadata

Debug:

- print source revision
- inspect patch task log
- verify layer revisions
- rebuild from clean fetch/unpack/patch tasks

### Device Tree Change Is In Wrong Tree

Some platforms have both kernel and U-Boot device tree sources. A Linux driver change usually needs the kernel DTB. A U-Boot pre-relocation driver may need U-Boot's DTB.

Debug:

- identify which DTB is deployed
- identify which component consumes it
- decompile deployed DTB
- compare with source DTS

## Common Mistakes

- Editing generated `.config` and losing changes.
- Editing vendor layers for product policy.
- Combining unrelated changes in one patch.
- Carrying workaround patches with no removal plan.
- Putting application files into rootfs overlays instead of packages.
- Assuming a config fragment was applied without checking final config.
- Forgetting hardware revision constraints in board patches.
- Treating U-Boot and kernel DTBs as always identical.

## Debugging Checklist

- Identify source of truth for the changed artifact.
- Check final generated config.
- Check patch application logs.
- Check layer or package ownership.
- Check machine and provider selection.
- Check deployed artifact, not only build output.
- Check hardware revision assumptions.
- Record patch reason, owner, and removal condition.

## Related Topics

- [BSP Build Integration](../bsp-build-integration.md)
- [Artifact Flow and Provenance](artifact-flow-and-provenance.md)
- [Boot Debugging and Runtime Validation](boot-debugging-and-runtime-validation.md)
- [Release Reproducibility](release-reproducibility.md)

## References

- Linux kernel Kconfig documentation
- U-Boot documentation
- Yocto Project Development Tasks Manual
- Buildroot manual
- TI Processor SDK Linux documentation
