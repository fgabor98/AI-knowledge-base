---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Field Updates, Recovery, And Key Rotation

A production update design must survive hostile inputs, power loss, storage errors, incompatible Device Trees, unsuccessful health checks, expired keys, and operational mistakes. Verified boot answers what may run; transactional update policy decides what becomes active and how the device remains recoverable.

## Define The Update State Machine

Use explicit durable states rather than loosely coordinated flags:

```text
STABLE_A
  -> WRITING_B
  -> VERIFIED_B
  -> TRIAL_B(attempts=N)
  -> ACCEPTED_B
  -> STABLE_B

TRIAL_B failure/exhaustion -> STABLE_A
unrecoverable slot failure -> VERIFIED_RECOVERY
```

Every transition must specify:

- atomic persistent write or journal behavior
- power-loss result
- authentication and hash checks
- attempt counter ownership
- watchdog behavior
- health criteria
- rollback target
- relationship to protected security-version state

## Write Inactive, Verify, Then Select

A safe high-level sequence is:

1. Authenticate update metadata before trusting offsets, sizes, versions, or board scope.
2. Confirm product identity, hardware revision, dependency, storage, and security-floor constraints.
3. Write only the inactive slot and update staging area.
4. Read back and hash every artifact, including DTB/DTBO and manifest.
5. Verify the signed configuration using production-equivalent policy.
6. Atomically mark the candidate as trial with a bounded attempt count.
7. Reboot into the complete candidate release set.
8. Run hardware-critical health checks.
9. Commit the slot only after success.
10. Advance any irreversible rollback floor at its separately reviewed commit point.

Never overwrite the only verified recovery route before the candidate proves itself.

## Make Slot Selection Trustworthy

Slot metadata can become an authorization bypass if an attacker can redirect component paths. Constrain it to a small authenticated or replay-protected state such as:

```text
active=A|B
trial=A|B|none
attempts_remaining=0..N
accepted_release_set=<digest or ID>
```

The selector should choose a signed configuration representing a complete slot. It should not accept mutable filenames for kernel, DTB, and initramfs independently.

## Design Power-Loss Tests

Cut power at every durable step:

- update download
- metadata verification
- erase and each write block
- read-back verification
- trial-state commit
- boot-attempt decrement
- application health acknowledgement
- accepted-state commit
- security-floor advancement

After each cut, the device must boot a verified accepted slot, continue a safely resumable update, or enter verified recovery. “Power loss unlikely here” is not a production guarantee.

## Make Health DT-Aware

A candidate can reach a login prompt while essential DT-described hardware is broken. Verify at least:

- expected root model/compatible and release-set identity
- required devices bound to expected drivers
- boot storage and integrity root
- network/control plane required for recovery
- watchdog handoff and reboot-reason recording
- thermal sensors, cooling, power supplies, and regulators
- board-option peripherals selected by overlays
- absence of persistent probe deferrals for critical consumers
- subsystem-level operations, not only sysfs presence

Collect the evidence bundle before marking the release accepted.

## Recovery Is A Product Mode

A secure recovery path needs:

- immutable or separately protected entry policy
- authenticated recovery image and DTB
- compatibility across supported hardware and security floors
- minimal drivers required to diagnose and rewrite slots
- restricted interfaces and credentials
- rate limits or physical-presence rules where appropriate
- logging of recovery reason and actions
- a tested way back to a normal signed release

Do not solve recovery by enabling arbitrary U-Boot commands, unsigned USB boot, or a universal DTB with broad debug hardware enabled.

## Plan Key Rotation Before It Is Needed

A rotation normally requires overlap:

```text
R0: devices trust old key K1
R1: authenticated verifier update adds K2 while retaining K1
R2: releases are accepted under controlled K1/K2 policy
R3: fleet evidence confirms K2-capable verifier deployment
R4: releases use K2; policy revokes or stops requiring K1
```

The exact order depends on whether policy requires any or all keys, how the trust store is protected, and whether verifier updates are atomic. Test all intermediate states and rollback paths.

Separate keys by purpose:

- immutable/offline root
- bootloader or trust-store update
- normal production release
- recovery release
- factory/provisioning
- development

A development key should never be accepted by production policy. A recovery key should not silently authorize a normal feature-rich image.

## Respond To Key Compromise

Predefine:

1. detection and signing freeze
2. affected key, products, artifact classes, and release window
3. trustworthy inventory of devices and accepted verifier versions
4. emergency trust-store or security-floor update
5. recovery for devices unable to consume the normal rotation path
6. monitoring for compromised-key signatures
7. post-incident manifest and provenance audit

Revocation that bricks devices is not a successful response. Neither is retaining a known-compromised key indefinitely because recovery was never designed.

## Anti-Rollback Transaction

Treat the security floor as scarce, protected, often irreversible state. Before raising it, prove:

- candidate and recovery images meet or exceed the new floor
- both boot under the installed verifier and hardware variants
- candidate health is accepted
- recovery entry works
- power failure cannot leave a floor higher than every bootable image
- service tooling understands the new state

A signed version property stored only in a mutable DTB cannot protect itself. The comparison minimum must reside in protected monotonic state or equivalent platform policy.

## Fleet Rollout Controls

Use staged cohorts and gates:

- representative board revisions and optional overlays
- canary count and observation time
- boot failure, rollback, probe-defer, thermal, and watchdog metrics
- attested release-set distribution
- automatic rollout stop thresholds
- bandwidth and power constraints
- support bundle collection before retry

Fleet telemetry complements, but does not replace, on-device enforcement.

## Update Design Checklist

```text
[ ] signed metadata bounds every write and component relationship
[ ] complete release set is slot-local or atomically selected
[ ] inactive slot is read-back verified
[ ] trial attempts are durable and bounded
[ ] health check covers DT-described critical hardware
[ ] accepted-state write is atomic
[ ] security floor advances only after recoverability proof
[ ] recovery is authenticated, restricted, and compatible
[ ] key rotation intermediate states are tested
[ ] power-cut matrix covers every durable transition
[ ] field evidence identifies manifest, slot, signer, and final DT state
```

## Further Reading

- [U-Boot Verified Boot](https://docs.u-boot.org/en/stable/usage/fit/verified-boot.html)
- [U-Boot UEFI firmware update and anti-rollback](https://docs.u-boot.org/en/stable/develop/uefi/uefi.html)
- [Versioned Release Sets, Compatibility, And Rollback](versioned-release-sets-compatibility-and-rollback.md)
- [Secure Device Tree Release And Update Lab](secure-device-tree-release-and-update-lab.md)
