---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# CI, Hash Equivalence, and Shared State

## What Problem Does This Solve?

Yocto CI must produce trustworthy releases without compiling the world unnecessarily. Shared downloads, sstate, hash equivalence, clean workspaces, artifact promotion, and hardware validation provide speed while preserving traceability.

## Core Concepts

- clean build directory
- shared downloads
- shared state
- sstate mirror
- task hash
- hash equivalence
- artifact promotion
- build manifest
- cache trust
- hardware test
- release gate

## Reference Pipeline

```text
pin metadata/source manifest
-> parse/config sanity
-> build using trusted caches
-> compliance and reproducibility checks
-> image/artifact publication to candidate store
-> hardware boot/runtime validation
-> signing/promotion to release store
```

```mermaid
flowchart LR
    LOCK[Pinned source and layer manifest]
    CHECK[Parse and metadata checks]
    BUILD[Clean image build]
    QA[QA, CVE, license, and SBOM]
    CAND[Immutable candidate artifacts]
    HW[Hardware boot and runtime tests]
    SIGN[Controlled signing]
    REL[Release promotion]

    LOCK --> CHECK --> BUILD --> QA --> CAND --> HW --> SIGN --> REL
    HW -->|failed| BUILD
```

The failed-test arrow means a new corrected build must create a new candidate; tested artifacts are never modified in place.

## Clean Workspace, Shared Caches

Use fresh/isolated `TMPDIR` for release jobs while sharing:

- controlled `DL_DIR`
- trusted `SSTATE_DIR` or mirror

This tests that no untracked workdir state is required while retaining performance.

## Cache Trust Boundary

Sstate contains build outputs that can enter release artifacts.

Control:

- who can publish
- branch/release namespace
- CI identity
- transport/authentication
- retention
- compromise response

Do not consume arbitrary developer-writable caches in production signing jobs.

## Hash Equivalence

Hash equivalence can identify tasks whose output is equivalent even if metadata/task hashes differ, enabling broader reuse.

Conceptually:

```text
different task input hashes
-> equivalence service/output knowledge
-> known equivalent output
-> reuse instead of rebuild
```

Use the release-supported hash-equivalence configuration and operate the service as trusted build infrastructure.

## Diagnosing Cache Misses

Unexpected rebuild:

1. Identify task.
2. Compare task signatures.
3. Inspect variable/dependency differences.
4. Check machine/distro/tune changes.
5. Check cache/mirror availability.
6. Check host/environment variables entering signature.
7. Confirm equivalent output is known.

Do not exclude inputs from hashes merely to improve hit rate.

## Worked CI Configuration Pattern

Environment-specific settings can point builds at:

```bitbake
DL_DIR = "/ci-cache/downloads"
SSTATE_DIR = "/ci-cache/sstate"
```

Or use read-only mirrors plus per-job writable local caches.

Separate:

- trusted release cache
- feature-branch cache
- public upstream mirrors
- required source archive

## Pipeline Stages

### Metadata Validation

- check layer compatibility/dependencies
- parse target
- check unmatched appends
- inspect provider selections
- lint metadata where tooling exists

### Component Build

- build changed recipes or representative targets
- run recipe tests/QA
- generate task logs

### Full Image Build

- build release image/WIC/SDK
- generate package/license/SBOM/CVE output
- archive manifests/checksums

### Runtime Validation

- boot on target hardware/emulator
- capture serial logs
- verify U-Boot/kernel/rootfs identities
- run smoke/integration tests
- verify firmware/remoteproc

### Promotion

- sign approved artifacts
- publish immutable release bundle
- retain unsigned and signed checksums
- record test results and source manifest

## Artifact Promotion

Build once, test and promote the same bytes.

Avoid rebuilding after validation for release publication because the published bytes may differ from tested bytes.

If signing changes bytes, treat signing as a controlled transformation and validate signature plus signed artifact identity.

## Worked Manifest

```text
build-id: 2026.06.20-1234
machine: product-board
distro: product
image: product-image
layers: manifest.lock
kernel: commit/checksum
u-boot: commit/checksum
wic-sha256: ...
sdk-sha256: ...
sbom-sha256: ...
hardware-test: passed/job-5678
```

Prefer structured machine-readable manifests where practical.

## CI Concurrency

Multiple jobs sharing caches require:

- filesystem/service concurrency support
- correct permissions
- atomic publication
- cleanup that does not race active jobs
- disk monitoring

Use supported mirror/cache architectures rather than ad hoc rsync during active writes.

## Build Resource Controls

Tune:

- BitBake task parallelism
- per-task compile parallelism
- memory limits
- disk space/inodes
- I/O contention

A CI host with excessive parallelism can be slower and less reliable.

## Hardware Lab Integration

Automate where possible:

- power cycle
- serial capture
- flash/provision
- boot timeout
- version checks
- network/storage tests
- artifact log upload

Always record board revision, serial number, and boot source.

## Security And Secrets

Keep separate:

- source credentials
- package feed credentials
- signing keys
- device provisioning secrets

Build jobs should receive least privilege. Production signing should not run in ordinary feature-branch jobs.

## Common Mistakes

- Reusing dirty `TMPDIR` as release proof.
- Letting untrusted jobs publish production sstate.
- Rebuilding after hardware validation instead of promoting tested bytes.
- Hiding signature inputs to improve cache hits.
- Publishing artifacts without manifests/checksums.
- Running signing keys in general build workers.
- Treating successful build as hardware validation.

## Debugging Checklist

- Is source/layer manifest pinned?
- Is build workspace clean?
- Which caches/mirrors are trusted?
- Why did task hit/miss cache?
- Are cache services healthy?
- Are compliance artifacts generated?
- Are tested bytes the promoted bytes?
- Are hardware logs and identities archived?
- Is signing isolated and traceable?

## Related Topics

- [Reproducibility, Caches, and Mirrors](reproducibility-caches-and-mirrors.md)
- [Debugging BitBake Builds](debugging-bitbake-builds.md)
- [Licensing, CVE, and SBOM Workflows](licensing-cve-and-sbom.md)
- [BSP Release Reproducibility](../bsp-integration/release-reproducibility.md)

## References

- Yocto Project build performance and reproducibility documentation
- BitBake signature and hash equivalence documentation
- Yocto Project test infrastructure documentation
