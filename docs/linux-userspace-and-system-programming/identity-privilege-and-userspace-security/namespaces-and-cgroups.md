---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Namespaces And Cgroups

Namespaces change a process's view of selected kernel objects. Cgroups group processes for resource accounting and control. Neither mechanism, by itself, answers whether a request is authorized.

## Namespace dimensions

- **User:** maps UIDs/GIDs and capability meaning; useful for rootless isolation.
- **Mount:** gives a private mount topology; it does not automatically make files safe.
- **PID:** changes visible process IDs; the namespace's init process has special orphan and signal behavior.
- **Network:** separates interfaces, routes, ports, and firewall context.
- **IPC:** isolates System V IPC and related objects.
- **UTS:** separates hostname and domain name.
- **Cgroup and time:** alter cgroup visibility or selected clocks where supported.

Namespace setup has ordering constraints. A process must retain the authority to create or configure a namespace, and mount/network setup often needs to happen before dropping the corresponding capability. User namespace mappings must be written correctly before relying on the mapped identity.

## Cgroup v2

Cgroup v2 presents a unified hierarchy. Controllers can enforce or report CPU, memory, I/O, PIDs, and other resources. Important controls include memory limits and protection, CPU weight/max, I/O policy, `pids.max`, and pressure stall information. A limit is a policy decision: define what happens on reclaim, throttling, allocation failure, or OOM termination.

Use cgroups to contain blast radius and make resource ownership observable. Do not mistake `pids.max` for a complete fork-bomb defense or a memory limit for protection against all kernel memory use.

## Design questions

For each isolated service, document:

- Which objects must be visible and which must be hidden?
- Who creates the namespace and who owns its setup files?
- Is the service allowed to create children, mount filesystems, or change networking?
- What resource is being limited, at what level, and with what failure behavior?
- How will operators inspect the process from the host and from inside the namespace?

Debug from both views. `/proc`, `/sys`, device nodes, and paths may have different meanings inside an isolated environment. A diagnostic that works on the host may be unavailable in the service namespace, so preserve host-side evidence as well.

## Read the namespace and hierarchy actually in use

```sh
# Set pid to the service PID; inspection requires the relevant access.
readlink "/proc/$pid/ns/user"
readlink "/proc/$pid/ns/mnt"
readlink "/proc/$pid/ns/net"
cat "/proc/$pid/uid_map"
cat "/proc/$pid/gid_map"
cat "/proc/$pid/cgroup"
```

A user-namespace mapping such as `0 100000 65536` maps namespace UID 0
to parent UID 100000 over a range. Being root there does not grant authority
over arbitrary host resources. A mount namespace initially inherits a mount
view; creating it alone does not hide sensitive files or stop mount propagation.
Entering a PID namespace changes the namespace for subsequently created children,
so creating a child and mounting a suitable procfs are separate setup steps.

Locate the service's cgroup using its membership and the visible cgroup mount;
do not assume `/sys/fs/cgroup/memory.current` belongs to that service.
An administrator's host view and a delegated container view can differ.

## Limits form a hierarchy

In cgroup v2, a child's resource policy is constrained by its ancestors.
Controllers must be available and enabled for the relevant subtree. Delegation
grants a bounded ability to manage descendants, not unrestricted host policy.

`memory.high` causes pressure/reclaim behavior; `memory.max` imposes a
hard bound that can invoke cgroup OOM handling. `cpu.max` specifies quota
and period; `cpu.weight` affects relative share under contention.
`pids.max` counts tasks, including threads, and can make creation fail
without terminating existing workers. Observe `memory.events`, CPU
throttling statistics, and pressure while applying a representative burst.

Test the application's overload response before selecting production limits.
A useful outcome is “reject new work while preserving shutdown and diagnostics,”
not merely “the kernel eventually killed it.”
See [cgroup v2](https://docs.kernel.org/admin-guide/cgroup-v2.html).

## Related topics

- [Stage 12 overview](index.md)
- [Service sandboxing](../services-init-and-systemd/service-sandboxing-and-resource-controls.md)
