---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Stage 11: Services, Init, And systemd

A production userspace program needs a lifecycle owner. Init starts it in the right
environment, supervises it, supplies logs and limits, and reacts when it exits. This
stage explains that contract without confusing “process exists” with “service is
ready.”

## Service model

```text
boot -> PID 1 -> mounts/dependencies -> service start
                              |
                  readiness / health / watchdog
                              |
             stop -> deadline -> kill -> reap/restart
```

## Learning materials

1. [PID 1, Init, And Early Userspace](pid1-init-and-early-userspace.md)
2. [systemd Units, Dependencies, And Ordering](systemd-units-dependencies-and-ordering.md)
3. [Service Lifecycle, Readiness, And Restart](service-lifecycle-readiness-and-restart.md)
4. [Logging, tmpfiles, And Watchdogs](logging-tmpfiles-and-watchdogs.md)
5. [Service Sandboxing And Resource Controls](service-sandboxing-and-resource-controls.md)

## The service contract

| Area | Decision |
| --- | --- |
| Start | Executable, argv, environment, cwd, user, mounts, dependencies |
| Ready | What health evidence allows clients to proceed? |
| Stop | Signal, quiesce, deadline, escalation, child scope |
| Failure | Exit status, crash evidence, restart/backoff, degraded mode |
| State | Runtime directory, sockets, persistent data, stale artifacts |
| Observability | Logs, status, watchdog, metrics, core/symbol identity |
| Security | User, capabilities, namespaces, syscall/device/filesystem access |
| Resources | CPU, memory, FDs, tasks, time, I/O, storage |

## Stage lab

Inspect an installed unit or service, even if the target uses another init:

```sh
systemctl cat example.service 2>/dev/null || true
systemctl show example.service -p ExecStart -p User -p Restart 2>/dev/null || true
systemd-analyze verify example.service 2>/dev/null || true
```

Always compare the actual target init and version. BusyBox init, OpenRC, and a custom
supervisor use different readiness and restart contracts.

## Completion criteria

You can explain who owns PID 1 responsibilities, express dependency ordering, define
readiness and shutdown, preserve failure evidence, and apply least-privilege resource
controls without making the service impossible to debug.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](../hardware-facing-userspace-and-kernel-uapi/index.md)
- [Stage 12: Identity, Privilege, And Userspace Security](../identity-privilege-and-userspace-security/index.md)
- [Stage 14: Diagnostics, Debugging, And Performance](../diagnostics-debugging-and-performance/index.md)

## References

- [systemd documentation](https://www.freedesktop.org/wiki/Software/systemd/)
- [`systemd.service(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html)
- [`systemd.exec(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.exec.html)
