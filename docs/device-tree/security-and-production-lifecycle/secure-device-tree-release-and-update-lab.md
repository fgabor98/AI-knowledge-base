---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Secure Device Tree Release And Update Lab

This lab reviews and diagnoses a production update that is cryptographically valid but compositionally unsafe. You will reconstruct the trust chain, prove why the wrong DTB was selected, separate measurement from authorization, repair the release design, and preserve recovery before changing anti-rollback state.

The hardware facts are real: two mainline Linux board files for the TI AM62x, `k3-am625-sk.dts` and `k3-am62-lp-sk.dts`, and the TI board-identity EEPROM. The fleet, release numbers, keys, and update service are a hypothetical product built on those two designs.

## Scenario

The product ships in two hardware classes:

- **SK class** uses the AM625 SK design and `k3-am625-sk.dtb`. Through `k3-am625-sk-common.dtsi` its OPP table adds `opp-1400000000` for T-grade parts, with the comment “Requires VDD_CORE to be at 0.85V.” The board's VDD_CORE regulator is not described in DT.
- **LP class** uses the AM62x LP SK design and `k3-am62-lp-sk.dtb`. Its TPS65219 PMIC (`pmic@30` on `main_i2c0`) fixes `buck1`, `VDD_CORE`, at 750000 µV, and it has no 1.4 GHz OPP.

Each unit's class is read from the TI board EEPROM (`AM62-SKEVM` or `AM62-LP-SKEVM`), which is CRC-protected but not authenticated.

Release 41 supports both classes. Release 42 updates the kernel, modules, root filesystem, and both DTBs. It also fixes a remotely exploitable service and is intended to raise the security version from 8 to 9 after acceptance.

The boot medium uses A/B root filesystems, but the kernel FIT is global. Its FIT contains independently signed kernel and DTB image nodes. A mutable U-Boot environment variable `board_dtb` chooses the DTB subimage. The update writes slot B, changes `board_dtb`, and raises the minimum security version before the first trial boot.

Several LP-class devices then:

1. verify every selected FIT image successfully
2. boot the release-42 kernel and slot-B root filesystem
3. report `ti,am625-sk` as the live root compatible
4. offer 1.4 GHz in cpufreq on T-grade parts and crash intermittently under load
5. never probe the PMIC, because `pmic@30` is not in the SK-class tree
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
unit_id=PRD-LP-004812
eeprom_board_name=AM62-LP-SKEVM
speed_grade=T
accepted_release_set=prod-41
protected_min_security_version=9
active_rootfs=B
trial_attempts_remaining=0
```

### Release manifest excerpt

```yaml
manifest_version: 1
release_set: prod-42
security_version: 9
kernel:
  file: Image
  sha256: <kernel digest>
dtbs:
  sk:
    file: ti/k3-am625-sk.dtb
    sha256: <SK-class digest>
  lp:
    file: ti/k3-am62-lp-sk.dtb
    sha256: <LP-class digest>
compatibility:
  sk: conf-sk-r42
  lp: conf-lp-r42
```

### FIT design excerpt

```dts
/dts-v1/;

/ {
    images {
        kernel-42 {
            data = /incbin/("Image");
            type = "kernel";
            arch = "arm64";
            os = "linux";
            compression = "none";
            signature-1 {
                algo = "sha256,rsa2048";
                key-name-hint = "prod-k1";
            };
        };

        fdt-sk-42 {
            data = /incbin/("k3-am625-sk.dtb");
            type = "flat_dt";
            arch = "arm64";
            signature-1 {
                algo = "sha256,rsa2048";
                key-name-hint = "prod-k1";
            };
        };

        fdt-lp-42 {
            data = /incbin/("k3-am62-lp-sk.dtb");
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
setenv board_dtb fdt-sk-42
bootm ${fit_addr}:kernel-42 - ${fit_addr}:${board_dtb}
```

The update service wrote `board_dtb=fdt-sk-42` for every device because its board-name parser recognized only the exact string `AM62-SKEVM` and mapped every other value, including `AM62-LP-SKEVM`, to the SK-class default.

### Runtime evidence

```bash
tr '\0' '\n' </sys/firmware/devicetree/base/compatible
cat /sys/firmware/devicetree/base/model; echo
ls /sys/firmware/devicetree/base/bus@f0000/i2c@20000000/
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies
```

```text
ti,am625-sk
ti,am625
Texas Instruments AM625 SK
... (no pmic@30)
200000 400000 600000 800000 1000000 1250000 1400000
```

`/sys/firmware/fdt` does not hash to the release DTB, because U-Boot's fixups (`/chosen`, MAC addresses, thermal trips, firmware reservations) modify it. Compare it semantically with both release DTBs: its model, compatible, OPP table, and the absence of `pmic@30` match the SK-class file.

### Kernel log excerpt

```text
Machine model: Texas Instruments AM625 SK
system-health: stress test at max OPP failed
system-health: candidate release rejected
boot-policy: release 41 security version 8 below protected minimum 9
```

### Measurement summary

```text
PCR[4] replay: PASS
events:
  - bootloader release 2026.01
  - kernel Image (release 42)
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
| electrical safety | 1.4 GHz OPP exposed with VDD_CORE at 0.75 V | signed DTB image | selection not authenticated or bound to board identity | CPU faults at the top OPP; possible silent data corruption |
| power management | PMIC not described | add your answer | add your answer | add your answer |
| recovery integrity | unsigned service boot | add your answer | add your answer | add your answer |
| rollback protection | vulnerable release 41 | protected floor | add your answer | add your answer |
| availability | failed release 42 | A/B rootfs | add your answer | add your answer |

Then draw the actual trust chain and mark these inputs:

- EEPROM board name (CRC-protected, unauthenticated)
- update-service parser
- mutable `board_dtb`
- separately signed subimages
- protected security floor
- service-shell boot path
- measured event list

## Part 2: Prove The Failure Stage

Answer with evidence:

1. Was the selected SK-class DTB corrupted?
2. Was it signed by an accepted key?
3. Was it authorized for this LP-class unit and release composition?
4. Did Linux receive SK-class or LP-class data?
5. Did cpufreq and the other drivers probe?
6. Why does successful binding not prove functional or security correctness?
7. What is the earliest incorrect decision?
8. Which later symptoms are consequences rather than root causes?

Build a causal chain in this form:

```text
board-revision parser
  -> selection state
  -> verified object
  -> Linux live identity
  -> OPP and regulator description
  -> functional and safety failure
  -> health rejection
  -> rollback outcome
  -> recovery policy
```

## Part 3: Audit The Cryptographic Coverage

For each item, mark `covered`, `not covered`, or `covered but not policy-bound`:

| Item | Classification | Reason |
|---|---|---|
| bytes of SK-class DTB image |  |  |
| bytes of LP-class DTB image |  |  |
| kernel bytes |  |  |
| relationship kernel-42 + LP-class DTB |  |  |
| board identity `AM62-LP-SKEVM` -> LP configuration rule |  |  |
| value of mutable `board_dtb` |  |  |
| minimum security version |  |  |
| service-shell unsigned raw boot |  |  |

Explain the difference between:

- a valid signature on `fdt-sk-42`
- authorization to boot `fdt-sk-42` on an LP-class unit
- a signed configuration that binds the kernel and LP-class DTB
- platform policy that maps an authenticated LP-class identity to that configuration

## Part 4: Redesign The FIT And Selector

Sketch two signed configuration nodes:

```dts
configurations {
    default = "conf-sk-r42";

    conf-sk-r42 {
        /* kernel, fdt, ramdisk, signature */
    };

    conf-lp-r42 {
        /* kernel, fdt, ramdisk, signature */
    };
};
```

Your design must:

- use hashes for referenced images and signatures over configurations
- bind kernel, correct DTB, and initramfs
- require a production key anchored in authenticated U-Boot control state
- map the board identity through an exact-match allowlist to a configuration ID, and decide how to authenticate that identity (the TI EEPROM has only a CRC)
- reject an unknown identity rather than defaulting to the SK class
- prevent raw external DTB and legacy-image fallback
- log selected configuration, board identity, key ID, and release set

State whether the root filesystem or its integrity root must also be bound and how your platform does so.

## Part 5: Repair Measurement Policy

Design an event sequence that covers:

1. verifier and boot-policy identity
2. selected signed configuration/release set
3. kernel, DTB, initramfs, and root integrity identity
4. authenticated board class without leaking unnecessary unique identity
5. security version and slot
6. boot arguments
7. final FDT after approved fixups
8. recovery/debug mode

Answer:

- Why did PCR replay pass in the incident?
- Why was that insufficient to identify the wrong DTB?
- Would adding only the SK-class DTB digest make the boot authorized?
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

- live root compatible matches the authenticated board class
- expected release manifest digest
- `pmic@30` probed and `VDD_CORE` reported by the regulator framework
- cpufreq offers no OPP above what the measured VDD_CORE supports
- sustained stress at the top offered OPP
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
- valid SK-class configuration on an LP-class unit
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

The SK-class DTB is neither corrupted nor unsigned. It matches the manifest, and its image signature verifies. The failure is **authenticated but unauthorized composition**: a buggy parser writes a mutable selector that chooses a legitimate SK-class artifact for an LP-class unit. Individual image signatures do not bind the kernel/DTB relationship or the board-to-configuration policy.

Linux evidence identifies the SK-class handoff: root compatible, model, OPP table, and the missing `pmic@30` match the SK-class file. Because U-Boot fixups change the blob, the comparison is semantic, not a digest match. Every driver binds normally, because each node is valid hardware description for *some* board. The danger is that `opp-1400000000` assumes a VDD_CORE of 0.85 V that this board does not supply, and nothing in either DTB lets software detect the mismatch.

PCR replay passes because the recorded events were faithfully measured. The DTB and final FDT were absent, so the quote cannot distinguish the two classes. Even measuring the SK-class DTB would make the wrong choice visible, not authorized. Remote policy must connect the measured configuration, an authenticated board class, the release set, the security floor, and the operational mode.

The availability failure is independent and severe: the protected floor advanced before the trial was healthy, so the accepted release-41 fallback became ineligible. The safe order is write/read-back verify, trial boot, board-specific health, accept, prove floor-compatible recovery, then advance the irreversible floor under a separate transaction.

The service shell converts an update defect into a secure-boot bypass. Replace it with a signed, restricted recovery configuration. Remediation should revoke the broken update, preserve incident evidence, ship a verifier/update-policy correction through an already trusted path, restore a compatible verified recovery route, and only then reattempt release 42 with signed configurations and fixed board selection.

## Further Reading

- [FIT Authenticated Selection And Key Policy](fit-authenticated-selection-and-key-policy.md)
- [Measured Boot, Attestation, And Runtime Evidence](measured-boot-attestation-and-runtime-evidence.md)
- [Versioned Release Sets, Compatibility, And Rollback](versioned-release-sets-compatibility-and-rollback.md)
- [Field Updates, Recovery, And Key Rotation](field-updates-recovery-and-key-rotation.md)
