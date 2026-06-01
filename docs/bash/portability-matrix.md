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

Common targets:

| Target | Typical Constraint |
| --- | --- |
| Developer Linux | Often has recent Bash and GNU userland, but scripts may accidentally depend on local tools. |
| macOS / BSD userland | Bash may be older, and many tools use BSD options instead of GNU options. |
| CI container | Usually predictable, but only if the image is pinned and documented. |
| BusyBox / embedded Linux | Shell and utilities may be reduced or built with optional features disabled. |
| POSIX `/bin/sh` | Portable baseline, but excludes most Bash conveniences. |

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

## Shell Feature Matrix

Use this table to decide whether a script can rely on a shell feature. "Test" means the feature may exist in some builds or versions, but the script should probe it directly before depending on it.

| Feature | Bash 3.2 | Bash 4+ | POSIX `sh` | BusyBox `ash` |
| --- | --- | --- | --- | --- |
| Indexed arrays | Yes | Yes | No | No |
| Associative arrays | No | Yes | No | No |
| `[[ ... ]]` | Yes | Yes | No | Do not rely |
| `(( ... ))` arithmetic command | Yes | Yes | No | Do not rely |
| `$(( ... ))` arithmetic expansion | Yes | Yes | Yes | Usually |
| `set -o pipefail` | Yes | Yes | No | Test |
| Process substitution `<(...)` | Yes | Yes | No | No |
| `mapfile` / `readarray` | No | Yes | No | No |
| `declare -n` namerefs | No | Bash 4.3+ | No | No |
| `wait -n` | No | Bash 4.3+ | No | Test |
| `globstar` | No | Yes | No | No |
| `extglob` | Yes | Yes | No | Do not rely |

## Userland Tool Matrix

Shell portability is only half the problem. Many Bash scripts are really Bash plus `find`, `sed`, `grep`, `stat`, `date`, and Coreutils.

| Operation | GNU/Linux | BSD/macOS | BusyBox / Embedded | Safer Pattern |
| --- | --- | --- | --- | --- |
| In-place `sed` | `sed -i` or `sed -i.bak` | `sed -i ''` for no backup | Varies by build | Write to a temp file, then `mv`. |
| Perl regex grep | `grep -P` often available | Usually unavailable | Usually unavailable | Use `grep -E`, `awk`, `perl`, or a real parser. |
| File metadata | `stat -c '%s' file` | `stat -f '%z' file` | Options vary | Avoid parsing metadata unless target-specific. |
| Canonical path | `readlink -f path` | Often unavailable | Sometimes available | Use `realpath` if required and probed. |
| Date parsing | `date -d '...'` | Different `date` flags | Limited formats | Avoid portable date parsing in shell. |
| Timeout command | `timeout` from Coreutils | Often `gtimeout` if installed separately | Applet may be absent | Probe with `command -v timeout`. |
| NUL-safe find | `find -print0` | Usually available | Usually available if enabled | Prefer `find -print0` plus `read -d ''` in Bash. |
| NUL-safe xargs | `xargs -0` | Usually available | Usually available if enabled | Prefer arrays or `while read -d ''` when possible. |

When a script must run across these environments, document the tested matrix near the script or in project docs. If the matrix is too wide, simplify the shell script or move the logic to a language with better standard-library behavior.

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

For external command behavior, check the exact option you need rather than only checking that the command exists:

```bash
if ! find . -maxdepth 0 -print0 >/dev/null 2>&1; then
    printf 'find -print0 is required\n' >&2
    exit 2
fi
```

## Common Mistakes

- Testing only on one developer workstation.
- Declaring `#!/bin/sh` while using Bash arrays.
- Assuming GNU options exist on BusyBox or BSD tools.
- Using associative arrays on Bash 3.2.
- Assuming `timeout`, `readlink -f`, or GNU `sed -i` exists everywhere.
- Forgetting that embedded systems often have reduced toolsets.
- Checking for a command name but not the specific option behavior the script uses.
- Writing "portable Bash" when the actual requirement is POSIX `sh`.

## Debugging Checklist

- Record `bash --version`.
- Record `/bin/sh --version` when supported.
- Run scripts in the target container or root filesystem.
- List external commands with `command -v`.
- Check every non-POSIX option used by external commands.
- Add feature gates near script startup.
- Test on a pinned macOS/BSD or BusyBox target when those environments matter.
- Keep a small compatibility script or CI job for the target matrix.

## Related Topics

- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)
- [Associative Arrays](associative-arrays.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)
- [Performance Boundaries](performance-boundaries.md)

## References

- [GNU Bash Reference Manual: Bash POSIX Mode](https://www.gnu.org/software/bash/manual/html_node/Bash-POSIX-Mode.html)
- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
- [POSIX.1-2024: Utility Conventions](https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/basedefs/V1_chap12.html)
