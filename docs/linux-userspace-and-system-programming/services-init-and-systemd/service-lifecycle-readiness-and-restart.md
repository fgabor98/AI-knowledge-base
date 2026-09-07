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

## Select the startup contract explicitly

`Type=simple` considers the process launched early in startup.
`Type=exec` waits for executable invocation but still does not prove
application initialization. `Type=notify` waits for the program's readiness
notification. `Type=oneshot` represents bounded work; with
`RemainAfterExit=yes`, an active unit need not have any running process.

For a daemon that implements systemd notification, this is an illustrative
policy, to be adjusted to measured startup and shutdown times:

```ini
[Unit]
Description=Example device service
StartLimitIntervalSec=60
StartLimitBurst=3

[Service]
Type=notify
NotifyAccess=main
ExecStart=/usr/bin/example-device-service
User=example
RuntimeDirectory=example
StateDirectory=example
Restart=on-failure
RestartSec=3s
RestartPreventExitStatus=78
TimeoutStartSec=20s
TimeoutStopSec=10s
KillMode=control-group
```

Here exit 78 is an **application convention** for invalid configuration.
The application sends `READY=1` after its required startup work and performs
shutdown after SIGTERM. A program without notification support will time out
with this unit. Rate limiting eventually stops automatic restarts; it is not
an infinite slow retry mechanism. See
[systemd.service](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html).

A deliberate stop should not start a reconnect loop. Give the application a
shutdown deadline shorter than the supervisor's limit, reserve time for cleanup,
and make the next start tolerate SIGKILL at every step.

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
