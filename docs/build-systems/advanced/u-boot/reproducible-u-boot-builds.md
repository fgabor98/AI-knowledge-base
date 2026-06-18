---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Reproducible U-Boot Builds

## What Problem Does This Solve?

Reproducible U-Boot builds make bootloader releases auditable and debuggable. If source, defconfig, toolchain, firmware inputs, signing policy, and environment are controlled, a release can be rebuilt or explained later.

For embedded products, this matters because U-Boot is often the first mutable code that runs after ROM or firmware.

## Core Concepts

- source revision
- defconfig
- final `.config`
- toolchain
- host tools
- generated artifacts
- timestamps
- version string
- signing determinism
- artifact checksum
- release manifest

## Controlled Inputs

Record:

- U-Boot source commit
- patch stack
- SDK release
- defconfig
- final `.config`
- selected device tree
- firmware blobs
- toolchain version
- host tool versions
- signing tool version
- environment defaults

## Build Metadata

U-Boot version strings may include:

- source version
- local version
- build date/time
- dirty tree marker
- compiler details

For release builds, define policy for:

- dirty source trees
- version string format
- timestamp handling
- local version suffix
- build container or SDK version

## Artifact Comparison

Compare:

```sh
sha256sum tiboot3.bin tispl.bin u-boot.img u-boot.itb
```

Archive:

```text
checksums.sha256
manifest.txt
build.log
final.config
```

For FIT images:

```sh
dumpimage -l u-boot.itb
```

## Signing And Reproducibility

Signed artifacts may differ even when unsigned inputs match if signing metadata changes.

Record:

- unsigned checksums
- signed checksums
- signing key identifier
- signing tool version
- certificate metadata
- signing timestamp policy

Do not compare signed artifacts without understanding signing metadata.

## CI Policy

CI should:

- use pinned SDK/container/toolchain
- reject dirty release trees
- archive final `.config`
- archive all boot artifacts
- archive checksums
- archive serial validation logs when hardware testing is available
- record signing status
- publish manifest with release bundle

## Common Mistakes

- Recording source commit but not defconfig.
- Archiving U-Boot proper but not SPL.
- Ignoring firmware blobs.
- Comparing signed artifacts without unsigned checksums.
- Reusing output directories across boards.
- Not recording environment defaults.
- Allowing dirty local builds into release.

## Debugging Checklist

- Is source commit recorded?
- Is patch stack recorded?
- Is defconfig recorded?
- Is final `.config` recorded?
- Are firmware blobs recorded?
- Is toolchain recorded?
- Are all artifacts checksummed?
- Is signing metadata recorded?
- Can the version string be tied to a release?

## Related Topics

- [Release Artifacts and Provenance](release-artifacts-and-provenance.md)
- [Secure Boot and Signing](secure-boot-and-signing.md)
- [Cross-Building and Flashing](cross-building-and-flashing.md)
- [BSP Release Reproducibility](../bsp-integration/release-reproducibility.md)

## References

- U-Boot documentation
- Reproducible Builds project documentation
- Vendor SDK release documentation
