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
