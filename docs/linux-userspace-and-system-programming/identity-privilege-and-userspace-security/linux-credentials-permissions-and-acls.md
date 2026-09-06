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
