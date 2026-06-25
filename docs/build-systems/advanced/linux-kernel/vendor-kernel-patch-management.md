---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Vendor Kernel Patch Management

## What Problem Does This Solve?

Embedded Linux products often start from a vendor kernel rather than a clean upstream kernel. For TI Sitara work, that may mean a TI kernel branch delivered through Processor SDK Linux or Yocto metadata.

Patch management is the discipline that keeps your product changes understandable, reviewable, upgradeable, and separable from vendor code.

## Core Concepts

- upstream kernel
- stable kernel
- vendor kernel
- SDK kernel
- product kernel
- patch stack
- downstream patch
- board patch
- driver patch
- metadata-owned change
- rebase
- forward-port

## Mental Model

Think in layers:

```text
upstream Linux
-> stable branch
-> SoC vendor kernel
-> SDK integration
-> board/product patches
-> local debug patches
```

Your job is to make the top layers small, intentional, and easy to replay onto a new vendor release.

## Why Vendor Kernels Exist

Vendor kernels often include:

- SoC enablement not yet upstream
- boot-critical driver support
- power-management changes
- multimedia or accelerator support
- board DTS files
- vendor defconfigs
- SDK integration patches
- backports from newer kernels

For production, using the vendor kernel can be pragmatic. The danger is letting product-specific changes dissolve into the vendor tree without ownership.

## Patch Classification

Classify every kernel change before carrying it.

| Patch Type | Example | Preferred Home |
| --- | --- | --- |
| board DTS | custom carrier board pinmux | kernel tree or BSP layer patch |
| config | enable product filesystem | config fragment |
| driver fix | fix probe sequence | kernel patch |
| driver feature | add hardware support | kernel patch, upstream candidate |
| debug | extra logs | temporary branch or debug patch |
| packaging | install module package | Yocto metadata |
| boot artifact selection | choose DTB filename | bootloader script or image metadata |

Many changes do not belong in kernel source. A config setting belongs in a fragment. A package inclusion belongs in image metadata. A boot filename belongs in deployment or bootloader configuration.

## Good Patch Stack Shape

A maintainable stack has small, named patches:

```text
0001-arm64-dts-ti-add-custom-carrier-board.patch
0002-net-ti-fix-reset-delay-for-product-phy.patch
0003-spi-add-board-specific-chip-select-quirk.patch
```

Each patch should answer:

- what changed
- why it changed
- what hardware or product it affects
- whether it is temporary
- whether it should be upstreamed

Avoid one large patch named:

```text
product_kernel_changes.patch
```

That becomes nearly impossible to rebase safely.

## DTS Patch Discipline

DTS changes should be isolated from driver changes when possible.

Good:

```text
0001-arm64-dts-ti-add-product-board.patch
0002-arm64-dts-ti-enable-product-can-interface.patch
```

Risky:

```text
0001-product-board-and-driver-fixes-and-config.patch
```

DTS patches should clearly identify:

- board file changed
- included DTSI files affected
- peripheral enabled
- pinctrl state
- clocks and regulators
- interrupt wiring
- runtime validation performed

## Driver Patch Discipline

Driver patches should separate:

- bug fixes
- hardware support
- temporary diagnostics
- product-specific policy
- upstreamable cleanup

Product policy often does not belong in a generic driver. If a behavior is board-specific, prefer device tree data, module parameters, or product configuration where appropriate.

## Config Patch Discipline

Do not patch vendor defconfig directly unless that is the established project policy.

Prefer fragments:

```text
board.cfg
product.cfg
debug.cfg
production.cfg
```

This makes it clear which configuration is vendor baseline and which configuration is product intent.

## Yocto Patch Flow

In Yocto, kernel patches are usually carried in a kernel recipe append:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-arm64-dts-ti-add-product-board.patch"
SRC_URI += "file://0002-net-ti-fix-product-phy-reset.patch"
SRC_URI += "file://product.cfg"
```

Important checks:

```sh
bitbake -e virtual/kernel | grep '^SRC_URI='
bitbake virtual/kernel -c patch
bitbake virtual/kernel -c devshell
```

Use the work directory to inspect the exact patched source, but keep the maintained patches in your layer.

## TI Processor SDK Upgrade Flow

When moving from one TI SDK release to another:

1. Record current SDK release, kernel commit, machine, and provider.
2. Export or identify the current product patch stack.
3. Import the new SDK baseline.
4. Apply product patches one by one.
5. Resolve conflicts by patch category.
6. Rebuild kernel image, DTBs, and modules.
7. Audit final config.
8. Validate boot and runtime drivers.
9. Update release artifact manifest.

Do not start by copying the old patched kernel tree over the new SDK tree. That discards the value of the SDK update.

## Rebase Strategy

Rebase order:

1. DTS board additions
2. DTS board modifications
3. driver bug fixes
4. driver feature patches
5. local diagnostics
6. temporary hacks

Temporary hacks should be obvious and easy to drop. If they are still needed after multiple releases, classify them honestly as product patches or upstream candidates.

## Upstream Vs Downstream Decisions

Upstream candidates:

- generic driver fixes
- binding fixes
- reusable board support
- correctness fixes
- maintainable feature additions

Downstream-only candidates:

- product policy
- temporary diagnostics
- unreleased hardware details
- local manufacturing support
- changes tied to private board variants

Even downstream patches should be written as if someone else will rebase them later.

## Review Checklist

For each kernel patch, check:

- Is this source change actually needed?
- Should this be a config fragment instead?
- Should this be Yocto metadata instead?
- Is the patch small enough?
- Does the commit message explain hardware context?
- Does it affect all boards or only one board?
- Is there a runtime validation note?
- Is it temporary or permanent?
- Is it upstreamable?

## Common Mistakes

- Editing the vendor kernel tree directly without exporting patches.
- Mixing DTS, config, driver, and debug changes in one patch.
- Carrying local debug prints into production branches.
- Rebasing by copying files instead of replaying patches.
- Losing track of which SDK release a patch was based on.
- Keeping product config inside a vendor defconfig.
- Putting packaging decisions into kernel source.

## Related Topics

- [Configuration Fragments and Auditing](configuration-fragments-and-auditing.md)
- [Device Tree Binding Validation](device-tree-binding-validation.md)
- [Kernel Release Artifacts](kernel-release-artifacts.md)
- [Configuration and Patch Ownership](../bsp-integration/configuration-and-patch-ownership.md)

## References

- Linux kernel submitting patches documentation
- Yocto Project kernel development documentation
- TI Processor SDK Linux documentation
