---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Stage 13: Persistent State, Storage, And Power Loss

Persistent state is an interface between software versions, hardware lifetime, and failure timing. A design must say what data is authoritative, what can be rebuilt, what must survive reboot, and what happens when power disappears halfway through a write.

## Persistence ladder

Classify every datum as configuration, identity/credential, calibration, operational state, cache, log, update metadata, or diagnostic evidence. Then choose the least durable store that satisfies the requirement. Reconstructible cache belongs on a different path from irreplaceable calibration; logs should not consume the same quota as boot-critical state.

For each record define owner, format version, size bound, update rate, retention, confidentiality, integrity, recovery behavior, and migration policy. Include the storage medium and filesystem in the threat model.

## Failure model

Power loss can occur during data write, metadata update, rename, filesystem journal commit, erase/program operation, or a bootloader state transition. “The write returned” only means the kernel accepted the request. Durability requires the correct synchronization operation and a medium/filesystem that honors it.

## Completion checklist

- [ ] Every persistent datum has an owner, schema, and recovery policy.
- [ ] Updates are atomic with respect to readers and power loss.
- [ ] Corruption is detected and a safe fallback exists.
- [ ] Wear, quota, and write amplification are measured.
- [ ] Update metadata cannot select an unverified or incomplete image.

## Further reading

- [`fsync(2)`](https://man7.org/linux/man-pages/man2/fsync.2.html)
- [`rename(2)`](https://man7.org/linux/man-pages/man2/rename.2.html)
- [ext4 journaling](https://docs.kernel.org/filesystems/ext4/journal.html)
- [UBI documentation](https://docs.kernel.org/filesystems/ubifs.html)

## Stage lab and acceptance evidence

Choose one settings record and document its schema, writer, lock, permissions,
size bound, durability point and fallback generation. Build the failure table
before implementing the write path. Demonstrate that old readers retain their
old snapshot while new readers see a complete replacement.

Repeat with disk full, missing data mount, read-only storage, interrupted
migration and rollback to the previous binary. Record what each test proves:
a process kill tests recovery code; a target power cut tests additional storage
assumptions. Neither replaces the other.

## Learning materials

1. [State Classes And Storage Policy](state-classes-and-storage-policy.md)
2. [Atomic Persistence And Schema Migration](atomic-persistence-and-schema-migration.md)
3. [Embedded Filesystems, Wear, And Durability](embedded-filesystems-wear-and-durability.md)
4. [Updates, Rollback, And Recovery State](updates-rollback-and-recovery-state.md)
