---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Portability Matrix

## What Problem Does This Solve?

Bash scripts often move between developer machines, CI images, embedded root filesystems, and production systems. Portability work identifies which shell and userland features are actually available before scripts depend on them.

## Core Concepts

- Bash version matters.
- `/bin/sh` may be `dash`, BusyBox `ash`, Bash, or another shell.
- GNU and BSD userland tools differ.
- macOS historically ships old Bash versions.
- Embedded systems may omit Bash or full GNU Coreutils.
- `#!/usr/bin/env bash` depends on `env` and `PATH`.
- Scripts should document their required shell and external commands.

## Mental Model

Portability is a target matrix, not a feeling. List the environments that matter and test against them.

## Syntax / API / Mechanism

Check Bash version:

```bash
printf 'Bash %s\n' "$BASH_VERSION"
printf 'major=%s minor=%s\n' "${BASH_VERSINFO[0]}" "${BASH_VERSINFO[1]}"
```

Gate a feature:

```bash
if ((BASH_VERSINFO[0] < 4)); then
    printf 'Bash 4 or newer is required\n' >&2
    exit 2
fi
```

Check command behavior:

```bash
command -v timeout >/dev/null 2>&1 || printf 'timeout missing\n' >&2
```

## Minimal Example

```bash
if [[ -z "${BASH_VERSION:-}" ]]; then
    printf 'this script requires Bash\n' >&2
    exit 2
fi
```

For a runnable demonstration, see `examples/bash/portability-matrix-demo.sh`.

## Real-World Example

Document script requirements:

```bash
require_bash_4() {
    if ((BASH_VERSINFO[0] < 4)); then
        printf 'requires Bash 4 or newer\n' >&2
        exit 2
    fi
}

need_command find
need_command mktemp
```

This makes deployment failures explicit.

## Common Mistakes

- Testing only on one developer workstation.
- Declaring `#!/bin/sh` while using Bash arrays.
- Assuming GNU options exist on BusyBox or BSD tools.
- Using associative arrays on Bash 3.2.
- Assuming `timeout`, `readlink -f`, or GNU `sed -i` exists everywhere.
- Forgetting that embedded systems often have reduced toolsets.

## Debugging Checklist

- Record `bash --version`.
- Record `/bin/sh --version` when supported.
- Run scripts in the target container or root filesystem.
- List external commands with `command -v`.
- Check every non-POSIX option used by external commands.
- Add feature gates near script startup.

## Related Topics

- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)
- [Associative Arrays](associative-arrays.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)
- [Performance Boundaries](performance-boundaries.md)

## References

- [GNU Bash Reference Manual: Bash POSIX Mode](https://www.gnu.org/software/bash/manual/html_node/Bash-POSIX-Mode.html)
- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
- [POSIX.1-2024: Utility Conventions](https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/basedefs/V1_chap12.html)
