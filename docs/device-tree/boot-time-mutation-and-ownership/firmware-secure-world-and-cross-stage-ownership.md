---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Firmware, Secure World, And Cross-Stage Ownership

ROM, trusted firmware, management processors, SPL, U-Boot, and Linux can all inspect or modify a DTB. Multiple stages are legitimate only when their responsibilities are disjoint or their precedence is explicit. A later normal-world stage must not overwrite secure-world ownership because its tree appears editable.

## Create A Writer Matrix

| Property class | Authoritative producer | Serializer/final writer |
|---|---|---|
| secure reserved memory | trusted firmware | trusted firmware or validated bootloader bridge |
| installed/trained RAM | memory-init authority | SPL/U-Boot |
| board identity | authenticated provisioning source | selection/board layer |
| boot slot and command line | verified boot policy | OS image preparation |
| initrd addresses | loader after placement | generic boot code |
| RNG seeds | trusted RNG owner | latest authorized handoff stage |
| firmware interface node | platform integration | base DTS, narrowly adjusted if negotiated |

Fill this for the actual platform. “Last writer wins” is not an ownership design.

## Firmware-Supplied DTBs

An earlier stage can supply the complete tree. The next stage should:

1. authenticate or establish trust in the supplying stage/tree
2. validate FDT structure and maximum size
3. validate compatible/platform identity
4. reserve/copy it into owned writable memory
5. inventory existing firmware mutations
6. apply only authorized later changes

Do not assume a firmware-origin tree has padding or lies in memory safe from later DMA/decompression.

When U-Boot uses a firmware-provided tree for its control FDT and Linux handoff, distinguish copies and mutation lifetimes. Modifying one shared blob after U-Boot device binding is unsafe.

## Trusted-Firmware Reservations

Secure world can own memory, devices, interrupt routing, clocks, or remote cores. Its DT contribution might disable normal-world devices, reserve memory, or expose a mediated firmware interface.

Normal-world validation should confirm:

- ranges are within platform-supported memory
- no integer overflow
- no contradictory normal-world consumer remains enabled
- secure and shared windows have correct attributes
- hardware firewalls match the description

It must not shrink or drop an authenticated secure reservation merely to recover RAM.

## Handoff Contracts

For each boundary—ROM→SPL, SPL→U-Boot, trusted firmware→normal world, U-Boot→Linux—define:

- pointer/address and maximum size
- ownership of the buffer
- authentication/integrity status
- whether mutation is allowed
- available capacity
- cache maintenance
- reservation until next consumer copies/unflattens it
- version/capability negotiation
- failure behavior

A raw pointer without a lifetime contract leads to use-after-overwrite across stages.

## Avoid Duplicate Discovery

If trusted firmware reports DRAM and secure reservations, U-Boot should not independently derive a conflicting map from registers it may not own. If SPL validated board identity, U-Boot should consume the handed-off normalized identity rather than reread an EEPROM through a different path unless revalidation is intentional.

Duplicate discovery can differ because:

- hardware state changed
- one stage sees a secure alias
- EEPROM read transiently failed
- parsers use different versions
- stage-specific defaults diverge

Prefer one authoritative discovery with a versioned handoff.

## Cache And Visibility

When a stage mutates an FDT for another CPU or execution world:

- complete all libfdt writes
- perform required cache clean/invalidate operations
- use architecture-required barriers
- prevent concurrent writers
- transfer buffer ownership

`fdt_pack()` does not make data coherent. A correct tree in one cache can appear corrupt to another agent.

## Warm Boot, Kexec, And Resume

Cold boot often hides stale ownership. Test:

- warm reset where DRAM retains an old FDT
- watchdog reset during mutation
- kexec with Linux-generated `/chosen` data
- secure firmware update changing reservation size
- suspend/resume where management firmware remains active

Every new boot must rebuild or validate checkpoints, clear one-time seeds, and remove stale initrd/overlay state. Do not trust RAM solely because its header magic remains.

## Measured And Verified Boot

If a stage verifies a base tree and a later trusted stage mutates it, attestation should describe the final state or authorized transformation sequence.

Possible measurements:

- base and ordered overlay hashes
- normalized firmware reservation manifest
- board identity/policy version
- hardware-composition tree before ephemeral secrets
- final redacted/canonical DTB

Never measure and publish secret seed bytes. Define how dynamic but legitimate hardware changes affect expected measurements.

## Failure Escalation

Fail closed for:

- invalid secure reservations
- incompatible firmware handoff version
- untrusted tree supplier
- conflicting device ownership
- buffer outside accessible/protected RAM
- mutation after final measurement without policy

Recovery itself must use a known-safe tree and ownership map. Do not fall back to an older base that re-enables secure-owned hardware.

## Authoritative References

- [U-Boot Devicetree Control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot bloblist](https://docs.u-boot.org/en/latest/develop/bloblist.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux OP-TEE binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/arm/firmware/linaro,optee-tz.yaml)
- [Linux arm64 booting requirements](https://docs.kernel.org/arch/arm64/booting.html)

## Continue

Proceed to [Final-Tree Validation, Diffing, And Runtime Forensics](final-tree-validation-diffing-and-runtime-forensics.md).
