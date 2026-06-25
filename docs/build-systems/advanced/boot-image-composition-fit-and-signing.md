---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Boot Image Composition, FIT, And Signing

## What Problem Does This Solve?

Production systems must know exactly which bootloader, kernel, DTB, initramfs, firmware, signatures, and keys are assembled into bootable artifacts. This page connects build outputs to boot image composition and signing workflows.

## Core Concepts

- boot ROM artifact
- SPL/TPL
- U-Boot proper
- FIT image
- ITS file
- kernel image
- DTB
- initramfs
- hash
- signature
- secure boot
- chain of trust

## FIT Image Mental Model

```text
kernel image
DTB
initramfs
load addresses
hashes
signatures
-> ITS description
-> mkimage
-> signed FIT image
```

FIT images are useful because they bundle boot components with metadata and can support hashing and signing.

## Example FIT Build

```bash
mkimage -f product.its product.itb
dumpimage -l product.itb
```

In real products, the `.its` file should be generated or owned by build metadata, not hand-edited after release.

## Signing Concerns

Signing introduces build-system ownership questions:

- where private keys live
- whether signing happens in CI or a secure signing service
- whether unsigned developer images are allowed
- how public keys enter U-Boot or ROM configuration
- how revoked keys and rollback are handled
- how signatures are audited in release artifacts

Never treat signing as a final manual step with no manifest. It must be traceable.

## TI-Specific Note

TI SoCs may have additional boot artifacts and security-device distinctions. Secure and non-secure artifacts are not interchangeable. Always align signing and boot-image generation with the SoC technical reference manual and selected SDK documentation.

## Common Mistakes

- signing a kernel but not the DTB that controls hardware
- updating DTBs outside the signed image
- storing private keys in the source repository
- releasing unsigned developer artifacts by accident
- mixing secure and non-secure boot binaries
- not recording the exact `.its` input used for release

## Related Topics

- [U-Boot FIT Images and Boot Artifacts](u-boot/fit-images-and-boot-artifacts.md)
- [U-Boot Secure Boot and Signing](u-boot/secure-boot-and-signing.md)
- [TI Processor SDK Boot Artifact Pipeline](ti-processor-sdk/boot-artifact-pipeline.md)

