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
