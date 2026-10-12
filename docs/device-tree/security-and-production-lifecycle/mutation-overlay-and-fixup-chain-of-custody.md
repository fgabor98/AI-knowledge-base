---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Mutation, Overlay, And Fixup Chain Of Custody

Authentication proves the bytes that were covered at verification time. Bootloaders frequently modify the working FDT afterward. Security review must therefore account for the final tree handed to Linux, the code making each change, and every input that drives it.

## Build A Transformation Ledger

Use the checkpoint timeline from [Boot-Time Mutation And Ownership](../boot-time-mutation-and-ownership.md#treat-the-dtb-as-a-build-product-with-a-timeline), extended to the Linux live tree after any runtime overlays. Its [mutation record](../boot-time-mutation-and-ownership.md#mutation-record) captures ownership; security review adds authorization, determinism, and measurement.

For each transition capture:

| Field | Question |
|---|---|
| code owner | Which authenticated boot stage performs it? |
| input | Where does the new value come from? |
| authorization | Why may this input affect the tree? |
| validation | What range, format, identity, and overlap checks run? |
| determinism | Can the transformation be reproduced from recorded inputs? |
| measurement | Is the final effect covered by a later measurement? |
| evidence | Can a field unit report before/after identity or a ledger entry? |

## Classify Mutations

Use the data classes in [Classify The Data](../boot-time-mutation-and-ownership/mutation-provenance-authorities-and-checkpoints.md#classify-the-data). Their security treatment differs:

- **Deterministic release transformations** (includes, versioned overlays) can be reproduced and authenticated as release inputs.
- **Device-identity transformations** (serials, MACs, calibration, board revision) need structure, authenticity, uniqueness, product-binding, and bounds checks; physical proximity does not make EEPROM data trusted.
- **Hardware-discovery transformations** (DRAM size, disabled cores, firmware reservations) need a defined trusted-firmware interface and rejection of overlaps or impossible values.
- **Ephemeral transformations** (seeds, initrd location, event-log addresses) are legitimate without reproducible final bytes; record their origin and measure the final handoff when attestation matters.
- **Operator- or environment-driven transformations** (boot arguments, overlay lists, environment-derived addresses) are high risk; authenticate or constrain the policy rather than trusting arbitrary strings.

## Overlay Trust Rules

An overlay can rewrite security-relevant properties as effectively as a replacement base DTB. Production policy must authenticate its identity, base compatibility, allowed targets, order, conflicts, entitlement, lifecycle, and measurement ([Trust Boundary](../overlays-in-depth/validation-security-and-product-architecture.md#trust-boundary)). Including base and overlays in one signed FIT configuration authenticates the declared composition; manually loading a `.dtbo` after that verification creates a new trust edge that needs separate enforcement.

## Constrain Mutation Scope

For optional hardware, a policy engine can parse an authenticated manifest such as:

```text
product=axc200
board_revision=B
release_set=42
base=axc200-revb.dtb
overlays=pcie-radio.dtbo,front-panel.dtbo
overlay_order=pcie-radio,front-panel
security_version=9
```

The values must be cryptographically bound to the release or derived from authenticated device state. The bootloader should map policy identifiers to known image nodes, not concatenate untrusted filenames or arbitrary FDT paths.

## Validate High-Risk Fixups

- **Memory and reserved memory:** apply the overflow, containment, overlap, alignment, and ownership checks in [Deliberate Failure Checks](../boot-time-mutation-and-ownership/ram-discovery-reservations-and-memory-fixups.md#deliberate-failure-checks).
- **Boot arguments:** prefer a signed allowlisted template plus narrow device-specific fields ([`bootargs`](../boot-time-mutation-and-ownership/chosen-bootargs-initrd-console-and-seeds.md#bootargs)); review parameters affecting root selection, integrity enforcement, IOMMU, debug, module loading, lockdown, console, and memory limits.
- **Identity and calibration:** check format and range before injection; the live Device Tree is visible to privileged software and is not a secret store.

## Integrity Is Not Confidentiality

Signing a DTB prevents unauthorized changes; it does not hide its contents. Avoid embedding long-lived secrets such as private keys, disk unlock keys, or credentials. Even ephemeral seed properties require careful lifecycle behavior: follow platform bindings, ensure consumers erase or sanitize sensitive seed material when designed to do so, and do not expose it unnecessarily in field bundles.

## Capture The Final Handoff

Useful strategies include:

- hash the packaged base and every overlay
- record signed configuration, slot, board identity, and ordered overlay list
- log each approved fixup class without leaking secret values
- measure the final FDT after all security-relevant mutations
- expose the original boot FDT where platform policy permits
- compare runtime identity with the signed release manifest

If the final tree cannot be reproduced byte-for-byte because of ephemeral data, produce a canonical semantic report with sensitive fields redacted and separately prove the immutable release inputs.

## Mutation Review Example

```text
mutation: set /memory reg
code: authenticated DRAM-discovery firmware + verified U-Boot fixup
input: firmware memory-map interface v3
validation: allowed banks, address bounds, overlap checks
authorization: platform firmware trust chain
reproducible: with recorded memory-map response
measured: final FDT in PCR policy
evidence: release set, map digest, final FDT digest
failure: authenticated recovery; never guess maximum RAM
```

## Unsafe Patterns

- verify base DTB, then load arbitrary overlay from writable media
- apply a valid overlay to an untested base because symbols resolve
- accept board identity solely from writable EEPROM without authentication or constraints
- let an unrestricted boot script edit `/chosen/bootargs`
- inject protected-memory coordinates without overlap validation
- hash only the packaged DTB while attesting that the runtime hardware policy is known
- store provisioning secrets as ordinary DT properties
- expose interactive `fdt` mutation commands in a production escape path

## Further Reading

- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/stable/usage/fdt_overlays.html)
- [U-Boot FIT Overlay Usage](https://docs.u-boot.org/en/stable/usage/fit/overlay-fdt-boot.html)
- [Overlays In Depth](../overlays-in-depth.md)
- [Measured Boot, Attestation, And Runtime Evidence](measured-boot-attestation-and-runtime-evidence.md)
