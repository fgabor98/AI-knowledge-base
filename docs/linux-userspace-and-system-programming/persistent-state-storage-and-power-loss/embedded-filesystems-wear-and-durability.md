---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Embedded Filesystems, Wear, And Durability

Flash storage has erase-block geometry, finite program/erase endurance, bad-block behavior, and translation layers that make write patterns important. Filesystem journaling improves consistency but does not make arbitrary power-loss behavior identical across media.

## Choose with the medium

SquashFS is useful for immutable compressed root filesystems. Overlayfs adds a writable upper layer but needs a deliberate upper/work layout and does not make lower content mutable. ext4 and f2fs operate on block devices. JFFS2 normally operates directly on MTD, while UBIFS operates on UBI volumes above MTD; UBI handles wear and bad blocks below UBIFS.

The correct choice depends on raw versus managed flash, capacity, write rate, RAM, recovery time, power-fail requirements, and update strategy. Do not select a filesystem from a generic benchmark without testing the target medium and workload.

## Wear and quotas

Count writes to hot metadata as well as payload bytes. Repeatedly rewriting one small file can amplify into filesystem and flash-block writes. Batch changes, use append/generation strategies, reserve space for recovery, rotate logs, and rate-limit nonessential persistence. Watch for a filesystem becoming read-only and make that condition visible to the service supervisor.

## Power-fail testing

Test at realistic write points: before flush, after flush, after rename, during migration, while rotating logs, and during update metadata changes. Reboot repeatedly and classify outcomes as old-valid, new-valid, recoverable-fallback, or unsafe/ambiguous. A test that only checks “the process restarted” misses silent data loss.

## Follow the storage stack

```text
eMMC / SD / SSD: application -> block filesystem -> block layer -> controller/FTL -> flash
raw NAND:       application -> UBIFS -> UBI -> MTD -> flash
raw flash:      application -> JFFS2 -> MTD -> flash
```

An eMMC controller hides erase geometry and bad-block management behind a
block interface. Raw MTD exposes a different abstraction. UBIFS is built on
UBI volumes; JFFS2 normally uses MTD directly. Do not format a block device
with a raw-flash recipe merely because both devices contain NAND.
See [UBIFS](https://docs.kernel.org/filesystems/ubifs.html).

Journaling and crash recovery preserve the filesystem's invariants; they do
not automatically make every application update a durable transaction.
The application's flush and publication order still matters. Device firmware
that misreports persistence can invalidate assumptions above it.

## A write-budget calculation

A 4 KiB snapshot written once per second is about 338 MiB per day before
filesystem metadata and device write amplification. Once per minute reduces
the logical traffic by sixty, but can lose up to a minute of uncommitted
changes. Choose the interval from the allowed data loss and recovery model.

Measure actual workload writes and tail latency on the target. Nearly full
storage can change garbage collection, write amplification, and sync latency.
Test fresh, representative aged, and low-free-space conditions; a short test
on empty media does not establish field endurance.

An overlay upper layer can continue shadowing files after a lower image
update. Decide whether upper state migrates, is reset, or is restricted to
specific data paths. Treat overlay whiteouts and copied-up configuration as
part of update compatibility.

## Related topics

- [Stage 13 overview](index.md)
- [Durability and locking](../system-calls-files-and-file-descriptors/durability-locking-and-power-loss.md)
