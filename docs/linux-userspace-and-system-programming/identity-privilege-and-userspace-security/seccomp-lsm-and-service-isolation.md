---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Seccomp, LSM, And Service Isolation

## Seccomp

Seccomp filters restrict system calls and can inspect the syscall number and selected arguments. A practical policy starts from observed behavior, then removes unnecessary calls and chooses explicit actions for violations: kill, trap, log, or return an error. `no_new_privs` is commonly required before an unprivileged process installs a filter.

Syscall filtering is not a complete policy language. A permitted syscall may have dangerous argument combinations, and alternate syscalls may reach similar functionality. Keep the filter close to the program's stable runtime contract, and test startup, normal operation, reload, crash reporting, and shutdown.

## LSM policy

Linux Security Modules provide mandatory or path/label/operation-based policy. SELinux uses labels and domains, AppArmor uses profiles and path rules, and Landlock lets an application restrict its own future filesystem/network access where supported. The enforcement decision may be visible through audit logs rather than the application's error alone.

When diagnosing denial, collect the application errno, kernel/audit record, effective identity, path labels/profile, namespace, and capability state. “Permission denied” can be a mode-bit, ACL, capability, mount, namespace, or LSM result.

## Layering

A robust service boundary may combine:

- a dedicated UID and private groups;
- read-only or narrowly writable filesystem paths;
- namespace and cgroup restrictions;
- a capability bounding set and `no_new_privs`;
- seccomp after startup requirements are known;
- an LSM profile for mandatory policy;
- input validation and a narrow IPC protocol.

Each layer needs a failure-mode test. Verify that an expected denial is logged usefully, does not leave partial state, and does not cause an unsafe fallback. Avoid copying a generic syscall allowlist or sandbox profile without tracing the exact binary, libc, loader, architecture, and optional features.

## What a syscall filter can inspect

A seccomp BPF filter sees syscall metadata and argument values, not arbitrary
memory behind user pointers. Allowing `openat` only for a particular pathname
cannot be implemented by comparing its pointer value. Use a filesystem/LSM
policy for that requirement. Check the syscall architecture as well as number;
different ABIs can assign different meanings to a number.
See [seccomp filtering](https://docs.kernel.org/userspace-api/seccomp_filter.html).

Install restrictions before untrusted execution. In a threaded program,
applying a filter to one thread does not automatically cover siblings:
use a supported synchronization mechanism and treat failure as startup failure.
A trace of the successful path misses recovery, DNS, locale, allocator, and
loader paths. Test the deployed libc and update combinations too.

## A denial investigation

Suppose opening a sensor returns `EACCES`. First establish the actual node
and directory permissions from the service namespace. Then inspect groups,
device restrictions, capabilities and active LSM policy. If an audit denial
names a profile/domain, fix the narrow rule or deployment label only after
confirming the operation belongs in the service contract.

For a seccomp violation, distinguish a delivered `SIGSYS`, a returned
policy errno, and a logged but permitted call. Log-mode evidence is not proof
of enforcement. Landlock also requires querying the supported ABI and handling
missing features deliberately; requested restrictions vary by kernel version.
Layering is useful because a syscall allowlist and a filesystem policy constrain
different aspects of the same operation.

## Related topics

- [Stage 12 overview](index.md)
- [Service sandboxing](../services-init-and-systemd/service-sandboxing-and-resource-controls.md)
