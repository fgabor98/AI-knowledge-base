# Embedded Filesystems, Wear, And Durability

Flash storage has erase-block geometry, finite program/erase endurance, bad-block behavior, and translation layers that make write patterns important. Filesystem journaling improves consistency but does not make arbitrary power-loss behavior identical across media.

## Choose with the medium

SquashFS is useful for immutable compressed root filesystems. Overlayfs adds a writable upper layer but needs a deliberate upper/work layout and does not make lower content mutable. ext4 and f2fs are common block-filesystem choices with different workload tradeoffs. JFFS2 and UBIFS are designed for raw or managed flash through the MTD/UBI stack; UBI handles wear and bad blocks below UBIFS.

The correct choice depends on raw versus managed flash, capacity, write rate, RAM, recovery time, power-fail requirements, and update strategy. Do not select a filesystem from a generic benchmark without testing the target medium and workload.

## Wear and quotas

Count writes to hot metadata as well as payload bytes. Repeatedly rewriting one small file can amplify into filesystem and flash-block writes. Batch changes, use append/generation strategies, reserve space for recovery, rotate logs, and rate-limit nonessential persistence. Watch for a filesystem becoming read-only and make that condition visible to the service supervisor.

## Power-fail testing

Test at realistic write points: before flush, after flush, after rename, during migration, while rotating logs, and during update metadata changes. Reboot repeatedly and classify outcomes as old-valid, new-valid, recoverable-fallback, or unsafe/ambiguous. A test that only checks “the process restarted” misses silent data loss.
