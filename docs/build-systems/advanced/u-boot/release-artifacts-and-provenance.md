---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Release Artifacts and Provenance

## What Problem Does This Solve?

A U-Boot release is a boot chain, not a single binary. To reproduce, debug, flash, or certify a system, you need the exact artifacts, configuration, source revisions, firmware components, signing state, and deployment layout.

For embedded Linux products, missing U-Boot provenance makes boot failures hard to diagnose and field updates risky.

## Core Concepts

- release bundle
- SPL/TPL artifacts
- U-Boot proper
- FIT image
- firmware blob
- defconfig
- final `.config`
- environment defaults
- boot media layout
- checksum
- serial log
- signing provenance

## Required Artifacts

Archive:

- first-stage boot artifacts
- SPL/TPL artifacts
- U-Boot proper artifacts
- FIT images or boot scripts
- U-Boot DTBs
- firmware blobs required by the platform
- defconfig
- final `.config`
- build logs
- map files where useful
- checksums
- serial boot log from reset

For TI-style boot chains, archive the named deploy artifacts as a set, not individually.

## Suggested Layout

```text
u-boot-release/
  manifest.txt
  artifacts/
    tiboot3.bin
    tispl.bin
    u-boot.img
    u-boot.itb
  config/
    board_defconfig
    final.config
    environment.txt
  logs/
    build.log
    serial-from-reset.log
  provenance/
    source-revisions.txt
    toolchain.txt
    checksums.sha256
```

## Manifest Contents

Record:

- product
- board revision
- SoC and security type
- U-Boot version
- SDK release
- source commit
- patch stack revision
- defconfig
- final `.config` checksum
- toolchain version
- signing state
- boot media layout
- artifact checksums

## Serial Log As Artifact

A release should include a serial log from reset through Linux handoff.

It proves:

- first stage runs
- SPL version
- U-Boot proper version
- selected boot media
- selected kernel/DTB/FIT path
- bootargs handoff

This is especially useful when a field board later appears to boot an older stage.

## Environment Provenance

Archive:

- compiled default environment
- manufacturing environment policy
- expected saved variables
- environment storage location
- reset procedure

Environment is part of boot behavior. Treat it as release data.

## Common Mistakes

- Archiving only `u-boot.img`.
- Losing SPL or first-stage artifacts.
- Not recording final `.config`.
- Not recording saved environment assumptions.
- Not preserving signing logs.
- Not recording boot media offsets.
- Not capturing serial output from reset.

## Debugging Checklist

- Does serial log match release version?
- Do flashed artifacts match checksums?
- Are SPL and U-Boot proper from same release?
- Is the saved environment expected?
- Is the board security type recorded?
- Are signing inputs recorded?
- Is boot media layout documented?

## Related Topics

- [Source Tree and Outputs](source-tree-and-outputs.md)
- [Environment and Boot Flow](environment-and-boot-flow.md)
- [Secure Boot and Signing](secure-boot-and-signing.md)
- [Reproducible U-Boot Builds](reproducible-u-boot-builds.md)

## References

- U-Boot documentation
- U-Boot environment documentation
- Vendor BSP release documentation
