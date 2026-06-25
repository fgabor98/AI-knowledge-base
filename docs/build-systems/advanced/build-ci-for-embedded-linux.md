---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Build CI For Embedded Linux

## What Problem Does This Solve?

Embedded CI must build large images, reuse caches safely, publish artifacts, collect manifests, and often boot hardware. This page defines the build-system responsibilities of a platform CI pipeline.

## CI Stages

```text
checkout pinned sources
-> restore downloads/sstate/cache
-> parse metadata
-> build selected targets
-> run static checks
-> publish artifacts
-> boot smoke test
-> archive logs and manifests
```

## Infrastructure Pieces

- build runners
- shared downloads mirror
- shared sstate/cache
- artifact repository
- license/SBOM archive
- hardware lab
- serial log capture
- power control
- result dashboard

## Hardware-In-The-Loop

Useful checks:

- board powers on
- serial console appears
- U-Boot version matches build
- kernel boots
- rootfs reaches expected target
- network comes up
- critical services start
- firmware loads

Hardware CI should save full serial logs.

## Cache Policy

Cache aggressively but deliberately:

- share downloads
- share sstate where hash policy allows
- do not share unstable workdirs
- invalidate caches on toolchain and metadata changes
- measure cache hit rates

## Common Mistakes

- building from unpinned branches
- not archiving failed task logs
- treating build success as boot success
- letting every job fetch from the public internet
- using one mutable build directory for unrelated branches
- publishing images without manifests

## Related Topics

- [Build Quality Gates](../build-quality-gates.md)
- [Yocto CI, Hash Equivalence, and Shared State](yocto-openembedded/ci-hash-equivalence-and-sstate.md)
- [Reproducible Embedded Linux Releases](reproducible-embedded-linux-releases.md)

