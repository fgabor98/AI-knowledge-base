---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Board Porting Build Workflow

## What Problem Does This Solve?

Board porting is not only driver work. It is a build workflow that moves from a vendor EVM to a product board through controlled changes in machine metadata, U-Boot, kernel, device tree, firmware, image layout, and deployment.

## Recommended Flow

```text
vendor EVM prebuilt image
-> source-built EVM image
-> artifact map
-> product machine
-> product DTS
-> U-Boot changes
-> kernel config and patches
-> firmware packaging
-> WIC/deployment changes
-> boot validation
```

## Change Ownership

| Change | Owner |
| --- | --- |
| board identity | machine configuration |
| hardware description | DTS/DTSI |
| early boot storage/DDR | U-Boot/SPL metadata and patches |
| Linux driver enablement | kernel config and patches |
| firmware files | firmware recipes |
| rootfs content | image recipe/package groups |
| partition layout | WIC or image tooling |
| flashing workflow | release/deployment tooling |

## Bring-Up Strategy

Make one class of change at a time:

1. serial console
2. boot media
3. DDR and early boot
4. kernel with minimal rootfs
5. storage
6. networking
7. firmware and remote cores
8. application image content
9. production update/recovery layout

## Evidence To Capture

- full serial log from reset
- boot media switch settings
- U-Boot environment
- kernel command line
- DTB model
- deployed artifact checksums
- image manifest
- board revision

## Common Mistakes

- changing U-Boot, kernel, DTB, rootfs, and media layout all at once
- using EVM machine forever
- skipping source-built EVM validation
- manually copying files without recording checksums
- confusing board electrical issues with build output issues
- not preserving failing boot logs

## Related Topics

- [BSP Build Integration](bsp-build-integration.md)
- [Custom Sitara Board Bring-Up](ti-processor-sdk/custom-sitara-board-bring-up.md)
- [Device Tree Build and Validation](device-tree-build-and-validation.md)

