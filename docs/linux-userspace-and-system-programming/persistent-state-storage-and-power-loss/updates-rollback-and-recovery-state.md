# Updates, Rollback, And Recovery State

## A/B and generation-based updates

A resilient update keeps a known-good bootable image while preparing a new one. The update manager writes the inactive slot, verifies its contents and metadata, then performs one atomic boot-selection transition. The bootloader should try the new slot only a bounded number of times and revert when userspace fails to report health.

The health signal must mean more than “the process started.” It should include filesystem availability, required services, essential hardware, configuration migration, and—where appropriate—a controlled functional check. Keep the old slot usable until the new slot has passed the acceptance window.

## Metadata contract

Boot selection metadata should include slot identity, image version, integrity/authenticity result, attempt count, confirmation state, and rollback reason. Protect it from partial writes with redundant records or a transactional mechanism. Validate all fields before acting; a corrupt attempt counter must not disable the recovery path.

## Recovery behavior

Define behavior for interrupted download, interrupted installation, power loss during metadata commit, failed migration, incompatible configuration, repeated watchdog resets, and a full data partition. Recovery must leave a diagnosable artifact and preserve enough logs to explain the decision. A rollback that restores binaries but leaves an incompatible persistent schema is not a complete rollback.

Treat factory reset and rescue mode as explicit, authenticated operations. They should not accidentally erase evidence or calibration data that the recovery procedure needs.
