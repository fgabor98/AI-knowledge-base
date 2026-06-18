---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Vendor U-Boot Patch Management

## What Problem Does This Solve?

Vendor U-Boot trees often carry SoC support, firmware integration, board ports, secure boot support, and SDK-specific packaging. Product teams then add custom board changes on top.

Patch management keeps those layers separable so SDK upgrades and board revisions remain possible.

## Core Concepts

- upstream U-Boot
- vendor U-Boot
- SDK U-Boot
- product patch
- board port
- DTS patch
- SPL patch
- environment policy patch
- temporary bring-up patch
- rebase

## Patch Categories

Classify changes:

| Patch Type | Example | Preferred Treatment |
| --- | --- | --- |
| board DTS | custom carrier board | product board patch |
| defconfig | enable board target | product board patch |
| SPL/DRAM | custom memory topology | high-risk board patch |
| environment | boot target order | product boot policy |
| driver fix | generic bug | upstream candidate |
| debug hack | extra logs/delays | temporary patch |
| packaging | deploy artifact names | build metadata or recipe |

## Good Patch Stack Shape

Use small patches:

```text
0001-arm-dts-add-product-board-u-boot-dts.patch
0002-configs-add-product-board-defconfig.patch
0003-board-ti-add-product-board-detection.patch
0004-env-set-product-boot-target-order.patch
```

Avoid one large patch containing DTS, DRAM, environment, and driver changes.

## SDK Upgrade Flow

1. Record old SDK and U-Boot commit.
2. Record product patch stack.
3. Import new SDK baseline.
4. Replay patches one at a time.
5. Rebuild all boot artifacts.
6. Validate serial from reset.
7. Validate environment and boot flow.
8. Validate Linux handoff.

Do not copy the old modified tree over the new vendor tree.

## Temporary Bring-Up Patches

Mark temporary patches clearly:

- debug prints
- forced delays
- disabled authentication
- hardcoded boot paths
- temporary PMIC settings

Before release, either remove them or promote them to reviewed product patches with explanation.

## Common Mistakes

- Editing vendor tree directly without exported patches.
- Mixing debug hacks into board support.
- Rebasing by copying directories.
- Putting boot environment policy inside unrelated driver patches.
- Losing track of SDK baseline.
- Treating secure boot workarounds as production changes.

## Debugging Checklist

- Identify upstream/vendor/product layers.
- Identify SDK baseline.
- Classify every patch.
- Separate board DTS, defconfig, SPL, environment, and driver work.
- Rebase one patch at a time.
- Validate artifacts after rebase.
- Remove or justify temporary patches.

## Related Topics

- [Board Porting and Bring-Up](board-porting-and-bring-up.md)
- [Secure Boot and Signing](secure-boot-and-signing.md)
- [Release Artifacts and Provenance](release-artifacts-and-provenance.md)
- [Configuration and Patch Ownership](../bsp-integration/configuration-and-patch-ownership.md)

## References

- U-Boot development documentation
- Vendor SDK documentation
- U-Boot submitting patches documentation
