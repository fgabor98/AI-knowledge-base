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

Record the boot pipeline as explicit states:

```text
D0  packaged base DTB
D1  selected authenticated base
D2  authenticated overlays applied in declared order
D3  board identity and memory fixups
D4  /chosen boot arguments and initrd bounds
D5  reserved-memory / firmware handoff fixups
D6  random seed and measurement-log metadata
D7  final FDT passed to Linux
D8  Linux live tree after any runtime overlays
```

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

### Deterministic release transformations

Compile-time includes and overlays built from versioned source can be reproducible and authenticated as release inputs.

### Device-identity transformations

Serial numbers, MAC addresses, calibration values, and board revisions often come from EEPROM, OTP, or a secure element. Validate structure, authenticity, uniqueness, product binding, and bounds. Do not assume physical proximity makes EEPROM data trusted.

### Hardware-discovery transformations

DRAM size, disabled cores, reserved firmware regions, and detected peripherals may be supplied by trusted firmware. Define the firmware interface and reject overlaps or impossible values before editing `memory`, `/reserved-memory`, or CPU nodes.

### Ephemeral transformations

Random seeds, initrd locations, and event-log addresses vary per boot. They can be legitimate without reproducible final DTB bytes. Record their origin and measure the relevant final handoff when attestation matters.

### Operator- or environment-driven transformations

Boot arguments, overlay lists, and addresses derived from mutable environment are high risk. Authenticate or constrain the policy rather than trusting arbitrary strings.

## Overlay Trust Rules

An overlay can rewrite security-relevant properties as effectively as a replacement base DTB. A production overlay policy should define:

- authorized overlay identities and signer
- compatible base release or stable overlay ABI
- allowed target nodes/properties
- permitted combinations and application order
- conflict and dependency rules
- board identity/feature entitlement requirements
- whether removal or runtime application is allowed
- final measurement and field reporting

Including base and overlays in one signed FIT configuration authenticates the declared composition. Manually loading a `.dtbo` after that verification creates a new trust edge that needs separate enforcement.

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

### Memory and reserved memory

Check:

- integer overflow and address-cell width
- containment within installed physical memory
- overlap among kernel, initrd, FDT, firmware, DMA pools, and protected regions
- alignment and architecture constraints
- whether a region must be `no-map`, reusable, or assigned to one owner

### Boot arguments

Prefer a signed allowlisted template plus narrow device-specific fields. Review parameters that affect root selection, integrity enforcement, IOMMU behavior, debug facilities, module loading, lockdown, console, and memory limits.

### Identity and calibration

Check format and range before injection. Treat secret provisioning material separately: the live Device Tree is commonly visible to privileged software and is not a secret store.

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
