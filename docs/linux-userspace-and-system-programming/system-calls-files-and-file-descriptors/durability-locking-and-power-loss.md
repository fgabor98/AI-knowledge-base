---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Durability, Locking, And Power Loss

## What problem does this solve?

Returning from `write` means bytes were accepted by the interface, not necessarily
that they reached nonvolatile media. Multiple processes can also update the same
state unless their coordination protocol is explicit. This page separates ordering,
visibility, locking, and durability so a product can define what survives a crash or
power interruption.

## The durability ladder

```text
application buffer
      |
write returned
      |
kernel page cache / filesystem journal
      |
fsync or fdatasync returned
      |
device cache / storage controller
      |
power-loss-safe nonvolatile media
```

Each step is a different guarantee. `fflush` moves libc-buffered bytes to the FD; it
does not request storage durability. `fsync` requests synchronization for a file FD;
`fdatasync` can omit metadata not needed to read the data. Neither promise can exceed
the filesystem, device, mount, and hardware power-loss behavior. A storage device
with volatile write cache may require a flush/FUA policy outside the application.

## Durable replacement

For a small named state file:

```text
write complete bytes to a new file in the same directory
validate and close application-level content
fsync(new_fd)
rename(new_name, final_name)
fsync(directory_fd)
```

The directory synchronization matters because the rename is a directory metadata
change. On filesystems and products with weaker or different guarantees, verify the
policy experimentally. If any step fails, classify the state as unchanged, new,
unknown, or requiring recovery; do not claim success because the write returned.

## Journals and schemas

For frequent or larger state, use a journal or generation protocol:

```text
header: magic, schema, generation, length, checksum
payload
commit marker / valid generation
```

On startup, validate length, checksum, schema, generation, and semantic ranges. Choose
the newest complete generation and ignore an incomplete tail. Bound replay time and
record count. Schema migration must be atomic and recoverable if power fails midway.

## Advisory locks

`flock` and POSIX `fcntl` record locks are advisory: they coordinate only with
cooperating processes using the same convention. They do not make a regular file
safe against a process that ignores the lock or replaces the pathname.

Document:

- lock file or data FD and the scope it protects;
- shared versus exclusive mode;
- blocking versus nonblocking acquisition;
- owner death and stale lock behavior;
- whether lock lifetime follows an FD, an open-file description, or a process;
- whether the lock survives `fork`, `exec`, and descriptor duplication;
- what happens when a filesystem is remote, read-only, or unavailable.

Use a directory-relative, securely created lock object. A PID written into a lock
file is not proof that the owner is alive because PIDs are reused; use a PIDFD or
another verified identity where appropriate.

## Power-loss test matrix

Test interruption at each boundary:

| Point of interruption | Expected recovery question |
| --- | --- |
| Before temp creation | Old state remains valid |
| During temp write | Partial temp is ignored or cleaned |
| After file sync | Temp is complete but may not be named |
| During rename | Old or new generation is selected safely |
| Before directory sync | Recovery policy handles uncertain namespace durability |
| During migration | Old schema or complete new schema remains usable |
| During log append | Parser rejects incomplete record and continues |

Use actual target storage and representative power interruption or fault injection.
A desktop ext4 test is evidence for that setup only, not for every flash device,
filesystem, controller, or mount option.

## Full and read-only storage

Handle `ENOSPC`, `EDQUOT`, `EROFS`, `EIO`, and directory-entry failures. Reserve space
for recovery metadata and logs; do not let normal logging consume the only space in
which a service can commit state. Define whether the service degrades, sheds cache,
or stops accepting configuration when durable storage is unavailable.

## Common mistakes

- Treating `write`, `fflush`, or `close` as a power-loss commit.
- Syncing a file but not the directory after rename.
- Calling advisory locking mandatory enforcement.
- Using a PID file as a robust lock or owner identity.
- Migrating a file in place without an interrupted-migration format.
- Ignoring storage-device caches, wear, quotas, and read-only remounts.
- Testing only clean shutdown instead of abrupt power loss.

## Debugging checklist

- State the exact durability guarantee the product requires.
- Record filesystem, mount options, storage device, and sync calls.
- Inspect file and directory generations after induced interruption.
- Check locks using the actual owner and cooperation protocol.
- Test full, read-only, I/O-error, and absent-storage behavior.
- Preserve recovery logs and distinguish confirmed commit from unknown outcome.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](index.md)
- [Regular-File I/O And Metadata](regular-file-io-and-metadata.md)
- [Read-Only Rootfs, Overlayfs, And Persistent State](../linux-runtime-filesystem-and-rootfs/read-only-rootfs-overlayfs-and-persistent-state.md)
- [Atomic Persistence And Schema Migration](../persistent-state-storage-and-power-loss/atomic-persistence-and-schema-migration.md)

## References

- [`fsync(2)`](https://man7.org/linux/man-pages/man2/fsync.2.html)
- [`open(2)`](https://man7.org/linux/man-pages/man2/open.2.html)
- [`flock(2)`](https://man7.org/linux/man-pages/man2/flock.2.html)
- [`fcntl(2)` record locks](https://man7.org/linux/man-pages/man2/fcntl.2.html)
- [Linux kernel filesystems documentation](https://www.kernel.org/doc/html/latest/filesystems/index.html)
