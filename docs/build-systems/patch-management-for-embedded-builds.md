---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Patch Management For Embedded Builds

## What Problem Does This Solve?

Embedded products often carry vendor patches, board patches, backports, workarounds, and product changes for years. Patch management keeps those changes reviewable, rebased, upstreamable, and traceable across SDK upgrades.

## Core Concepts

- patch series
- `git format-patch`
- `git am`
- quilt
- upstream status
- vendor branch
- product branch
- `.bbappend`
- patch provenance

## Good Patch Metadata

A useful patch answers:

- what does this change?
- why is it needed?
- is it product-specific or generally useful?
- has it been submitted upstream?
- which SDK or kernel/U-Boot version was it based on?
- how was it tested?

For Yocto-style recipes, include patches through metadata:

```bitbake
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-board-fix-ethernet-reset-gpio.patch \
"
```

## Patch Stack Categories

Classify patches:

- upstream backport
- vendor patch
- board enablement
- product policy
- temporary workaround
- debug-only change

Debug-only changes should not quietly enter production images.

## Rebase Workflow

For SDK upgrades:

1. reproduce old baseline
2. reproduce new vendor baseline
3. list product patches
4. drop patches already included upstream/vendor
5. rebase remaining patches
6. retest each affected subsystem
7. update patch notes

## Common Mistakes

- editing vendor layers directly
- carrying patches with no explanation
- combining unrelated changes into one patch
- leaving debug prints in production patches
- rebasing without comparing against the new vendor baseline
- treating "patch applies" as proof that behavior is still correct

## Related Topics

- [Source Fetching and Patch Management](source-fetching-and-patch-management.md)
- [Vendor Kernel Patch Management](advanced/linux-kernel/vendor-kernel-patch-management.md)
- [Vendor U-Boot Patch Management](advanced/u-boot/vendor-u-boot-patch-management.md)

