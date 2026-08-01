---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Secure Device Tree Release And Update Lab

This lab reviews and diagnoses a production update that is cryptographically valid but compositionally unsafe. You will reconstruct the trust chain, prove why the wrong DTB was selected, separate measurement from authorization, repair the release design, and preserve recovery before changing anti-rollback state.

## Scenario

AXC200 devices have two board revisions:

- revB uses capture DMA stream ID `0x31`
- revC uses capture DMA stream ID `0x47` and a different thermal trip table

Release 41 supports both revisions. Release 42 updates the kernel, modules, root filesystem, and both DTBs. It also fixes a remotely exploitable service and is intended to raise the security version from 8 to 9 after acceptance.

The boot medium uses A/B root filesystems, but the kernel FIT is global. Its FIT contains independently signed kernel and DTB image nodes. A mutable U-Boot environment variable `board_dtb` chooses the DTB subimage. The update writes slot B, changes `board_dtb`, and raises the minimum security version before the first trial boot.

Several revC devices then:

1. verify every selected FIT image successfully
2. boot the release-42 kernel and slot-B root filesystem
3. report `axc200-revb` as the live root compatible
4. bind the capture driver but fail DMA transfers
5. overheat under load because revB trip points were selected
6. fail the health check and attempt rollback
7. reject release 41 because the protected floor is already 9
8. enter a service shell that permits unsigned raw DTB boot

Measured-boot telemetry contains kernel, initramfs, and bootargs events, but no DTB or final-FDT event. The support portal displays “secure boot passed” and “PCRs valid.”

## Learning Objectives

You will:

- build an asset/threat/trust-boundary table
- distinguish authentic subimages from an authenticated configuration
- identify mutable selection and recovery bypasses
- calculate and verify artifact identities from a manifest
- prove the runtime DTB is wrong for the unit
- explain why the measurement evidence cannot identify that mismatch
- design a signed release configuration and slot-local release tuple
- create negative verification and power-cut tests
- define a safe anti-rollback commit point
- design a two-release key rotation without losing recovery

## Provided Evidence

### Device identity

```text
unit_id=AXC2-C-004812
otp_product=AXC200
otp_board_revision=C
accepted_release_set=axc200-prod-41
protected_min_security_version=9
active_rootfs=B
trial_attempts_remaining=0
```

### Release manifest excerpt

```yaml
manifest_version: 1
release_set: axc200-prod-42
security_version: 9
kernel:
  file: Image-6.12.18-axc2
  sha256: 1111111111111111111111111111111111111111111111111111111111111111
dtbs:
  revB:
    file: axc200-revb.dtb
    sha256: bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
  revC:
    file: axc200-revc.dtb
    sha256: cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
compatibility:
  revB: conf-revb-r42
  revC: conf-revc-r42
```

### FIT design excerpt

```dts
/dts-v1/;

/ {
    images {
        kernel-42 {
            data = /incbin/("Image-6.12.18-axc2");
            type = "kernel";
            arch = "arm64";
            os = "linux";
            compression = "none";
            signature-1 {
                algo = "sha256,rsa2048";
                key-name-hint = "prod-k1";
            };
        };

        fdt-revb-42 {
            data = /incbin/("axc200-revb.dtb");
            type = "flat_dt";
            arch = "arm64";
            signature-1 {
                algo = "sha256,rsa2048";
                key-name-hint = "prod-k1";
            };
        };

        fdt-revc-42 {
            data = /incbin/("axc200-revc.dtb");
            type = "flat_dt";
            arch = "arm64";
            signature-1 {
                algo = "sha256,rsa2048";
                key-name-hint = "prod-k1";
            };
        };
    };
};
```

### Boot script excerpt

```text
setenv board_dtb fdt-revb-42
bootm ${fit_addr}:kernel-42 - ${fit_addr}:${board_dtb}
```

The update service wrote `board_dtb=fdt-revb-42` for every device because its board-revision parser treated ASCII `C` as an unknown value and fell back to revB.

### Runtime evidence

```bash
tr '\0' '\n' </sys/firmware/devicetree/base/compatible
```

```text
acme,axc200-revb
acme,axc200
```

```bash
sha256sum /sys/firmware/fdt
```

```text
bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb  /sys/firmware/fdt
```

For this exercise, the platform exposes the unmodified boot FDT and the digest matches the release manifest's revB DTB exactly.

### Kernel log excerpt

```text
axc-capture 4a100000.capture: OF compatible acme,axc-capture-v3
axc-capture 4a100000.capture: using IOMMU stream ID 0x31
axc-capture 4a100000.capture: DMA transaction timed out
axc-thermal 4a200000.thermal: critical trip 105000 mC
system-health: capture self-test failed
system-health: candidate release rejected
boot-policy: release 41 security version 8 below protected minimum 9
```

### Measurement summary

```text
PCR[4] replay: PASS
events:
  - bootloader release 7.3.1
  - kernel Image-6.12.18-axc2
  - initramfs release 42
  - bootargs root=slotB ro quiet
DT measurement event: absent
final FDT measurement event: absent
quote nonce verification: PASS
```

## Part 1: Threat Model

Complete this table:

| Asset/goal | Threat or failure | Existing control | Gap | Consequence |
|---|---|---|---|---|
| board-specific isolation | wrong stream ID | signed DTB image | selection not authenticated/bound to OTP identity | DMA failure or incorrect isolation |
| thermal safety | wrong trip table | add your answer | add your answer | add your answer |
| recovery integrity | unsigned service boot | add your answer | add your answer | add your answer |
| rollback protection | vulnerable release 41 | protected floor | add your answer | add your answer |
| availability | failed release 42 | A/B rootfs | add your answer | add your answer |

Then draw the actual trust chain and mark these inputs:

- OTP board revision
- update-service parser
- mutable `board_dtb`
- separately signed subimages
- protected security floor
- service-shell boot path
- measured event list

## Part 2: Prove The Failure Stage

Answer with evidence:

1. Was the selected revB DTB corrupted?
2. Was it signed by an accepted key?
3. Was it authorized for this revC unit and release composition?
4. Did Linux receive revB or revC data?
5. Did the capture driver match and bind?
6. Why does successful binding not prove functional or security correctness?
7. What is the earliest incorrect decision?
8. Which later symptoms are consequences rather than root causes?

Build a causal chain in this form:

```text
board-revision parser
  -> selection state
  -> verified object
  -> Linux live identity
  -> IOMMU/thermal configuration
  -> functional and safety failure
  -> health rejection
  -> rollback outcome
  -> recovery policy
```

## Part 3: Audit The Cryptographic Coverage

For each item, mark `covered`, `not covered`, or `covered but not policy-bound`:

| Item | Classification | Reason |
|---|---|---|
| bytes of revB DTB image |  |  |
| bytes of revC DTB image |  |  |
| kernel bytes |  |  |
| relationship kernel-42 + revC DTB |  |  |
| OTP revision C -> revC configuration rule |  |  |
| value of mutable `board_dtb` |  |  |
| minimum security version |  |  |
| service-shell unsigned raw boot |  |  |

Explain the difference between:

- a valid signature on `fdt-revb-42`
- authorization to boot `fdt-revb-42` on an AXC200 revC
- a signed configuration that binds the kernel and revC DTB
- platform policy that maps authenticated OTP revision C to that configuration

## Part 4: Redesign The FIT And Selector

Sketch two signed configuration nodes:

```dts
configurations {
    default = "conf-revb-r42";

    conf-revb-r42 {
        /* kernel, fdt, ramdisk, signature */
    };

    conf-revc-r42 {
        /* kernel, fdt, ramdisk, signature */
    };
};
```

Your design must:

- use hashes for referenced images and signatures over configurations
- bind kernel, correct DTB, and initramfs
- require a production key anchored in authenticated U-Boot control state
- map OTP revision through a small allowlist to a configuration ID
- reject unknown revision rather than defaulting to revB
- prevent raw external DTB and legacy-image fallback
- log selected configuration, board identity, key ID, and release set

State whether the root filesystem or its integrity root must also be bound and how your platform does so.

## Part 5: Repair Measurement Policy

Design an event sequence that covers:

1. verifier and boot-policy identity
2. selected signed configuration/release set
3. kernel, DTB, initramfs, and root integrity identity
4. OTP-derived board class without leaking unnecessary unique identity
5. security version and slot
6. boot arguments
7. final FDT after approved fixups
8. recovery/debug mode

Answer:

- Why did PCR replay pass in the incident?
- Why was that insufficient to identify the wrong DTB?
- Would adding only the revB DTB digest make the boot authorized?
- Which events are stable across the fleet and which vary per unit/boot?
- How will the remote verifier distinguish known, authorized, current, and healthy?

## Part 6: Design Safe Update And Rollback

Write a state transition table:

| From | Operation | Durable state | Power-loss result | Next validation |
|---|---|---|---|---|
| stable A/41 | write inactive B/42 set |  |  |  |
| writing B | read-back verify |  |  |  |
| verified B | select trial |  |  |  |
| trial B | health fails |  |  |  |
| trial B | health passes |  |  |  |
| accepted B | raise floor 8 -> 9 |  |  |  |

The update must keep kernel, DTB, initramfs, modules, and rootfs aligned. Decide whether to use per-slot FITs, one signed container with immutable configurations, or another atomic scheme.

Define health tests for:

- live root compatible matches OTP board class
- expected release manifest digest
- capture DMA transaction
- active IOMMU stream mapping
- thermal trip points and sensor plausibility
- watchdog and network recovery path
- critical deferred-probe list

Explain why the floor must not rise before both candidate acceptance and floor-compatible recovery are proven.

## Part 7: Recovery And Key Rotation

Replace the unsigned service-shell path with a verified recovery design. Specify:

- recovery key and how it is scoped
- minimal recovery kernel/DTB hardware support
- security version compatibility
- physical presence or authenticated remote authorization
- commands deliberately unavailable
- evidence recorded
- how repaired normal slots are reverified

Then plan rotation from production key K1 to K2:

| Release | Trust store | Accepted signer policy | Normal release signer | Recovery signer |
|---|---|---|---|---|
| R42 | K1 |  |  |  |
| R43 transition | K1 + K2 |  |  |  |
| R44 adoption | K1 + K2 |  |  |  |
| R45 retirement | K2 |  |  |  |

State the fleet evidence required before K1 retirement and how a device stuck on R42 reaches a K2-capable verifier safely.

## Part 8: Negative And Power-Cut Tests

Create expected outcomes for:

- unsigned FIT
- unknown signing key
- changed DTB byte
- valid revB configuration on revC unit
- modified configuration referencing another valid DTB
- external unsigned overlay
- security version below protected floor
- corrupt slot metadata
- unsigned recovery image
- power cut during DTB write
- power cut after trial selection
- power cut before health acknowledgement
- power cut while persisting accepted state
- power cut before/after floor advancement

Every case must end in a verified accepted slot, a retryable safe state, or verified recovery.

## Deliverables

Produce:

1. threat model and trust-chain diagram
2. evidence-backed causal timeline
3. cryptographic coverage table
4. corrected FIT/selector design
5. measurement and remote-approval policy
6. A/B transaction and anti-rollback commit rule
7. recovery and K1-to-K2 rotation plan
8. negative and power-cut test matrix
9. incident containment and fleet remediation sequence

## Reference Analysis

The revB DTB is neither corrupted nor unsigned. Its digest matches the manifest and its image signature verifies. The failure is **authenticated but unauthorized composition**: a buggy parser writes a mutable selector that chooses a legitimate revB artifact for a revC unit. Individual image signatures do not bind the kernel/DTB relationship or the board-to-configuration policy.

Linux evidence conclusively identifies the revB handoff: root compatible and `/sys/firmware/fdt` digest agree with the revB manifest entry. The driver binds because the compatible is valid; it then programs stream ID `0x31`, so functional DMA failure and incorrect thermal policy follow from the earlier selection error.

PCR replay passes because the recorded events were faithfully measured. The DTB and final FDT were absent, so the quote cannot distinguish revB from revC. Even measuring revB would make the wrong choice visible, not authorized. Remote policy must connect measured configuration, authenticated OTP-derived board class, release set, security floor, and operational mode.

The availability failure is independent and severe: the protected floor advanced before the trial was healthy, so the accepted release-41 fallback became ineligible. The safe order is write/read-back verify, trial boot, board-specific health, accept, prove floor-compatible recovery, then advance the irreversible floor under a separate transaction.

The service shell converts an update defect into a secure-boot bypass. Replace it with a signed, restricted recovery configuration. Remediation should revoke the broken update, preserve incident evidence, ship a verifier/update-policy correction through an already trusted path, restore a compatible verified recovery route, and only then reattempt release 42 with signed configurations and fixed board selection.

## Further Reading

- [FIT Authenticated Selection And Key Policy](fit-authenticated-selection-and-key-policy.md)
- [Measured Boot, Attestation, And Runtime Evidence](measured-boot-attestation-and-runtime-evidence.md)
- [Versioned Release Sets, Compatibility, And Rollback](versioned-release-sets-compatibility-and-rollback.md)
- [Field Updates, Recovery, And Key Rotation](field-updates-recovery-and-key-rotation.md)
