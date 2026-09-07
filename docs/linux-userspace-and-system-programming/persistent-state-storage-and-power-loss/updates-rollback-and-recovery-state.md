---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Updates, Rollback, And Recovery State

## A/B and generation-based updates

A resilient update keeps a known-good bootable image while preparing a new one. The update manager writes the inactive slot, verifies its contents and metadata, then performs one atomic boot-selection transition. The bootloader should try the new slot only a bounded number of times and revert when userspace fails to report health.

The health signal must mean more than “the process started.” It should include filesystem availability, required services, essential hardware, configuration migration, and—where appropriate—a controlled functional check. Keep the old slot usable until the new slot has passed the acceptance window.

## Metadata contract

Boot selection metadata should include slot identity, image version, integrity/authenticity result, attempt count, confirmation state, and rollback reason. Protect it from partial writes with redundant records or a transactional mechanism. Validate all fields before acting; a corrupt attempt counter must not disable the recovery path.

## Recovery behavior

Define behavior for interrupted download, interrupted installation, power loss during metadata commit, failed migration, incompatible configuration, repeated watchdog resets, and a full data partition. Recovery must leave a diagnosable artifact and preserve enough logs to explain the decision. A rollback that restores binaries but leaves an incompatible persistent schema is not a complete rollback.

Treat factory reset and rescue mode as explicit, authenticated operations. They should not accidentally erase evidence or calibration data that the recovery procedure needs.

## A concrete update state machine

```text
RUNNING_A -> DOWNLOAD_B -> VERIFY_B -> INSTALL_B -> SELECT_TRIAL_B
       (fail before selection: continue A)
SELECT_TRIAL_B -> BOOT_B -> MIGRATE/VALIDATE -> CONFIRM_B
                      -> failure/attempt budget -> FALLBACK_A
```

Each arrow needs one owner and a durable record whose interrupted form has
a defined interpretation. Confirmation should happen only after the product's
required health checks. It must identify the slot actually booted, rather
than trusting which slot the downloader intended to select.
See [RAUC boot confirmation](https://rauc.readthedocs.io/en/latest/using.html).

## Rollback includes shared data

Suppose B migrates shared settings from schema 1 to schema 2 and then fails
its hardware health test. Booting A again works only if A can read the data
or a valid schema-1 generation remains. Options include backward-compatible
extensions, per-slot state with explicit synchronization, or delayed destructive
migration until rollback is no longer permitted. Select a policy before release.

Anti-rollback security policy can intentionally prevent booting an older signed
image. Coordinate monotonic version counters with the recovery design: advancing
an irreversible counter too early may invalidate the only fallback image.
This is a system contract among update manager, bootloader and trust policy.

## Recovery evidence

Preserve attempted version, verified artifact identity, previous confirmed slot,
boot attempts, failed health criterion, migration generation and reset reason.
Test power loss before and after selection and confirmation, not just while
copying the image. Factory reset must enumerate which data classes it clears
and which provisioning/calibration records survive.

## Related topics

- [Stage 13 overview](index.md)
- [Durability and locking](../system-calls-files-and-file-descriptors/durability-locking-and-power-loss.md)
