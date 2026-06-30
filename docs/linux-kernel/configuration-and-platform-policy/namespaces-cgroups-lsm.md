---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Namespaces, Cgroups, And LSM Overview

## What Problem Does This Solve?

Namespaces, cgroups, and Linux Security Modules provide isolation, resource control, and security policy enforcement.

These mechanisms are often required by modern userspace stacks, container runtimes, service managers, sandboxing systems, and product security policy. Enabling the wrong subset can create confusing failures:

- containers start but have no network namespace
- systemd expects cgroup v2 controllers that are missing
- device access is blocked by cgroup or LSM policy
- SELinux or AppArmor is configured in userspace but inactive in the kernel
- user namespaces are enabled without a product threat-model review

This page is an orientation for kernel configuration and platform policy. It is not a full container or MAC policy guide.

## Core Concepts

- namespaces
- cgroups
- capabilities
- SELinux
- AppArmor
- Landlock overview
- device access control
- container runtime dependencies

## Mental Model

These features are kernel mechanisms used by userspace policy. Enable only what the product or platform actually needs, then validate the userspace stack against it.

```text
kernel config provides mechanism
command line selects or orders policy
userspace mounts/configures interfaces
runtime uses namespaces/cgroups/LSMs
product policy decides what is allowed
```

Kernel support alone does not create isolation. Userspace must configure it correctly.

## Namespaces

Namespaces isolate global kernel resources.

Common namespace types:

| Namespace | Isolates |
| --- | --- |
| mount | mount table |
| PID | process ID view |
| network | interfaces, routing tables, sockets |
| IPC | SysV IPC and POSIX message queues |
| UTS | hostname and domain name |
| user | user and group ID mappings |
| cgroup | cgroup hierarchy view |
| time | selected clock offsets |

Kernel config area:

```text
CONFIG_NAMESPACES
CONFIG_UTS_NS
CONFIG_IPC_NS
CONFIG_PID_NS
CONFIG_NET_NS
CONFIG_USER_NS
CONFIG_CGROUPS
```

Exact symbols depend on kernel version.

Runtime inspection:

```sh
ls -l /proc/self/ns
readlink /proc/self/ns/net
```

## User Namespace Policy

User namespaces are powerful because they can let unprivileged users create isolated root-like environments inside a namespace.

Questions:

- Does the product run containers that require user namespaces?
- Are unprivileged user namespaces allowed?
- Which services may create them?
- Does LSM/seccomp policy cover the intended use?
- Are kernel versions and backports reviewed for namespace-related vulnerabilities?

Do not enable user namespaces only because a desktop distro does. Embedded products should make an explicit policy choice.

## Cgroups

Cgroups organize processes and apply resource control.

Common uses:

- CPU limits
- memory limits
- I/O control
- process count limits
- service accounting
- container resource control
- freezer-like lifecycle control

Cgroup v2 is the modern unified hierarchy on many systems.

Runtime checks:

```sh
findmnt -t cgroup2
cat /sys/fs/cgroup/cgroup.controllers
cat /proc/self/cgroup
```

Systemd-based systems often expect a usable cgroup hierarchy. Container runtimes may require specific controllers.

## Cgroup Controller Policy

Common controller areas:

| Area | Purpose |
| --- | --- |
| cpu | CPU distribution and quota |
| cpuset | CPU and memory-node placement |
| memory | memory accounting and limits |
| io | block I/O control |
| pids | process count limits |
| hugetlb | huge page control |
| freezer | process freezing in supported models |
| bpf/device policy | device and network filtering hooks in some setups |

Do not enable controllers blindly. Each controller adds kernel code, accounting overhead, and userspace expectations.

## Capabilities

Linux capabilities split traditional root privilege into smaller units.

Examples:

```text
CAP_NET_ADMIN
CAP_SYS_ADMIN
CAP_SYS_MODULE
CAP_SYS_RAWIO
CAP_SYS_BOOT
```

Policy warning:

```text
CAP_SYS_ADMIN is broad. Treat it as high privilege.
```

Review services that need hardware access. Many driver debug workflows accidentally require capabilities that should not exist in production services.

## LSMs

Linux Security Modules provide security hooks used by policies such as SELinux, AppArmor, Smack, TOMOYO, Landlock, Yama, and integrity modules.

Kernel config and command line together decide what is available and active.

Runtime check:

```sh
cat /sys/kernel/security/lsm
```

Policy files and userspace setup are separate. A kernel with SELinux support but no valid policy is not a complete SELinux product.

## LSM Selection And Ordering

Review:

```text
CONFIG_SECURITY
CONFIG_SECURITY_SELINUX
CONFIG_SECURITY_APPARMOR
CONFIG_SECURITY_YAMA
CONFIG_SECURITY_LANDLOCK
CONFIG_LSM
security=
lsm=
```

The active LSM list and order matter. Verify runtime state, not only config.

Example:

```sh
cat /sys/kernel/security/lsm
dmesg | grep -i -e selinux -e apparmor -e landlock -e yama
```

## Device Access Control

Hardware access is affected by multiple layers:

```text
device node permissions
udev rules
groups
capabilities
cgroup device policy
LSM policy
seccomp filters
driver permissions
```

If a service cannot open `/dev/demo0`, check all layers before blaming the driver.

Debug sequence:

```sh
ls -l /dev/demo0
id
cat /proc/self/status | grep Cap
cat /proc/self/cgroup
dmesg | grep -i denied
```

## Container Runtime Dependencies

A container runtime may need:

- namespaces
- cgroups
- overlay filesystem
- veth and bridge networking
- netfilter/nftables
- seccomp
- capabilities
- LSM integration
- idmapped mounts or user namespaces, depending on runtime

Do not enable "container support" as one vague feature. Build a runtime dependency checklist for the actual runtime and product.

## Seccomp And BPF

Seccomp filters restrict system calls. Many container runtimes and sandboxing systems depend on it.

Review config areas:

```text
CONFIG_SECCOMP
CONFIG_SECCOMP_FILTER
CONFIG_BPF
CONFIG_BPF_SYSCALL
```

BPF has security and observability implications. Production policy should define who can load BPF programs and whether unprivileged BPF is allowed.

## Product Profiles

Example profile split:

```text
minimal appliance:
  no containers
  limited cgroups for service supervision
  one major LSM or simpler hardening profile

container host:
  namespaces enabled
  cgroup v2 controllers required by runtime
  seccomp and LSM integration
  network filtering stack

developer image:
  broader namespace and tracing support
  explicit non-production label
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| container runtime fails immediately | missing namespace/cgroup/seccomp option | runtime diagnostics |
| service has no memory limit | missing cgroup controller or wrong hierarchy | `/sys/fs/cgroup` |
| LSM policy ignored | LSM not active or policy not loaded | `/sys/kernel/security/lsm` |
| device access denied | cgroup/LSM/permissions issue | denial logs and `/dev` mode |
| unexpected privilege | capabilities too broad | `/proc/<pid>/status` |
| production attack surface too broad | developer namespace features left enabled | final `.config` review |

## Practice Exercises

### Exercise 1: Runtime Dependency Matrix

For one userspace stack, document:

```text
requires containers?
required namespaces
required cgroup version
required controllers
required LSM
required seccomp/BPF support
device access policy
```

### Exercise 2: Active Policy Check

On a running target, capture:

```sh
ls -l /proc/self/ns
findmnt -t cgroup2
cat /sys/fs/cgroup/cgroup.controllers
cat /sys/kernel/security/lsm
```

Compare runtime state with final `.config`.

### Exercise 3: Device Denial Debugging

Run a service that should access one device node. If access fails, identify which layer denied it.

## Debugging Checklist

- Check kernel config support.
- Check mount points and active controllers.
- Check audit logs or denial logs.
- Distinguish kernel support from userspace policy configuration.
- Check active LSM list and order.
- Check capabilities and seccomp status.
- Check cgroup hierarchy version.
- Check runtime-specific requirements, not generic container claims.

## Related Topics

- [Kernel Command Line Policy](kernel-command-line-policy.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Embedded Productization](../../embedded-productization/index.md)
- [Module Signing And Hardening](module-signing-and-hardening.md)

## Official References

- [Namespaces](https://docs.kernel.org/admin-guide/namespaces/index.html)
- [Control Group v2](https://docs.kernel.org/admin-guide/cgroup-v2.html)
- [Linux Security Module Usage](https://docs.kernel.org/admin-guide/LSM/index.html)
- [The kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
