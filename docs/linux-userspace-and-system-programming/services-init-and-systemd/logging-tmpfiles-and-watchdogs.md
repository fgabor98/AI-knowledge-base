---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Logging, tmpfiles, And Watchdogs

## What problem does this solve?

Logs and runtime directories are operational resources. They must remain useful when
storage is full, services restart, and the watchdog or supervisor needs evidence.

## Logging contract

Each high-value message should include monotonic or event time, service instance,
state, operation/request ID, peer/device, result, errno, duration, and generation as
appropriate. Separate expected retry noise from actionable failure. Bound rate and
size; redact credentials, tokens, and sensitive payloads.

Use stdout/stderr to the supervisor or a deliberate logging API. Do not assume `/var`
is writable or that a log file survives reboot. Define rotation, retention, remote
export, and behavior when logging fails.

## tmpfiles and runtime directories

Runtime paths under `/run` should be created with ownership/mode and cleaned at boot.
Use tmpfiles policy or an equivalent init mechanism for directories, sockets, and
volatile files. Do not put persistent configuration or identity there.

## Watchdogs

Feed a watchdog only after health criteria prove meaningful progress: event loop,
required workers, dependencies, and device communication. A heartbeat thread alone
can mask a deadlocked main service. Record feed and pretimeout evidence without
filling storage.

## Common mistakes

- Logging secrets or unbounded payloads.
- Writing logs to a read-only/full partition without fallback.
- Treating `/run` as persistent.
- Feeding watchdog from code that does not prove service health.
- Allowing diagnostic logging to alter timing or flash endurance.

## Debugging checklist

- Inspect journal/log sink, rotation, disk/inode use, and service identity.
- Check runtime directory owner/mode and stale socket/lock cleanup.
- Record watchdog owner, interval, feed criteria, pretimeout, and reset cause.
- Test full storage, missing log sink, restart, crash, and watchdog expiry.

## Related topics

- [Stage 11: Services, Init, And systemd](index.md)
- [Service Lifecycle, Readiness, And Restart](service-lifecycle-readiness-and-restart.md)
- [Read-Only Rootfs, Overlayfs, And Persistent State](../linux-runtime-filesystem-and-rootfs/read-only-rootfs-overlayfs-and-persistent-state.md)
- [CAN, Watchdog, And Control Interfaces](../hardware-facing-userspace-and-kernel-uapi/can-watchdog-and-control-interfaces.md)

## References

- [`systemd-journald.service(8)`](https://www.freedesktop.org/software/systemd/man/latest/systemd-journald.service.html)
- [`tmpfiles.d(5)`](https://www.freedesktop.org/software/systemd/man/latest/tmpfiles.d.html)
- [`systemd.service(5)` watchdog](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html#WatchdogSec=)
