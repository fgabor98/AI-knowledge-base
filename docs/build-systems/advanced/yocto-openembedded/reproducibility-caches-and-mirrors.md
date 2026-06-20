---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Reproducibility, Caches, and Mirrors

## What Problem Does This Solve?

Production embedded Linux builds must be rebuildable after source servers change, developers leave, caches are cleaned, or a field issue appears. Yocto provides downloads caches, source mirrors, shared state, locked revisions, license data, build history, and reproducibility mechanisms, but they require deliberate release policy.

## Core Concepts

- `DL_DIR`
- `SSTATE_DIR`
- source mirror
- sstate mirror
- fixed revision
- shared-state signature
- reproducible build
- source archive
- license manifest
- build history
- release manifest
- offline build

## Mental Model

```text
source inputs + metadata revisions + configuration + toolchain policy
-> deterministic tasks
-> reusable sstate where signatures match
-> release artifacts + manifests + archived source/license data
```

Caching improves speed. Reproducibility proves identity. They are related but not the same.

## Downloads Cache

`DL_DIR` stores fetched source archives and repository mirrors.

Configure shared location:

```bitbake
DL_DIR = "/srv/yocto/downloads"
```

Benefits:

- avoids repeated downloads
- supports multiple build directories
- helps prepare offline builds

The cache is not a complete release source archive unless it is intentionally preserved and verified.

## Shared-State Cache

`SSTATE_DIR` stores reusable task output indexed by signatures.

```bitbake
SSTATE_DIR = "/srv/yocto/sstate-cache"
```

Benefits:

- faster developer builds
- faster CI
- reduced duplicate compilation

Do not treat sstate as source-of-truth release artifacts. It is an optimization cache.

## Source Mirrors

Mirrors provide alternate source locations when upstream is slow, unavailable, moved, or prohibited in release infrastructure.

Policies may include:

- premirrors checked before upstream
- fallback mirrors
- internal source archive
- network-disabled release build after cache population

Test mirror completeness. A build that silently falls back to the public network has not proven offline rebuildability.

## Sstate Mirrors

An sstate mirror publishes reusable task output to developers/CI.

Consider:

- trust boundary
- access control
- release/branch namespace
- retention
- poisoning risk
- compatibility across hosts/configurations

Only consume trusted sstate for release builds.

## Fixed Source Revisions

Release recipes should use fixed source revisions and verified archive checksums.

Avoid:

- branch heads
- floating tags
- automatically changing latest revisions
- unchecksummed archives

Record layer repository revisions too; recipe `SRCREV` values alone do not capture metadata state.

## AUTOREV

Automatic latest-revision behavior can be useful during active development but undermines release reproducibility.

If used:

- scope it to development configuration
- never rely on it for release identity
- replace with fixed commits before release
- record resolved revisions during CI

## Task Signatures

Task signatures determine whether cached output is reusable.

Inputs can include:

- variable values
- task code
- dependency signatures
- source revisions
- configuration

If a task reuses stale-looking output, investigate signature inputs rather than deleting caches blindly.

## Reproducibility Levels

### Rebuildable

All source, metadata, configuration, and tools are available to rebuild.

### Functionally Reproducible

Rebuilt artifacts provide equivalent product behavior, with understood metadata differences.

### Byte-Reproducible

Selected artifacts are byte-identical across controlled builds.

Define which level is required for each release artifact.

## Sources Of Nondeterminism

- timestamps
- build paths
- host user/hostname
- unordered filesystem traversal
- random IDs
- network-generated content
- generated archives with unstable metadata
- embedded Git dirty state
- unpinned tools
- signing timestamps/nonces

Use Yocto reproducibility tooling and fix upstream recipes rather than postprocessing unexplained differences.

## Build History

Build history can record changes in:

- package versions
- package dependencies
- installed files
- image composition
- image size

It helps review intentional and accidental product changes between builds.

Keep build history under an appropriate retention/version-control policy; it can become large and noisy without review discipline.

## License And Source Artifacts

Release requirements can include:

- license manifest
- license text archive
- corresponding source archive
- patch/source metadata
- package manifest
- SBOM
- CVE report

Configure and test these outputs before release day.

Legal obligations depend on licenses and distribution model; tooling output supports but does not replace legal review.

## Release Manifest

Record:

- product version
- `MACHINE`, `DISTRO`, image target
- all layer repositories/commits
- kas/repo manifest if used
- provider and source revisions
- tool/container identity
- artifact checksums
- package/image manifest
- license/source/SBOM outputs
- signing metadata
- build logs and validation results

## Offline Build Test

Recommended release gate:

1. Populate approved source mirror/download cache.
2. Start clean build directory.
3. Disable external network access.
4. Build release image using approved mirrors.
5. Confirm no missing source.
6. Record source archive and checksums.

This tests source availability, not necessarily byte reproducibility.

## Cache Maintenance

Plan:

- capacity monitoring
- retention by branch/release
- safe cleanup tooling
- permissions
- backups for source mirrors
- separation between disposable sstate and required source archives

Never let an automated cache cleanup delete the only copy of release source.

## CI Architecture

Typical flow:

```text
trusted source mirror
+ shared downloads
+ trusted sstate mirror/cache
+ pinned layer manifest
-> clean CI build directory
-> artifacts/manifests/compliance output
-> hardware validation
-> signed release publication
```

Use clean build directories for release confidence while sharing controlled caches for performance.

## TI Processor SDK Release Perspective

Record:

- TI SDK release and layer revisions
- TI kernel/U-Boot/firmware revisions
- product layer revision
- toolchain/container version
- machine/image target
- security-device/signing variant
- complete deploy artifact set

Vendor download URLs and branches can change; internal source mirroring is valuable for long-lived products.

## Worked Example: Offline Rebuild Gate

```text
Job A: populate approved downloads/source mirror
Job B: clean workspace, public network blocked
Job B: build product-image from pinned layer manifest
Job B: publish missing-source report if fetch tries outside mirror
```

Passing proves source availability under the configured policy. Run separate artifact comparison for reproducibility.

## Worked Example: Cache Is Fast But Release Is Wrong

If a build restores sstate successfully but board boots old kernel:

1. Confirm kernel deploy checksum.
2. Confirm WIC boot partition checksum.
3. Confirm flashed media checksum/serial identity.
4. Only then investigate kernel task signatures.

Sstate correctness cannot fix flashing the wrong artifact.

## Common Mistakes

- Treating sstate as source archival.
- Using floating source revisions in releases.
- Keeping only final images without metadata/source manifests.
- Never testing an offline rebuild.
- Sharing untrusted sstate into release builds.
- Deleting caches before understanding signature behavior.
- Generating license/source artifacts only after product release.
- Failing to archive signing metadata and unsigned checksums.

## Debugging Checklist

- Are all layer/source revisions fixed?
- Is every archive checksummed?
- Are sources available from controlled mirrors?
- Can release build run without public network?
- Is sstate trusted and correctly keyed?
- Are output differences explained?
- Are build history and package manifests reviewed?
- Are license/source/SBOM artifacts generated?
- Are deploy artifacts checksummed and archived?
- Can release be reconstructed from manifest?

## Related Topics

- [Build Directory and Configuration](build-directory-and-configuration.md)
- [Tasks and Workdirs](tasks-and-workdirs.md)
- [Debugging BitBake Builds](debugging-bitbake-builds.md)
- [BSP Release Reproducibility](../bsp-integration/release-reproducibility.md)

## References

- Yocto Project Reproducible Builds documentation
- Yocto Project Reference Manual
- BitBake User Manual
- OpenChain and SPDX documentation
