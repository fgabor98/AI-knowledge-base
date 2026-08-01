---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Security And Production Lifecycle

A production Device Tree is executable hardware policy supplied across a trust boundary. It controls which devices Linux exposes, which addresses it touches, which DMA relationships it accepts, which reserved memory it excludes, and which boot arguments it consumes. Treating a DTB as harmless configuration leaves a verified kernel dependent on an unauthenticated hardware description.

This module builds a defensible lifecycle from threat model through authenticated selection, measured evidence, reproducible release artifacts, field update, rollback, recovery, and key rotation.

## Learning Outcomes

After completing this module, you should be able to:

- identify the security consequences of malicious or stale DT nodes and properties
- draw the trust chain from immutable root key to bootloader, selected configuration, kernel, DTB, overlays, initramfs, and root filesystem
- distinguish a digest, signature, authorization policy, measurement, and anti-rollback state
- explain why signing individual FIT images does not necessarily authenticate their combination or selection
- design signed FIT configurations that bind a kernel to the intended DTB, overlays, and other boot payloads
- inventory every mutation after verification and decide whether it is deterministic, authorized, measured, and auditable
- reason about bootloader fixups, `/chosen`, secrets, firmware-generated data, and runtime overlays without confusing integrity with confidentiality
- use TPM PCRs and an event log as evidence while recognizing that measurement does not block unauthorized boot
- define a compatible release set across boot firmware, kernel, DTB/DTBO, modules, device firmware, and root filesystem
- separate functional rollback from security anti-rollback and preserve a signed recovery path
- produce byte-reproducible DT artifacts and a release manifest that traces inputs, tools, configuration, transformations, and outputs
- design A/B updates, acceptance criteria, boot-attempt accounting, power-loss behavior, and key rotation without creating an unbootable fleet
- diagnose whether a field failure is caused by corruption, unauthorized substitution, wrong selection, post-verification mutation, incompatibility, or rollback policy

## Prerequisites

Complete [Runtime Inspection](runtime-inspection.md), [Overlays In Depth](overlays-in-depth.md), and [Build And Diagnostic Tools](build-and-diagnostic-tools.md). You should already be able to prove artifact identity at each boot checkpoint and distinguish the boot FDT from the live Linux tree.

## Learning Path

1. [Device Tree Threat Model And Trust Boundaries](security-and-production-lifecycle/device-tree-threat-model-and-trust-boundaries.md)
2. [FIT Authenticated Selection And Key Policy](security-and-production-lifecycle/fit-authenticated-selection-and-key-policy.md)
3. [Mutation, Overlay, And Fixup Chain Of Custody](security-and-production-lifecycle/mutation-overlay-and-fixup-chain-of-custody.md)
4. [Measured Boot, Attestation, And Runtime Evidence](security-and-production-lifecycle/measured-boot-attestation-and-runtime-evidence.md)
5. [Versioned Release Sets, Compatibility, And Rollback](security-and-production-lifecycle/versioned-release-sets-compatibility-and-rollback.md)
6. [Reproducible DTB Builds, Provenance, And Manifests](security-and-production-lifecycle/reproducible-dtb-builds-provenance-and-manifests.md)
7. [Field Updates, Recovery, And Key Rotation](security-and-production-lifecycle/field-updates-recovery-and-key-rotation.md)
8. [Secure Device Tree Release And Update Lab](security-and-production-lifecycle/secure-device-tree-release-and-update-lab.md)

## Security Invariants

```text
immutable trust anchor
  -> authentic verifier
  -> authenticated configuration selection
  -> authenticated kernel + DTB + overlays + initramfs
  -> controlled and recorded mutations
  -> measured final handoff where required
  -> compatible release-set activation
  -> bounded rollback and signed recovery
  -> field evidence tied to a release manifest
```

Every arrow needs an enforcement mechanism and a test. “Secure boot enabled” is not evidence that the DTB, its selection, or later mutations are covered.

## Evidence Matrix

| Evidence | Establishes | Does not establish |
|---|---|---|
| SHA-256 digest | exact bytes match a known digest | who authorized those bytes |
| valid signature | authorized signer approved covered bytes | freshness or device suitability |
| signed FIT image | one payload is authentic | configuration selection is authentic |
| signed FIT configuration | covered payload set and relationship are authentic | anti-rollback policy was enforced |
| TPM PCR value | sequence-dependent measurement state | which events occurred without a trustworthy log |
| event log replay matches PCR | log explains that PCR | measured payload was authorized |
| reproducible DTB | independent builds can yield same bytes | source itself is trustworthy |
| release manifest | declared component identities and relationships | device actually booted that set |
| A/B rollback | availability after a bad update | protection from an old vulnerable release |

## Production Review Questions

For every delivered DTB or DTBO, answer:

1. Who can replace it at rest, in transit, in the boot environment, or in memory?
2. Which immutable or previously authenticated key authorizes it?
3. Is the selected combination authenticated, not just each component?
4. Which code may mutate the verified tree, and from which inputs?
5. Is the final handoff measured or otherwise reconstructable?
6. Which kernel, modules, firmware, and root filesystem are compatible with it?
7. What monotonic state rejects vulnerable versions?
8. What happens after power loss at every update step?
9. How is a broken release recovered without bypassing verification?
10. Which manifest and field evidence prove what actually ran?

## Completion Check

You are ready for [Product-Scale Maintenance And Engineering](product-scale-maintenance-and-engineering.md) when you can:

- produce a DT-specific threat model with assets, actors, entry points, and consequences
- identify a FIT design vulnerable to mix-and-match selection even though every payload has a valid signature
- show where the trusted public key lives and why a key stored only beside the signed image is not a trust anchor
- classify every boot-time tree mutation as trusted input, policy decision, measured output, or security gap
- explain verified boot, measured boot, and anti-rollback without treating them as substitutes
- define a release compatibility tuple and rollback rules that preserve both security and recoverability
- rebuild a DTB independently and explain any byte difference from recorded inputs
- audit a release manifest against packaged and runtime evidence
- design a power-loss-safe A/B activation and a recoverable signing-key rotation
- diagnose the lab incident without disabling verification or forcing an incompatible DTB

## Authoritative References

- [U-Boot Verified Boot](https://docs.u-boot.org/en/stable/usage/fit/verified-boot.html)
- [U-Boot FIT Signature Verification](https://docs.u-boot.org/en/stable/usage/fit/signature.html)
- [U-Boot Measured Boot](https://docs.u-boot.org/en/stable/usage/measured_boot.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/stable/usage/fdt_overlays.html)
- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)
- [SOURCE_DATE_EPOCH specification](https://reproducible-builds.org/specs/source-date-epoch/)

## Related Topics

- [U-Boot And Bootloader Device Tree](u-boot-and-bootloader-device-tree.md)
- [Binding Design And Stable ABI](binding-design-and-stable-abi.md)
- [Overlays In Depth](overlays-in-depth.md)
- [Build And Diagnostic Tools](build-and-diagnostic-tools.md)
- [Runtime Inspection](runtime-inspection.md)
- [Product-Scale Maintenance And Engineering](product-scale-maintenance-and-engineering.md)
