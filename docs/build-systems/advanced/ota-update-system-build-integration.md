---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# OTA And Update System Build Integration

## What Problem Does This Solve?

Update systems are build-system concerns because the build must generate signed bundles, partition layouts, bootloader coordination, version metadata, rollback policy, and installable artifacts.

## Core Concepts

- A/B rootfs
- rollback
- update bundle
- artifact signing
- bootloader environment
- RAUC
- SWUpdate
- Mender
- immutable rootfs
- data partition

## Build Outputs

An update-capable build may produce:

- full disk image
- rootfs image
- update bundle
- manifest
- signature
- compatibility metadata
- bootloader environment defaults
- recovery image

## A/B Mental Model

```text
active slot A
-> install update into inactive slot B
-> mark B pending
-> reboot
-> bootloader selects B
-> userspace marks B good
-> rollback to A if validation fails
```

The bootloader and Linux userspace must agree on slot metadata.

## Build-System Ownership

Own these in metadata:

- partition layout
- bundle format
- signing keys or signing service interface
- version fields
- compatible hardware identifiers
- bootloader variables
- post-install hooks
- rollback tests

## Common Mistakes

- adding OTA late after partition layout is fixed
- signing only the transport package and not payloads
- forgetting bootloader environment persistence
- updating rootfs without matching kernel/modules
- not testing power loss during update
- not distinguishing factory image from update bundle

## Related Topics

- [Boot Image Composition, FIT, and Signing](boot-image-composition-fit-and-signing.md)
- [Filesystem Image Basics](../filesystem-image-basics.md)
- [Embedded Productization](../../embedded-productization/index.md)

