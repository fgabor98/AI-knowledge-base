---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Measured Boot, Attestation, And Runtime Evidence

Verified boot decides whether an artifact is authorized before use. Measured boot records what was used so local or remote policy can evaluate it. A secure product may need both: measurement alone observes an unauthorized boot, while verification alone may provide insufficient evidence about selection and post-verification fixups.

## Understand PCR Extension

A TPM Platform Configuration Register is normally extended, not assigned:

```text
PCR_new = HASH(PCR_old || event_digest)
```

The result commits to ordered measurements. It does not reveal the sequence. An event log records event types, algorithms, digests, and descriptions so a verifier can replay extensions and compare the reconstructed PCR with a quoted value.

Consequences:

- the same events in a different order produce a different PCR
- PCR equality can represent a known boot sequence
- a log that does not replay to the quoted PCR is not trustworthy evidence
- a matching log may still describe unauthorized but faithfully measured software

## Decide What To Measure

For DT-aware boot, consider:

- boot firmware stages and verifier configuration
- selected FIT configuration
- kernel, base DTB, each overlay, and initramfs
- security version and slot selection
- boot arguments
- final working FDT after security-relevant fixups
- recovery path and debug state
- root filesystem integrity root or policy

U-Boot measured-boot support can measure the OS image, initrd, and boot arguments, and can include the Device Tree when configured. Confirm the behavior in the exact U-Boot version and platform integration.

## Measure Components Or Final State

There are two useful designs:

### Component ledger

Measure signed base, ordered overlays, and each fixup input. This preserves causality but requires the verifier to reproduce transformation semantics.

### Final handoff

Measure the completed FDT after mutations. This directly commits to kernel input but may vary with DRAM discovery, seeds, addresses, or device identity.

Many systems use both: stable component/release events for fleet policy and a final FDT event for forensic completeness.

## Treat The Event Log As Untrusted Until Replay

The event log lives in ordinary memory and is handed to the OS, often using Device Tree-described reserved memory or standard platform interfaces. An attacker may alter a log unless the PCR quote anchors it. Validation order is:

1. authenticate the attestation key and nonce-bound quote
2. verify the quote signature
3. replay the event log with the correct algorithms and ordering
4. compare replayed PCR values with the quote
5. evaluate events against approved release and device policy

Do not approve a device merely because a text log contains expected filenames or hashes.

## Inspect Linux Evidence

Platform paths vary. On TPM-enabled Linux systems, begin with:

```bash
ls -l /sys/class/tpm /sys/kernel/security 2>/dev/null
find /sys/kernel/security -maxdepth 3 -type f \
  \( -iname '*event*' -o -iname '*measurement*' \) 2>/dev/null

dmesg --color=never | grep -Ei 'tpm|event log|measurement|secure boot'
```

When `securityfs` is mounted and the platform exposes a binary or ASCII event log, copy it without transformation:

```bash
sudo cp /sys/kernel/security/tpm0/binary_bios_measurements ./eventlog.bin
sha256sum ./eventlog.bin
```

Paths and supported tools depend on firmware, architecture, kernel configuration, and TPM stack. Preserve raw evidence before parsing.

## Build An Approval Policy

Avoid a single static PCR allowlist for every unit when legitimate dynamic data changes measurements. A verifier can evaluate event semantics:

- signer/key identity is allowed
- release set and security version are allowed for this product
- base DTB matches board identity
- overlay set is an authorized ordered subset
- mutable boot arguments conform to policy
- final FDT measurement is consistent with attested transformation inputs
- debug/recovery events meet the requested operational mode

Separate “known,” “authorized,” “current,” and “healthy.” They are different judgments.

## Freshness And Replay Resistance

Remote attestation should bind a fresh verifier nonce into the TPM quote. Otherwise an attacker may replay an old valid quote and log. Also bind evidence to device identity and expected attestation key enrollment.

Measurement does not create rollback resistance. A correctly quoted old release still needs a policy decision or protected minimum security version.

## Protect Privacy

Final FDTs and event logs can reveal:

- product and board identity
- serial numbers and MAC addresses
- memory layout
- enabled peripherals
- command-line parameters
- firmware versions
- possibly sensitive provisioning or debug state

Minimize collected data, apply access control and retention, redact only after preserving a forensically sound protected original, and never place secrets in the tree merely because attestation transport is encrypted.

## Diagnose A Measurement Mismatch

Use this order:

1. Confirm device, attestation key, nonce, PCR bank, and quote signature.
2. Preserve quote, raw event log, boot logs, release manifest, and runtime FDT.
3. Replay the log; distinguish replay failure from policy failure.
4. Find the first event digest different from the approved sequence.
5. Map it to base DTB, overlay, bootargs, slot, or final fixup state.
6. Compare pre-verification and final-handoff evidence.
7. Determine whether the cause is expected device variability, wrong selection, unauthorized mutation, stale release, or parser/tool error.

## Measurement Plan Template

```text
PCR/bank:
event order:
artifact or state:
measurement point (before/after mutation):
event type and description contract:
expected variability:
release-manifest mapping:
quote nonce and identity binding:
replay tool/version:
approval rule:
retention/privacy:
failure response:
```

## Further Reading

- [U-Boot Measured Boot](https://docs.u-boot.org/en/stable/usage/measured_boot.html)
- [Linux TPM documentation](https://docs.kernel.org/security/tpm/index.html)
- [Runtime Inspection](../runtime-inspection.md)
- [Versioned Release Sets, Compatibility, And Rollback](versioned-release-sets-compatibility-and-rollback.md)
