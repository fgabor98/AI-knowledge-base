---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Secure Boot and Signing

## What Problem Does This Solve?

Secure boot and signing ensure that only trusted boot artifacts execute. U-Boot can participate in several trust layers: SoC ROM authentication, SPL authentication, U-Boot proper verification, FIT signatures, and Linux artifact verification.

For embedded Linux products, this topic is critical because signing changes the build pipeline, artifact ownership, release process, recovery process, and debugging workflow.

## Core Concepts

- root of trust
- SoC ROM authentication
- secure vs non-secure device variants
- SPL authentication
- U-Boot verified boot
- FIT signature
- hash
- certificate
- key material
- signing tool
- production key
- debug key
- anti-rollback

## Mental Model

Secure boot is a chain:

```text
ROM trusts fused key/hash
-> ROM authenticates first stage
-> first stage authenticates next stage
-> U-Boot verifies FIT or boot artifacts
-> Linux verifies later components if configured
```

Every stage must know what it trusts and what it verifies.

## Authentication Vs Hashing

Hashing detects accidental or malicious modification if the hash itself is trusted.

Signing proves that a trusted key authorized the artifact.

In FIT:

- hashes can protect image data from corruption
- signatures can protect images and/or configurations
- signing configurations is usually important because configuration selection matters

## FIT Signatures

FIT signing usually starts from an ITS file with hashes and signatures.

Build:

```sh
mkimage -f image.its image.itb
```

Sign:

```sh
mkimage -F -k keys -K u-boot.dtb -r image.itb
```

Exact commands depend on U-Boot version and project policy.

Inspect:

```sh
dumpimage -l image.itb
mkimage -l image.itb
```

## What Requires Resigning?

Usually, any change to these requires regenerating and resigning the FIT:

- kernel image
- DTB
- initramfs
- load address metadata
- entry address metadata
- configuration node
- hash algorithm
- signature node

If a product "only changed the DTB," signed boot still treats that as a boot artifact change.

## Key Material Ownership

Separate responsibilities:

- developers can build unsigned or debug-signed artifacts
- CI can produce controlled release candidates
- release/security owner controls production signing
- manufacturing handles fused keys or device provisioning

Do not store production private keys in ordinary source repositories or developer workstations.

## Debug Vs Production Keys

Use separate keys for:

- local development
- CI test images
- factory images
- production releases

Device policy must decide which keys are accepted. A production device accepting development keys is not production secure boot.

## SoC ROM Authentication

Many SoCs authenticate the first boot stage before U-Boot code runs. This is vendor-specific.

Build implications:

- first-stage artifact may need vendor signing tools
- image format may differ by device security type
- certificates or headers may be prepended
- boot ROM may enforce load address and size constraints
- recovery flow may differ for secure devices

For TI platforms, distinguish GP, HS-FS, and HS-SE style workflows according to the exact SoC and SDK documentation.

## U-Boot Verified Boot

U-Boot verified boot commonly verifies a signed FIT before booting Linux.

Build implications:

- U-Boot must contain or access public keys
- FIT must be signed with corresponding private key
- selected configuration must be verified
- boot command must use verification-aware boot flow
- key updates require careful release policy

## Anti-Rollback

Some systems prevent booting older signed images.

This may involve:

- monotonic version counters
- rollback indexes
- eFuses
- secure storage
- bootloader policy

Build artifacts then need version metadata, and release engineering must handle version increments carefully.

## Signing Pipeline Shape

Recommended separation:

```text
build unsigned artifacts
-> generate manifest and checksums
-> sign in controlled environment
-> verify signed artifacts
-> publish release bundle
```

This makes it clear which step needs key access.

## Reproducibility With Signing

Signing can break byte-for-byte reproducibility if:

- signatures include timestamps
- certificate metadata changes
- key order changes
- packaging tools differ
- unsigned inputs are not deterministic

For production, record:

- unsigned artifact checksums
- signed artifact checksums
- signing tool version
- key identifier
- signing policy
- CI job or signing request ID

## Debugging Secure Boot Failures

Symptoms:

- ROM refuses first stage
- SPL authentication failure
- U-Boot reports bad FIT signature
- boot command refuses unsigned image
- board works with unsigned debug build but fails production boot

Checks:

- correct device security type
- correct signing keys
- correct artifact signed
- correct FIT configuration signed
- U-Boot contains matching public key
- boot command uses signed artifact
- no artifact modified after signing

## TI Sitara Considerations

For TI Processor SDK-style work:

- identify SoC security variant
- use SDK-matched signing tools
- keep firmware, SPL, U-Boot, and certificates aligned
- archive signing logs and generated signed artifacts
- do not mix GP and HS artifact assumptions
- validate boot on the same security class as production hardware

## Common Mistakes

- Signing the kernel but not the FIT configuration.
- Editing DTB after signing.
- Using development keys on a production-intended image.
- Mixing signed first stage with unsigned later boot policy.
- Losing the unsigned artifact checksum.
- Testing only on non-secure EVMs while targeting secure production devices.
- Treating signing as a final copy step instead of a release-controlled process.

## Debugging Checklist

- Identify the trust chain.
- Identify device security type.
- Identify every signed artifact.
- Identify every public key embedded or provisioned.
- Identify signing tool version.
- Inspect FIT signatures.
- Confirm selected FIT configuration is signed.
- Confirm no artifacts changed after signing.
- Capture serial log from reset.
- Archive signing manifest and checksums.

## Related Topics

- [FIT Images and Boot Artifacts](fit-images-and-boot-artifacts.md)
- [Release Artifacts and Provenance](release-artifacts-and-provenance.md)
- [Reproducible U-Boot Builds](reproducible-u-boot-builds.md)
- [TI Processor SDK Linux](../ti-processor-sdk/index.md)

## References

- U-Boot verified boot documentation
- U-Boot FIT image documentation
- SoC vendor secure boot documentation
