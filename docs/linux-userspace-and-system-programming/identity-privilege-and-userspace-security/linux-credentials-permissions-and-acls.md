---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Linux Credentials, Permissions, And ACLs

## What a process identity contains

Linux tracks real, effective, and saved-set user and group IDs. The real ID identifies the owner of the process; the effective IDs are normally used by permission checks; the saved IDs support controlled transitions such as a set-user-ID program temporarily dropping and regaining privilege. Supplementary groups add membership used during group permission checks.

Inspect the complete picture rather than only calling `getuid()`:

```c
uid_t r = getuid(), e = geteuid();
gid_t gr = getgid(), ge = getegid();
/* getresuid/getresgid and getgroups expose the saved and supplementary sets. */
```

`/proc/self/status` is convenient for diagnostics, but avoid treating its text format as a stable application protocol. Log numeric IDs and the resolved account name separately when identity matters.

## Permission checks

For a pathname, the kernel walks each directory and checks search (`x`) permission before checking the final object. The final check considers the effective UID, supplementary groups, mode bits, ACLs, mount flags, and security modules. Root-like privilege is not simply “UID zero”: capabilities can grant individual powers, while an LSM may still deny an operation.

Useful diagnostic commands are:

```sh
namei -l /srv/example/config.toml
stat -c '%A %a %U:%G %n' /srv/example/config.toml
getfacl -p /srv/example/config.toml
```

Use a dedicated service account, a private state directory, and explicit ownership. Do not make a whole tree world-writable to solve one access problem.

## Creation policy and transitions

`umask` removes permission bits at object creation; it does not fix permissions after creation and does not apply to every metadata operation. Set a deliberate umask early, but still pass explicit modes to `open`, `mkdir`, and temporary-file APIs. Be careful with inherited directory setgid bits, sticky directories, and default ACLs.

Set-user-ID and set-group-ID executables are compatibility mechanisms with a large attack surface. If they are unavoidable, minimize the privileged code path, validate all inherited state, close unexpected descriptors, and make the privilege transition explicit.

## Common mistakes

- Checking access with `access()` and opening later creates a time-of-check/time-of-use race.
- Comparing only `getuid()` misses effective privilege.
- Assuming a parent directory is safe while an attacker controls a path component is incorrect.
- Replacing an ACL or mode problem with `chmod -R 777` destroys the security boundary.
- Logging account names without numeric IDs loses precision when identity databases change.

Use `openat()` relative to a trusted directory, directory file descriptors, and—where available—`openat2()` resolution constraints for security-sensitive path traversal.

## Work through an access decision

Linux filesystem checks use filesystem UID/GID, which normally track the
effective IDs. The four numeric columns in `/proc/PID/status` for Uid/Gid
are real, effective, saved, and filesystem IDs. Most services should leave
filesystem IDs tracking the normal effective identity.

For `/srv/data/config`, directory search permission is required at every
component. Reading a directory lists names; searching traverses known names.
Deleting or renaming a file primarily requires permissions on its containing
directory, not write access to its contents. The sticky bit adds ownership
restrictions to deletion in shared directories.

Without a default ACL, creation mode is filtered by umask:
`0666 & ~0027 = 0640`. A directory default ACL changes the creation
algorithm, and the requested mode still limits permissions. An ACL mask caps
the effective permissions of named users and the group class. A named user
entry showing `rw-` with a read-only mask does not grant effective write.
Inspect `getfacl`, including its effective-permission comments, before
changing ownership. See [acl(5)](https://man7.org/linux/man-pages/man5/acl.5.html)
and [umask(2)](https://man7.org/linux/man-pages/man2/umask.2.html).

## Descriptor authority and revocation

Changing pathname permissions does not generally revoke existing open
descriptors. A helper that inherited a writable FD can retain authority after
dropping its UID or after an administrator changes the file mode. Inventory
descriptors as part of privilege reduction, and use an explicit service protocol
when access must be revoked during operation.

For a diagnostic exercise, draw the directory permissions, identify the
service's supplementary groups, then predict open, read, create, rename and
unlink results separately. Confirm them in a private test directory under the
actual service identity.

## Related topics

- [Stage 12 overview](index.md)
- [Service sandboxing](../services-init-and-systemd/service-sandboxing-and-resource-controls.md)
