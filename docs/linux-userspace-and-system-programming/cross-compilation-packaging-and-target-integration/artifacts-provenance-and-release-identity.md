---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Artifacts, Provenance, And Release Identity

Every deployed binary should be traceable to source, build instructions, toolchain, dependency inputs, configuration, and verification results. Use a stable release identifier and embed or expose a build ID that operators can collect from the running target.

## Record the inputs

Capture source revision, dirty-tree status, compiler and linker versions, sysroot digest, package lockfiles, generated files, build flags, target triple, kernel UAPI, image recipe/layer revisions, and test artifacts. Hash the shipped files and the complete image. Store debug symbols and source maps under controlled retention rather than discarding them after packaging.

## Reproducibility and trust

Control timestamps, archive ordering, locale, host paths, randomness, and generated metadata where reproducible builds are a goal. Compare two independent builds and investigate differences rather than declaring success from one checksum.

Authenticity and integrity are distinct: a checksum detects accidental change, while a signature establishes who approved the artifact. Verify signatures before installation and keep the trust-root update and revocation story explicit. An SBOM helps identify affected components but does not replace vulnerability response or runtime testing.

## Release handoff

Ship the artifact, manifest, provenance, SBOM, compatibility statement, migration notes, rollback instructions, and test summary together. The release identity must remain available after a field failure so a core dump, log line, or package can be mapped back to the exact source and dependencies.

## An evidence chain that survives a crash

A release manifest should connect image digest, application build ID, debug
artifact digest, source revision, toolchain/sysroot identity, package list,
kernel/DTB/firmware identities and configuration schema range. Record a test
run identifier and tested hardware revisions beside those artifacts.

A GNU build ID locates corresponding build artifacts; it is not a substitute
for a cryptographic authenticity policy. A file digest proves equality to
the recorded bytes; a trusted signature binds an approver to a manifest.
The manifest is useful only when its provenance and retention are controlled.

At runtime, expose a bounded `--version` or status response with immutable
build identity and mutable configuration generation separately. A dirty-tree
build should not report itself as identical to the clean revision. A kernel
release string alone also need not distinguish two differently configured
vendor kernels.

## Reproduce and compare

Build twice with the same declared inputs in independent clean locations.
Compare image/package digests, then inspect differing archives, ELF metadata,
timestamps and generated content. A deterministic application binary does
not prove the entire image is reproducible.

Archive the stripped deployed binary and matching symbols, then perform one
offline symbolization exercise using only the archived release bundle.
Document missing sources, symbols or licenses as release gaps before shipping.
Keep secrets and signing keys out of diagnostic/provenance bundles.

Use the same manifest for update acceptance and incident investigation so a
field report can identify a mixed kernel, firmware or userspace generation.

## Related topics

- [Stage 15 overview](index.md)
- [ELF and loader diagnosis](../diagnostics-debugging-and-performance/elf-abi-and-loader-diagnostics.md)
