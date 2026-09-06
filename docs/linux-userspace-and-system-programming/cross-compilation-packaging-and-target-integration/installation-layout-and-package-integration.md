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
