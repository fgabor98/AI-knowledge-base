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
