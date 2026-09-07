---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# systemd Units, Dependencies, And Ordering

## What problem does this solve?

Systemd separates “pulled in” dependencies from ordering. A unit can be ordered
after another without requiring it, or required without being ordered. Correct units
express both resource availability and start/stop ordering.

## Unit concepts

```ini
[Unit]
Description=Example service
Wants=network-online.target
After=network-online.target local-fs.target

[Service]
ExecStart=/usr/bin/example
User=example
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

`Requires`/`Wants` express dependency; `After`/`Before` express order. `BindsTo` and
`PartOf` add lifecycle coupling. Choose the weakest relationship that matches the
product; overusing `Requires` can turn an optional device into a boot failure.

## Readiness and conditions

Use path/device conditions only when they reflect a real contract. A path condition
can pass before a filesystem is mounted. Prefer explicit mount/device/service units,
activation, or a service’s own capability/readiness check. Network-online is only as
strong as the network manager’s implementation and should not replace reconnect logic.

## Exec and environment

Specify absolute `ExecStart`, `WorkingDirectory`, `EnvironmentFile` policy, user,
groups, standard streams, limits, and restart behavior. Do not assume an interactive
shell, PATH, home directory, or inherited descriptors. Use `ExecStartPre` for bounded
validation, not long-lived application logic.

## Verification

```sh
systemd-analyze verify ./example.service
systemctl list-dependencies example.service
systemctl show example.service
journalctl -u example.service -b
```

## Dependency behavior in concrete terms

| Declaration on A | What happens to B |
| --- | --- |
| `Wants=B.service` | Starting A also requests B; B failing need not stop A |
| `Requires=B.service` plus `After=B.service` | B is requested first; its activation failure prevents A starting |
| `After=B.service` alone | Orders jobs if both are present; does not start B |
| `PartOf=B.service` | B's stop/restart propagates to A; does not itself start B |
| `BindsTo=B.service` plus `After=B.service` | Stronger lifetime coupling when A cannot operate without B |

Ordering uses B's service type to decide when startup is complete. If B is
`Type=simple`, A may proceed before B has bound its socket. For a real
readiness dependency, B needs a supported readiness mechanism or socket
activation, and A still needs runtime disconnect handling.

`WantedBy=multi-user.target` is enablement metadata: enabling creates links.
Starting does not enable future boot starts, and enabling without `--now`
does not start immediately. `daemon-reload` reloads unit definitions;
application configuration reload is a separate action.

For a persistent mount needed by the service, consider
`RequiresMountsFor=/var/lib/example`. A condition such as
`ConditionPathExists=` is evaluated at start time; it does not watch for
the path to appear later. Avoid hiding required diagnostic command failures
behind `|| true` when verifying the actual unit.

## Common mistakes

- Using `After=` without `Requires=`/`Wants=` when availability is required.
- Assuming `network-online.target` proves a usable application route.
- Relying on PATH, cwd, shell syntax, or user login environment.
- Making optional hardware a hard boot dependency.
- Omitting restart/backoff and resource limits.

## Debugging checklist

- Inspect the rendered unit, drop-ins, dependencies, environment, user, and limits.
- Compare `After` ordering with actual readiness evidence.
- Capture `systemctl status`, journal, exit status, and cgroup state.
- Test missing mount/device, delayed network, config failure, restart, and shutdown.

## Related topics

- [Stage 11: Services, Init, And systemd](index.md)
- [Service Lifecycle, Readiness, And Restart](service-lifecycle-readiness-and-restart.md)
- [Service Sandboxing And Resource Controls](service-sandboxing-and-resource-controls.md)

## References

- [`systemd.unit(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html)
- [`systemd.service(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html)
- [`systemd.target(5)`](https://www.freedesktop.org/software/systemd/man/latest/systemd.target.html)
