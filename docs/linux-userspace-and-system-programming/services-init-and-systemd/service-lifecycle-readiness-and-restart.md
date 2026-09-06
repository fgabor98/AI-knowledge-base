---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Service Lifecycle, Readiness, And Restart

## What problem does this solve?

Supervisors can restart a process, but restart is not recovery. A service needs
defined startup, readiness, quiesce, shutdown, crash, and repeated-failure states.

## Lifecycle states

```text
STARTING -> READY -> QUIESCING -> STOPPED
    |         |          |
  failure   degraded   deadline -> forced stop
    v         v
 RESTARTING <- supervisor
```

Readiness should follow successful configuration, dependency checks, device capability
query, listener setup, and worker startup. Report degraded state separately when the
service is alive but cannot provide all functionality.

## Restart policy

Classify exits: clean administrative stop, configuration failure, transient dependency,
crash, watchdog timeout, resource kill. Use bounded restart rate and backoff. Preserve
the first failure’s core/log/status evidence; otherwise a restart loop overwrites the
useful cause with repetitive “started” messages.

## Shutdown

Stop intake, cancel/drain work, stop children, close hardware safely, commit or mark
unknown persistent operations, and exit before the deadline. The supervisor may then
escalate to SIGKILL. Startup must handle all unclean remnants.

## Common mistakes

- Advertising ready before device/configuration health exists.
- Restarting instantly forever.
- Treating a clean process exit as service success.
- Forgetting descendants and in-flight work during stop.
- Losing first-crash evidence under repeated restarts.

## Debugging checklist

- Record state transitions, readiness reason, exit status/signal, restart count, and
  supervisor deadline.
- Test dependency absence/delay, config failure, crash, watchdog, SIGTERM, SIGKILL,
  and repeated restart.
- Verify stale sockets, locks, devices, temp files, and persistent state recovery.

## Related topics

- [Stage 11: Services, Init, And systemd](index.md)
- [systemd Units, Dependencies, And Ordering](systemd-units-dependencies-and-ordering.md)
- [Logging, tmpfiles, And Watchdogs](logging-tmpfiles-and-watchdogs.md)
- [Hardware Service State Machines And Recovery](../design-and-architecture-patterns/hardware-service-state-machines-and-recovery.md)

## References

- [`systemd.service(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html)
- [`systemd.kill(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.kill.html)
- [`Restart=`](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html#Restart=)
