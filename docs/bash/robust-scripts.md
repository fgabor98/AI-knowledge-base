---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Robust Scripts

## What Problem Does This Solve?

A robust Bash script behaves predictably when input is missing, paths contain unusual characters, commands fail, or cleanup is required. It makes failure visible and controlled instead of continuing with corrupted assumptions.

This topic is about practical script structure, not just adding `set -euo pipefail` to the top of a file.

## Core Concepts

- Choose an explicit Bash shebang when the script uses Bash features.
- Use strict options deliberately, and understand their limitations.
- Validate arguments before doing work.
- Quote expansions by default.
- Use arrays for lists of arguments.
- Keep work inside `main "$@"`.
- Use small helper functions such as `usage`, `die`, and `log`.
- Create temporary files with `mktemp` and clean them with `trap`.
- Check dependencies with `command -v`.
- Preserve meaningful exit statuses.
- Run ShellCheck on every non-trivial script.

## Mental Model

A Bash script is glue around commands. Robustness comes from making the glue explicit:

- what inputs are accepted
- what commands are required
- what files may be created
- what happens on failure
- what cleanup is guaranteed
- what status is returned to the caller

Treat external commands as APIs. Their arguments, output, and exit statuses are part of your script's contract.

## Syntax / API / Mechanism

A common starting point:

```bash
#!/usr/bin/env bash
set -Eeuo pipefail

usage() {
    printf 'Usage: %s SOURCE_DIR\n' "${0##*/}"
}

log() {
    printf '%s\n' "$*" >&2
}

die() {
    log "error: $*"
    exit 1
}

main() {
    (($# == 1)) || {
        usage >&2
        exit 2
    }

    local source_dir=$1
    [[ -d "$source_dir" ]] || die "not a directory: $source_dir"
}

main "$@"
```

Important notes:

- `set -e` does not fire in every context. It is ignored in several conditional and compound-command positions.
- `set -u` makes unset variables errors, so use `${var:-}` when an unset value is allowed.
- `pipefail` changes pipeline status handling and is usually appropriate for scripts where upstream command failure matters.
- `set -E` makes an `ERR` trap inherit into functions, command substitutions, and subshells.
- `trap` handlers must tolerate partially initialized variables.

## Minimal Example

```bash
#!/usr/bin/env bash
set -euo pipefail

(($# == 1)) || {
    printf 'Usage: %s INPUT\n' "${0##*/}" >&2
    exit 2
}

tmpdir=$(mktemp -d)
cleanup() {
    rm -rf -- "$tmpdir"
}
trap cleanup EXIT

input=$1
[[ -r "$input" ]] || {
    printf 'not readable: %s\n' "$input" >&2
    exit 1
}

cp -- "$input" "$tmpdir/input"
printf 'copied input to temporary workspace\n'
```

For a runnable template, see `examples/bash/robust-script-template.sh`.

## Real-World Example

Find regular files in a source directory and process them one path at a time:

```bash
#!/usr/bin/env bash
set -Eeuo pipefail

src_dir=$1

[[ -d "$src_dir" ]] || {
    printf 'not a directory: %s\n' "$src_dir" >&2
    exit 1
}

find "$src_dir" -maxdepth 1 -type f -print0 |
while IFS= read -r -d '' path; do
    printf 'processing: %s\n' "$path"
done
```

This handles spaces and most unusual characters in filenames because it uses NUL-delimited records rather than whitespace-delimited text.

One caveat: the `while` loop is in a pipeline, so in Bash it runs in a subshell unless `lastpipe` is enabled in a non-interactive shell. Do not expect variables changed inside this loop to be visible after the loop.

## Common Mistakes

- Adding `set -e` and assuming every error is now handled.
- Using Bash syntax with a `#!/bin/sh` shebang.
- Expanding unquoted variables in filesystem operations.
- Parsing `ls` output.
- Building command strings and running them with `eval`.
- Forgetting to clean up temporary files on failure.
- Ignoring empty input, missing arguments, or unreadable files.
- Using global variables where function arguments would be clearer.
- Treating a list of arguments as a string instead of an array.

## Debugging Checklist

- Run the script with paths containing spaces.
- Run the script with missing, empty, and invalid arguments.
- Run ShellCheck and handle every warning deliberately.
- Run with `bash -n script.sh` to catch syntax errors.
- Add `set -x` temporarily for execution tracing.
- Check whether a failing command is inside a context where `set -e` is disabled.
- Verify cleanup by forcing an early failure.
- Verify the script's final exit status in success and failure cases.

## Related Topics

- [Quoting](quoting.md)
- [Exit Codes](exit-codes.md)
- `traps-and-cleanup.md`
- `read-lines-safely.md`
- `bash-vs-posix-sh.md`

## References

- [GNU Bash Reference Manual: Invoking Bash](https://www.gnu.org/software/bash/manual/html_node/Invoking-Bash.html)
- [GNU Bash Reference Manual: The Set Builtin](https://www.gnu.org/software/bash/manual/html_node/The-Set-Builtin.html)
- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
- [GNU Bash Reference Manual: Bash Conditional Expressions](https://www.gnu.org/software/bash/manual/html_node/Bash-Conditional-Expressions.html)
- [ShellCheck wiki](https://www.shellcheck.net/wiki/Home)
- [POSIX.1-2024: Utility Conventions](https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/basedefs/V1_chap12.html)
