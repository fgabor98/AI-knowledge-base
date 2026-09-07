---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Read-Only Rootfs, Overlayfs, And Persistent State

## What problem does this solve?

Read-only system images reduce accidental corruption and simplify updates, but they
force every write to have a deliberate destination. Overlay filesystems can make an
immutable lower image appear writable, yet their upper layer still has capacity,
wear, and recovery constraints. A service must know which data can be recreated and
which data must survive reboot, update, and power loss.

## Classify data before choosing a path

| Data | Recreate? | Typical policy |
| --- | --- | --- |
| Factory default | No, but shipped with image | Immutable lower layer, versioned with software |
| Active configuration | Usually no | Validated persistent partition, atomic replacement and migration |
| Calibration/identity | No | Protected persistent storage, backup and corruption detection |
| Runtime socket/PID | Yes | `/run`, service-owned, recreated at boot |
| Cache | Yes | Volatile or rebuildable persistent cache with a size bound |
| Logs | Evidence but bounded | Rotation, quotas, remote export, behavior when full |
| Update metadata | No during update | Durable transaction and rollback/recovery record |
| Temporary work | Yes | Private `tmpfs` or temporary file; cleanup on restart |

Do not make a directory writable simply because an application currently writes there.
Move the data to the class and mount that matches its lifetime.

## Overlayfs mental model

```text
visible merged tree
       |
       +-- lowerdir: immutable image
       +-- upperdir: writable changes
       +-- workdir: overlayfs bookkeeping
```

Reads search the merged view. A modification to a lower-only file may trigger a
copy-up into the upper layer; deletion may create a whiteout. The upper and work
directories must meet filesystem and mount requirements, and the upper layer can
fill even while the lower image has abundant space.

Overlayfs is a namespace and deployment mechanism, not a database transaction. A
crash during copy-up or update still needs filesystem and application recovery rules.
The visible path may hide whether data is lower, upper, or a whiteout; inspect the
mount and backing filesystems when diagnosing storage behavior.

## Read-only does not mean immutable everywhere

A rootfs can be mounted read-only while `/run`, `/tmp`, `/var`, or `/data` are
writable separate mounts. Conversely, a writable overlay upper layer may permit
changes that disappear on reboot. Define the expected writable paths in the service
contract and enforce them with service sandboxing where available.

```sh
findmnt -o TARGET,SOURCE,FSTYPE,OPTIONS
findmnt -T /etc
findmnt -T /var/lib/my-service
df -hT
df -ih
```

Do not use a failed write to discover policy in production; inspect the image and
mount configuration first. A read-only error may be `EROFS`, but a full upper layer
can present as `ENOSPC` or `EDQUOT`.

## Persistent-state protocol

For a small configuration or state file:

```text
read current file
validate syntax, schema, ranges, and identity
write a new file in the same directory
flush file data if the durability policy requires it
atomically rename the new file into place
flush the directory if required
```

Include a schema version, checksum or integrity field when corruption detection is
needed. Keep an old generation or recovery copy when rollback is safer than failure.
Never assume that `rename` alone survives a sudden power loss.

For larger state, use a journal, append-only record protocol, database with a known
embedded durability model, or an application-specific two-slot scheme. Bound file
size, record count, replay time, and recovery memory.

## Wear and capacity

Flash has finite erase/program endurance. High-frequency counters, logs, and state
rewrites can wear the persistent partition. Consider batching, rotation, wear-levelled
storage, a dedicated data partition, remote logging, and explicit loss policy. A
read-only rootfs does not protect a poorly designed writable data partition.

Track both bytes and inodes. A service can fail with free bytes remaining if it has
exhausted inodes, quotas, reserved blocks, or overlay upper-layer space.

## Factory reset and update policy

Define whether reset removes configuration, identity, calibration, logs, or update
metadata. An update should not accidentally copy volatile defaults over valid
persistent state. A/B systems should record slot selection, boot attempts, health,
and rollback decisions durably enough for the bootloader and userspace to agree.

## Common mistakes

- Writing mutable state into `/usr` or another image-owned path.
- Treating overlayfs as durable transactional storage.
- Assuming a successful write survives power loss.
- Ignoring upper-layer, inode, quota, and wear limits.
- Putting identity or calibration in a cache directory.
- Replaying an unbounded log during boot.
- Letting logs consume all space needed for recovery.

## Debugging checklist

- Classify the path’s data and expected lifetime.
- Identify visible, lower, upper, persistent, and volatile mounts.
- Check filesystem type, options, free bytes, free inodes, quota, and read-only state.
- Reproduce power loss at each persistence step on representative storage.
- Inspect update/reset/rollback behavior with old and new schema versions.
- Check whether a stale overlay whiteout or upper file hides a lower default.
- Verify service behavior when persistence is absent, corrupt, full, or read-only.

## Related topics

- [Stage 1: Linux Runtime, Filesystem, And Rootfs](index.md)
- [Mounts, Initramfs, And Rootfs Layout](mounts-initramfs-and-rootfs-layout.md)
- [Durability, Locking, And Power Loss](../system-calls-files-and-file-descriptors/durability-locking-and-power-loss.md)
- [Atomic Persistence And Schema Migration](../persistent-state-storage-and-power-loss/atomic-persistence-and-schema-migration.md)
- [OTA And Update System Build Integration](../../build-systems/advanced/ota-update-system-build-integration.md)

## References

- [Overlay Filesystem documentation](https://docs.kernel.org/filesystems/overlayfs.html)
- [`sync(2)`](https://man7.org/linux/man-pages/man2/sync.2.html)
- [`fsync(2)`](https://man7.org/linux/man-pages/man2/fsync.2.html)
- [Linux kernel filesystem documentation](https://www.kernel.org/doc/html/latest/filesystems/index.html)
