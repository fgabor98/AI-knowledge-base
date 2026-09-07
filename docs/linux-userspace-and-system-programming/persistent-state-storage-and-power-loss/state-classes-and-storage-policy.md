---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# State Classes And Storage Policy

## Classify before choosing a file

- **Factory identity and calibration:** authoritative, protected from casual reset, often signed or checksummed, and written rarely.
- **User configuration:** editable, versioned, validated, and recoverable to defaults.
- **Operational state:** useful across restart but not necessarily across replacement; bound its size and update rate.
- **Cache:** disposable and rebuildable; keep it away from boot-critical state.
- **Logs and crash evidence:** append or rotate with quotas; preserve enough context without filling the root filesystem.
- **Update state:** small, transactional, and understandable by both userspace and boot firmware.

## Policy fields

Record for each item:

| Field | Question |
| --- | --- |
| Owner | Which component may write it? |
| Authority | Is it the source of truth or a derived copy? |
| Lifetime | Does it survive restart, firmware update, factory reset, or replacement? |
| Integrity | How are truncation, stale data, and tampering detected? |
| Budget | What are the maximum bytes and writes per hour? |
| Recovery | What safe value applies when missing or invalid? |

Use separate directories, quotas, and permissions for unrelated classes. A service should not be able to overwrite boot selection merely because it can write its runtime state. If two components need the same value, define one owner and an explicit read interface rather than allowing competing writers.

## Operational discipline

Never assume storage is infinite or healthy. Monitor free space, write failures, filesystem read-only transitions, and accumulated update generations. Expose a clear state such as `valid`, `defaulted`, `migrating`, or `degraded`; silent fallback makes field diagnosis much harder.

## Example storage contract

| Record | Location policy | Writer | Failure behavior |
| --- | --- | --- | --- |
| Installed defaults | Immutable application data | Image builder | Reject incompatible image |
| User settings | Dedicated persistent state | Configuration owner | Keep last validated generation |
| Control socket | Private `/run` directory | Service instance | Recreate after proving ownership |
| Derived index | Bounded cache partition/directory | Cache worker | Rebuild from authoritative data |
| Device identity | Provisioning-owned storage | Provisioning workflow | Fail identity-dependent operations |
| Crash evidence | Quota-limited diagnostics | Collector | Preserve first cause; report truncation |

These are policy choices, not mandatory paths. Write down which records survive
a service restart, reboot, firmware rollback and factory reset. Keep defaults
distinct from user modifications so an update can supply new defaults without
silently replacing a user's choice.

## Locks and replaceable state

A lock on the current data inode does not necessarily protect the replacement
inode installed by rename. Use a separate stable lock object or one writer
service for the entire read-modify-write transaction. All writers must follow
the same protocol.

A PID file is diagnostic metadata, not exclusive ownership. PID reuse and
crashes make existence checks unreliable. Similarly, unlinking a Unix socket
path just because a connection attempt failed can race another starter.
Prefer supervisor-owned runtime directories and explicit instance ownership.

A missing persistent mount must not cause writes into an underlying empty
directory on the root filesystem. Verify the expected storage identity before
accepting mutations, and expose read-only/degraded operation when it is absent.

## Related topics

- [Stage 13 overview](index.md)
- [Durability and locking](../system-calls-files-and-file-descriptors/durability-locking-and-power-loss.md)
