---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Functions

## What Problem Does This Solve?

Functions let Bash scripts name repeated behavior, isolate argument handling, and keep the main flow readable. They are also the practical unit for logging, validation, cleanup, and small reusable command wrappers.

## Core Concepts

- Bash functions run in the current shell process.
- Function arguments are read through `$1`, `$2`, and `"$@"` inside the function.
- `local` variables are scoped to a function and its children.
- `return` sets a function's exit status.
- A function without `return` returns the status of its last command.
- Output should usually go to stdout only when it is data.
- Logs and errors should usually go to stderr.
- Prefer passing values as arguments over relying on globals.

## Mental Model

A Bash function is a named command. It should be called and checked like any other command:

```bash
if ensure_directory "$path"; then
    printf 'ready\n'
fi
```

Use return status for success or failure. Use stdout only when the caller needs to capture data.

## Syntax / API / Mechanism

Function definition:

```bash
name() {
    printf 'hello\n'
}
```

Arguments:

```bash
copy_file() {
    local src=$1
    local dst=$2

    cp -- "$src" "$dst"
}
```

Returning status:

```bash
is_readable_file() {
    local path=$1
    [[ -f "$path" && -r "$path" ]]
}
```

Producing data:

```bash
basename_without_ext() {
    local path=$1
    local name=${path##*/}
    printf '%s\n' "${name%.*}"
}
```

## Minimal Example

```bash
die() {
    printf 'error: %s\n' "$*" >&2
    exit 1
}

[[ -d "$1" ]] || die "not a directory: $1"
```

For a runnable demonstration, see `examples/bash/functions-demo.sh`.

## Real-World Example

Dependency checking:

```bash
need_command() {
    command -v "$1" >/dev/null 2>&1 || {
        printf 'missing command: %s\n' "$1" >&2
        return 1
    }
}

main() {
    need_command git
    need_command mktemp
    need_command tar
}

main "$@"
```

This keeps dependency checks readable and testable.

## Common Mistakes

- Forgetting `local` and accidentally modifying a global variable.
- Returning strings with `return`. Bash return statuses are numeric.
- Capturing log output because a function writes logs to stdout.
- Relying on global positional parameters instead of passing function arguments.
- Using `function name()` syntax when simple `name()` is enough.
- Assuming `local` exists in POSIX shell. It is common but not specified by POSIX.
- Letting a failing command become the function's accidental return status.

## Debugging Checklist

- Print function arguments with `printf '<%s>\n' "$@"`.
- Use `declare -p var` to inspect local and global variables.
- Check function status with `if function_name ...; then`.
- Keep logs on stderr so command substitution captures only data.
- Run ShellCheck for warnings around unused locals and masked return statuses.
- Use `set -x` around the function call when tracing is needed.

## Related Topics

- [Robust Scripts](robust-scripts.md)
- [Exit Codes](exit-codes.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)

## References

- `man bash`, sections `SHELL FUNCTIONS`, `Shell Parameters`, and `SHELL BUILTIN COMMANDS`
- GNU Bash Reference Manual, section `Shell Functions`
