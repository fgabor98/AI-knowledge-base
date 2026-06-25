---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Traps And Cleanup

## What Problem Does This Solve?

Traps let a Bash script run cleanup code when it exits or receives signals. They are essential for removing temporary files, releasing locks, restoring terminal state, and making interrupted scripts predictable.

## Core Concepts

- `trap 'commands' EXIT` runs commands when the shell exits.
- `trap 'commands' INT TERM` handles interruption and termination signals.
- Cleanup handlers must tolerate partially initialized state.
- Store temporary paths in variables initialized before the trap can run.
- Use `mktemp` for temporary files and directories.
- Preserve exit status inside cleanup when needed.
- Avoid complex cleanup strings; prefer a cleanup function.
- `EXIT` is not a signal. It is a Bash trap point.

## Mental Model

Register cleanup early, then make cleanup idempotent. A cleanup function should be safe to call even if setup only partly completed.

```bash
tmpdir=
cleanup() {
    if [[ -n "${tmpdir:-}" && -d "$tmpdir" ]]; then
        rm -rf -- "$tmpdir"
    fi
}
trap cleanup EXIT
```

## Syntax / API / Mechanism

Basic cleanup:

```bash
tmpdir=$(mktemp -d)
trap 'rm -rf -- "$tmpdir"' EXIT
```

Preferred cleanup function:

```bash
tmpdir=

cleanup() {
    local status=$?
    if [[ -n "${tmpdir:-}" && -d "$tmpdir" ]]; then
        rm -rf -- "$tmpdir"
    fi
    return "$status"
}
trap cleanup EXIT
```

Signal handling:

```bash
on_term() {
    printf 'terminated\n' >&2
    exit 143
}
trap on_term INT TERM
```

Exit status `143` commonly represents termination by signal 15: `128 + 15`.

## Minimal Example

```bash
tmpdir=$(mktemp -d)
trap 'rm -rf -- "$tmpdir"' EXIT

printf 'work dir: %s\n' "$tmpdir"
```

For a runnable demonstration, see `examples/bash/trap-cleanup.sh`.

## Real-World Example

Use a lock directory and clean it up:

```bash
lockdir=${TMPDIR:-/tmp}/my-script.lock

if ! mkdir -- "$lockdir"; then
    printf 'another instance is already running\n' >&2
    exit 1
fi

cleanup() {
    rmdir -- "$lockdir"
}
trap cleanup EXIT
```

Creating a directory is atomic on local POSIX filesystems. This is a common simple lock pattern, though more advanced locking may require `flock`.

## Common Mistakes

- Registering a trap after creating temporary files.
- Using `rm -rf "$tmpdir"` when `tmpdir` may be empty or unset.
- Writing complex quoted trap strings that expand at registration time unexpectedly.
- Assuming `EXIT` traps always run after `kill -9`. They do not.
- Forgetting to preserve or intentionally set the final exit status.
- Cleanup code that fails under `set -u` because variables are unset.
- Installing a trap in a subshell and expecting it to affect the parent shell.

## Debugging Checklist

- Force an early failure and verify cleanup happens.
- Press Ctrl-C during a test run and inspect temporary files.
- Print cleanup actions to stderr while testing.
- Initialize cleanup variables before registering the trap.
- Run with `set -u` to catch unset cleanup variables.
- Check whether cleanup is registered in the parent shell or a subshell.

## Related Topics

- [Robust Scripts](robust-scripts.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)
- [Exit Codes](exit-codes.md)
- [Subshells](subshells.md)

## References

- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
- [GNU Bash Reference Manual: Exit Status](https://www.gnu.org/software/bash/manual/html_node/Exit-Status.html)
- [GNU Coreutils manual: mktemp invocation](https://www.gnu.org/software/coreutils/manual/html_node/mktemp-invocation.html)
