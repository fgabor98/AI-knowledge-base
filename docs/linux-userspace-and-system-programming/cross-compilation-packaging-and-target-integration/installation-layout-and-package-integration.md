---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Installation Layout And Package Integration

## Stage before installing

Install into a staging root such as `DESTDIR`, then inspect the complete file list. Separate immutable program files from configuration, mutable state, cache, logs, sockets, and runtime directories. Make the ownership and lifecycle of each path explicit.

Typical policy questions are:

- Which files are replaced on upgrade and which persist?
- Who owns the service user, group, socket, and state directory?
- Which paths are writable, and with what modes or ACLs?
- Are configuration changes preserved, merged, or replaced?
- What happens when a directory is missing, read-only, or full?

Packages should declare dependencies rather than assuming a development host's contents. Include service units, tmpfiles rules, sysusers definitions, capabilities, default configuration, migration hooks, and uninstall behavior only when they are required and auditable.

Validate the staged package for duplicate ownership, unexpected setuid bits, world-writable paths, broken symlinks, missing interpreters, and files outside the intended prefix. Then install it into a clean target image and exercise boot, upgrade, removal, and rollback paths.

## Prefix and staging root are different

The installation prefix is the runtime layout; `DESTDIR` is an additional
build-time staging root. For an application configured with prefix `/usr`,
a staging root `/tmp/package-root` receives
`/tmp/package-root/usr/bin/app`. The binary must not embed
`/tmp/package-root` as its runtime prefix.

An illustrative package layout is:

```text
/usr/bin/app                       executable
/usr/share/app/defaults.toml       immutable defaults
/etc/app/config.toml               administrator configuration
/usr/lib/.../system/app.service    vendor unit, using distribution path variables
/var/lib/app/                      persistent state, created by managed policy
/run/app/                         runtime sockets, created at service start
```

Avoid shipping an active socket, PID file, generated secret or development
machine state. Decide whether mutable directories are package-owned or created
by the init system, and verify the same policy on a read-only rootfs.

## Upgrade and removal semantics

Configuration preservation is package-manager specific. Mark configuration
appropriately and define merge/conflict behavior; do not overwrite local
changes through an unconditional install hook. Removal may retain state while
purge removes it, depending on the platform and product policy.

Installation scripts can run against an offline root directory during image
construction. They must not accidentally start the host service or assume the
target is booted. Keep scripts idempotent and use supported integration classes.

Inspect the package file list, ownership, modes, interpreters, dependencies and
unexpected build paths. Test installation in a clean image, upgrade from the
old package with modified configuration, and restart using preserved state.

## Related topics

- [Stage 15 overview](index.md)
- [ELF and loader diagnosis](../diagnostics-debugging-and-performance/elf-abi-and-loader-diagnostics.md)
