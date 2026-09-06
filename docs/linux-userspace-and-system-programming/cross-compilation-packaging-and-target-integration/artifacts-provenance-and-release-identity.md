# Artifacts, Provenance, And Release Identity

Every deployed binary should be traceable to source, build instructions, toolchain, dependency inputs, configuration, and verification results. Use a stable release identifier and embed or expose a build ID that operators can collect from the running target.

## Record the inputs

Capture source revision, dirty-tree status, compiler and linker versions, sysroot digest, package lockfiles, generated files, build flags, target triple, kernel UAPI, image recipe/layer revisions, and test artifacts. Hash the shipped files and the complete image. Store debug symbols and source maps under controlled retention rather than discarding them after packaging.

## Reproducibility and trust

Control timestamps, archive ordering, locale, host paths, randomness, and generated metadata where reproducible builds are a goal. Compare two independent builds and investigate differences rather than declaring success from one checksum.

Authenticity and integrity are distinct: a checksum detects accidental change, while a signature establishes who approved the artifact. Verify signatures before installation and keep the trust-root update and revocation story explicit. An SBOM helps identify affected components but does not replace vulnerability response or runtime testing.

## Release handoff

Ship the artifact, manifest, provenance, SBOM, compatibility statement, migration notes, rollback instructions, and test summary together. The release identity must remain available after a field failure so a core dump, log line, or package can be mapped back to the exact source and dependencies.
