---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Service Sandboxing And Resource Controls

## What problem does this solve?

A service should have only the filesystem, devices, privileges, syscalls, and
resources required for its job. Sandboxing reduces blast radius, while limits prevent
one component from exhausting the target. Both can also break hidden assumptions and
must be tested with the production policy.

## Control categories

| Control | Examples | Failure to expect |
| --- | --- | --- |
| Identity | `User`, groups, capabilities, `NoNewPrivileges` | `EACCES`, `EPERM` |
| Filesystem | `ProtectSystem`, read-only paths, private `/tmp` | Missing/write-protected paths |
| Devices | `DeviceAllow`, private `/dev` | `ENODEV`, permission failure |
| Namespaces | mount, network, IPC, PID | Different paths, routes, PIDs |
| Syscalls | seccomp allow/deny policy | `SIGSYS`, `EPERM` |
| Resources | memory/tasks/FD/CPU/time limits | `ENOMEM`, `EMFILE`, throttling, OOM |
| Network | address family/interface restrictions | bind/connect failure |

Apply controls incrementally, verify the service still has required observability,
and document every exception. A sandbox is part of the deployment ABI.

## Resource budgeting

Bound descriptors, threads, mappings, memory, queue depth, CPU, log rate, and startup
time. Set cgroup limits according to measured peaks plus recovery headroom. A limit
without a graceful application policy turns overload into an abrupt kill.

## Debugging restricted services

Inspect rendered unit properties and audit logs. Compare interactive and service
identity, namespace, environment, cwd, mounts, capabilities, and limits. Temporarily
relax one control in a controlled test to confirm a hypothesis, then restore the
policy and add a targeted allow rule.

## A policy with visible consequences

Start from the real resource map. A local protocol daemon that needs no devices
can often use `NoNewPrivileges=yes`, `ProtectSystem=strict`,
`ProtectHome=yes`, `PrivateTmp=yes`, and `PrivateDevices=yes`, with
managed runtime/state directories. A hardware daemon usually needs a different
device policy: `DeviceAllow` does not make a device node appear inside the
private `/dev` created by `PrivateDevices`.

Similarly, making a path writable does not mount missing storage or create a
file, and a private network namespace has its own interfaces and routes.
Test the precise combination rather than each directive in isolation.

Measure a startup/recovery peak before choosing `MemoryHigh` and
`MemoryMax`. The former applies reclaim pressure/throttling; the latter is a
hard limit that can cause OOM handling. `TasksMax` counts threads as well as
processes. `LimitNOFILE` bounds descriptor numbers, while application
connection and queue limits should reject overload earlier.

Inspect effective properties and logs, then exercise one allowed and one denied
operation per boundary. `systemd-analyze security` is a useful policy overview;
its score does not establish that authorization or hardware safety is correct.
See [resource controls](https://www.freedesktop.org/software/systemd/man/latest/systemd.resource-control.html).

## Common mistakes

- Running as root because a sandbox was not designed.
- Granting all devices or capabilities for convenience.
- Hiding `/proc` and then removing all diagnosis paths.
- Setting limits below normal bursts or without recovery headroom.
- Treating an `EPERM` as an application bug without checking policy.

## Debugging checklist

- Capture unit, UID/GIDs, capabilities, namespaces, mounts, cgroup, and seccomp/LSM
  evidence.
- Test startup, device access, reload, logging, crash, and recovery under limits.
- Verify core dumps/debugging are available through a controlled support path.
- Check that failed writes/calls are handled and not retried into a storm.

## Related topics

- [Stage 11: Services, Init, And systemd](index.md)
- [Capabilities And Privilege Dropping](../identity-privilege-and-userspace-security/capabilities-and-privilege-dropping.md)
- [seccomp, LSM, And Service Isolation](../identity-privilege-and-userspace-security/seccomp-lsm-and-service-isolation.md)
- [Namespaces And cgroups](../identity-privilege-and-userspace-security/namespaces-and-cgroups.md)

## References

- [`systemd.exec(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.exec.html)
- [`systemd.resource-control(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.resource-control.html)
- [`systemd.directives(7)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.directives.html)
