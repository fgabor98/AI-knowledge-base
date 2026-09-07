---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Atomic Persistence And Schema Migration

## The durable replacement pattern

For a small complete record in one directory, a typical update is:

1. Create a uniquely named temporary file with restrictive permissions.
2. Write the complete new representation and check every return value.
3. Flush the file with `fsync()` or the required stronger primitive.
4. Rename it over the old name within the same filesystem.
5. Open and synchronize the containing directory when directory durability matters.
6. Reopen and validate the record after restart.

`rename()` gives readers an atomic name transition, but not necessarily durable storage by itself. Cross-filesystem renames fail, and a successful `close()` is not a substitute for the required synchronization. For larger data, use a journal, append-only generations, or a database whose durability contract is understood.

## Record design

Include a magic value, schema version, length, generation, and integrity check. Reject impossible lengths before allocation. A checksum detects accidental corruption; authentication or a signature is needed when an attacker can modify the medium. Generation numbers help reject stale copies and make recovery decisions deterministic.

Keep the old record until the new one is known to be durable. On load, validate candidates completely, choose the newest valid generation, and explicitly report whether recovery used a fallback. Never “repair” by overwriting the only surviving copy before the replacement has been verified.

## Schema migration

Migrations should be bounded, idempotent, and transactional from the perspective of the application. Prefer read-old/write-new conversion into a new generation. If a migration can fail, preserve the old version and make the service able to retry or roll back. Test upgrades across every supported version, interrupted migration points, missing fields, unknown fields, and oversized values.

## Visibility, durability, and ambiguous outcomes

Assume the old record was already durable, one writer owns the update, and
the filesystem supports the synchronization protocol:

| Last completed step | What a running reader can observe | What the writer can report |
| --- | --- | --- |
| Temporary write | Old named record; temporary may be incomplete | Not committed |
| Temporary file sync | Old name; complete new temporary | Prepared, not published |
| Rename | New opens see new inode; existing FDs still see old inode | Published, durability pending |
| Directory sync | New name with the filesystem's reported durability | Commit acknowledged |

If directory sync fails after rename, deleting the new record does not restore
a known previous state. Return an explicit uncertain durability result and
reconcile on restart. Retrying an operation after `EIO` is not by itself
proof that earlier writeback succeeded. Preserve the error and storage context.
See [fsync(2)](https://man7.org/linux/man-pages/man2/fsync.2.html).

The replacement inode has its own ownership, mode, ACLs and extended attributes.
Set required metadata before the file sync. A rename-overwrite does not retain
an addressable backup automatically. If rollback needs the old contents,
keep a separately committed previous generation with a defined retention rule.

## Multiple files and schema migration

Two atomic renames do not create a transaction across two files. A reader can
observe a new configuration and old calibration. One approach writes an
immutable generation directory, syncs its files and necessary directory entries,
then publishes one small manifest selecting that generation. Readers bind their
whole operation to that manifest's generation.

For schema 1 to 2, read and validate schema 1, transform into separate storage,
validate schema 2, persist, then switch the active generation. Record migration
identity so restart does not apply a non-idempotent transform twice. Keep
schema 1 readable while rollback to the old binary remains possible.

When using SQLite, choose and document journal and synchronization settings,
preserve recovery files, and use its transaction/backup APIs instead of applying
a home-grown rename protocol to a live database. See
[SQLite atomic commit](https://www.sqlite.org/atomiccommit.html).

## Interruption exercise

In a private test directory, pause the writer at each row of the table.
Read through an FD opened before rename and through a fresh open afterward;
they should demonstrate old/new inode visibility. Kill the writer before each
next step, restart, and validate the selected record and leftover temporaries.
This tests process-crash recovery. Real power cuts are a separate test of the
filesystem, controller and medium.

## Related topics

- [Stage 13 overview](index.md)
- [Durability and locking](../system-calls-files-and-file-descriptors/durability-locking-and-power-loss.md)
