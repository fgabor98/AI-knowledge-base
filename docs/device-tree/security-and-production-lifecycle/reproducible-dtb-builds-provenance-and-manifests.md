---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Reproducible DTB Builds, Provenance, And Manifests

A reproducible build yields bit-for-bit identical output from the same declared source, toolchain, environment, and procedure. It lets independent builders compare a release DTB with reviewed inputs. It complements signatures: reproducibility detects unexplained build differences; signatures express authorization.

## Define The Reproduction Boundary

Record more than the top-level `.dts`:

- source repository URLs and immutable commits
- patch series and dirty-tree state
- top-level DTS and all included `.dtsi` files
- generated headers and binding constants
- C preprocessor and flags
- `dtc` version, build, and flags
- kernel build configuration and target list
- overlay compilation flags such as `-@`
- host tools, locale, timezone, path, and environment variables
- scripts that patch, pad, sign, package, or apply overlays
- external board databases or generated identity tables

The signed FIT may not be reproducible without access to deterministic signing or the same signature service. Reproduce and compare the unsigned payloads and signed coverage separately.

## Build In Two Independent Roots

An illustrative check:

```bash
export LC_ALL=C
export TZ=UTC
export SOURCE_DATE_EPOCH=1767225600

make -C linux-src O="$PWD/out-a" ARCH=arm64 axc200-revb.dtb
make -C linux-src-copy O="$PWD/out-b" ARCH=arm64 axc200-revb.dtb

sha256sum \
  out-a/arch/arm64/boot/dts/vendor/axc200-revb.dtb \
  out-b/arch/arm64/boot/dts/vendor/axc200-revb.dtb

cmp -s \
  out-a/arch/arm64/boot/dts/vendor/axc200-revb.dtb \
  out-b/arch/arm64/boot/dts/vendor/axc200-revb.dtb
```

True independent reproduction should vary build path, worker, and preferably environment image while holding declared inputs constant. A second build in the same contaminated workspace has weaker diagnostic value.

`SOURCE_DATE_EPOCH` standardizes a time input for tools that support it; setting it does not by itself make a build reproducible.

## Diagnose Byte Differences

When hashes differ:

1. Preserve both outputs and build logs.
2. Compare sizes, FDT headers, and raw bytes.
3. Decode both DTBs with the same `dtc` and compare semantic output.
4. Compare preprocessed DTS inputs.
5. Compare tool versions, flags, include search order, locale, and generated headers.
6. Check node/property ordering and phandle allocation changes.
7. Check padding, symbols, fixups, reservation maps, and packaging.
8. Rebuild after removing one environmental difference at a time.

```bash
fdtdump build-a.dtb > build-a.fdtdump
fdtdump build-b.dtb > build-b.fdtdump

dtc -I dtb -O dts -o build-a.dts build-a.dtb
dtc -I dtb -O dts -o build-b.dts build-b.dtb

cmp -l build-a.dtb build-b.dtb | head
diff -u build-a.dts build-b.dts
```

A semantic match with a byte mismatch still matters when manifests, signatures, or boot policy identify exact bytes.

## Separate Artifact Stages

Hash each stage under a distinct name:

```text
source commit
preprocessed DTS
compiled base DTB
compiled DTBOs
offline-composed diagnostic DTB
unsigned FIT candidate
signed FIT release
packaged partition image
bytes read back from update bundle or media
```

Never label a post-signing or post-packaging hash simply `dtb_sha256` when multiple DTBs exist at different stages.

## Create A Machine-Readable Manifest

The schema is product-specific, but it should be canonical and unambiguous. Example:

```yaml
manifest_version: 1
release_set: axc200-prod-42
security_version: 9
source:
  linux_commit: 0123456789abcdef0123456789abcdef01234567
  patches_digest: sha256:1111...
build:
  environment_digest: sha256:2222...
  dtc_version: "DTC 1.7.x"
  source_date_epoch: 1767225600
artifacts:
  - role: base-dtb
    name: axc200-revb.dtb
    sha256: 3333...
    size: 49152
  - role: overlay
    name: front-panel-v2.dtbo
    sha256: 4444...
    size: 2048
composition:
  fit_configuration: conf-axc200-revb-r42
  overlay_order: [front-panel-v2.dtbo]
compatibility:
  boards: [axc200-revB, axc200-revC, axc200-revD]
  kernel_release: 6.12.18-axc2
```

Use a deterministic serialization or define which exact manifest bytes are signed. A signature over an ambiguous data model is dangerous.

## Sign The Manifest Or Bind It Into The Release

Choose one or more:

- include manifest digest in authenticated update metadata
- sign canonical manifest bytes with a release-metadata key
- embed release-set and manifest digest in a signed FIT configuration
- publish manifest and transparency evidence through a protected service

The bootloader need not parse a rich manifest if signed boot configuration already enforces minimal device policy. Field and support tooling can use the richer document, provided cryptographic binding connects both views.

## Protect The Signing Boundary

Build provenance should show:

```text
reviewed source -> isolated build -> unsigned candidate digest
                 -> policy checks -> release approval
                 -> hardware-backed signing -> signed artifact digest
                 -> packaging -> read-back verification -> publication
```

Do not make private keys available merely to improve reproducibility. Signers should enforce product, artifact type, security version, approver, and key policy and should produce auditable records.

## Release Evidence Bundle

Preserve:

- signed release/update artifact
- signed or bound manifest
- source and patch identifiers
- build environment identity
- full command/tool versions
- validation reports and negative verification tests
- independent reproduction result
- signer/key ID and signing audit reference
- compatibility and hardware test results
- rollout, rollback, and recovery policy

## Review Traps

- exact DTB hash recorded without its source commit
- source commit recorded but build used local patches
- `dtc --version` recorded while the kernel build invoked another binary
- manifest names a DTB but not its size or digest
- separately reproducible components packaged in nondeterministic order
- signed artifact reproducibility claimed despite nondeterministic signature bytes
- release rebuilt later from a moving container tag
- final bootloader-mutated FDT confused with the reproducible base artifact

## Further Reading

- [SOURCE_DATE_EPOCH specification](https://reproducible-builds.org/specs/source-date-epoch/)
- [Reproducible Builds documentation](https://reproducible-builds.org/docs/)
- [Build Pipeline, Preprocessing, And Artifact Provenance](../build-and-diagnostic-tools/build-pipeline-preprocessing-and-artifact-provenance.md)
- [Field Updates, Recovery, And Key Rotation](field-updates-recovery-and-key-rotation.md)
