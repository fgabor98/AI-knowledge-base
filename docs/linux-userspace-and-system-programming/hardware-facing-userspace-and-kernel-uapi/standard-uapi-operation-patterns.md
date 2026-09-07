---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Standard UAPI Operation Patterns

## What problem does this solve?

Hardware interfaces reuse patterns: open/configure, query capabilities, submit,
wait/readiness, transfer data, stop, and recover. Applying those patterns prevents
an application from treating a device as a regular file when it is actually a stateful
asynchronous subsystem.

## A disciplined sequence

```text
discover identity -> open with flags -> query capabilities
-> configure -> arm/start -> wait/readiness -> transfer
-> validate -> stop/flush -> close/recover
```

Check each return value. Keep configuration separate from operation state, and make
recovery able to return to a known baseline. Record the device generation so late
events from an old open cannot update new state.

## Common operations

| Operation | Questions |
| --- | --- |
| `open` | Does it claim hardware? Can it block? What permissions and mode? |
| `ioctl` | What structure/version/direction and side effects? |
| `read/write` | Stream, record, command, or data? Short result? |
| `poll/epoll` | What does readiness mean? Can events be lost/coalesced? |
| `mmap` | Who owns pages, when valid, and how synchronized? |
| sysfs read/write | Units, state, range, and atomicity? |
| close | Does it stop/reset, release power, or discard pending work? |

## Capability and state queries

Query capabilities and current state rather than assuming the device tree, firmware,
or board revision. Reject unsupported modes before enabling hardware. A capability
query may succeed while a later operation fails because the device is busy, unplugged,
or not powered.

## Data validation

Validate length, status, sequence, endianness, units, range, checksum, and timestamp
domain before handing data to product logic. Preserve raw status/error context for
diagnosis. Do not use a successful byte count as a validity flag.

## Recovery

Use an explicit state machine: disconnected, opened, configured, active, faulted,
recovering. On reset or unplug, stop requests, invalidate buffers, close/reopen if
safe, re-query capabilities, and reject late completions from the old generation.

## Failure during partial initialization

Suppose discovery and open succeed, three buffers are mapped, and enabling the
device fails. Record each acquired resource as it becomes owned. Cleanup must
release exactly those resources and preserve the enable error, while separately
reporting a failed stop/unmap/close if it affects recovery.

Do not enter the normal read loop merely because an FD exists. Distinguish
`OPEN`, `CONFIGURED`, and `ACTIVE`, and reject commands inappropriate to
the current state. During recovery, repeat capability negotiation because
firmware, device identity, or available formats may have changed.

An operation table should include a side-effect column. A failed query often
has no product effect; a failed write or ioctl may leave hardware partly
configured. For the latter, read back state, perform a documented reset, or
mark the outcome unknown before accepting further commands. Host fakes should
model each failure boundary so rollback is exercised before target testing.

## Common mistakes

- Assuming all device files support ordinary file semantics.
- Skipping capability/state queries.
- Starting hardware before configuration is validated.
- Treating readiness, bytes, or ioctl return as data validity.
- Reusing buffers or accepting events after device reset.

## Debugging checklist

- Record identity, generation, state, operation, FD, sequence, and duration.
- Trace open/configure/start/wait/transfer/stop/close.
- Test busy, unsupported, timeout, partial data, reset, unplug, and reopen.
- Correlate userspace result with sysfs, kernel log, and hardware evidence.
- Verify recovery reaches a known configuration, not merely an open FD.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [ioctl ABI And Compatibility](ioctl-abi-and-compatibility.md)
- [poll, mmap, And Device Events](poll-mmap-and-device-events.md)
- [Hardware Service State Machines And Recovery](../design-and-architecture-patterns/hardware-service-state-machines-and-recovery.md)

## References

- [Linux kernel userspace API](https://www.kernel.org/doc/html/latest/userspace-api/index.html)
- [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html)
- [`ioctl(2)`](https://man7.org/linux/man-pages/man2/ioctl.2.html)
