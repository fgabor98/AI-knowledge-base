---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Eval, Injection, And Shell Security

## What Problem Does This Solve?

Shell scripts often sit at trust boundaries: CI variables, filenames, device names, command-line options, and environment values. This page explains why `eval` and command strings are dangerous and how to construct commands without turning data into code.

## Core Concepts

- Data and code must stay separate.
- `eval` parses a string as shell code.
- Untrusted input must never be interpreted as shell syntax.
- Arrays are the right way to build commands with dynamic arguments.
- Quote expansions unless you intentionally want shell splitting or globbing.
- Use `--` before untrusted path arguments when supported.
- Treat environment variables as untrusted at script boundaries.
- Avoid putting secrets into command arguments where `ps` may expose them.

## Mental Model

If a value can influence shell syntax, it can become code. The safe pattern is to keep each command argument as a separate array element and execute the array directly.

## Syntax / API / Mechanism

Unsafe:

```bash
cmd="grep -n $pattern $file"
eval "$cmd"
```

Safer:

```bash
cmd=(grep -n -- "$pattern" "$file")
"${cmd[@]}"
```

Safe path handling:

```bash
rm -- "$path"
```

Environment validation:

```bash
case "${MODE:-}" in
    start|stop|status) ;;
    *) printf 'invalid MODE\n' >&2; exit 2 ;;
esac
```

## Minimal Example

```bash
pattern=$1
file=$2
cmd=(grep -n -- "$pattern" "$file")
"${cmd[@]}"
```

For a runnable demonstration, see `examples/bash/eval-injection-demo.sh`.

## Real-World Example

Build an `ssh` command safely:

```bash
host=$1
remote_path=$2

ssh_args=(-o BatchMode=yes -- "$host" ls -ld -- "$remote_path")
ssh "${ssh_args[@]}"
```

This still requires validating `host` for your environment, but it does not concatenate shell code locally.

## Common Mistakes

- Using `eval` to handle quoting problems created earlier.
- Building command strings instead of argument arrays.
- Passing untrusted values to `sh -c`.
- Expanding unquoted variables in `rm`, `cp`, `mv`, or `ssh`.
- Forgetting that filenames may begin with `-`.
- Logging secrets or passing secrets as command-line arguments.
- Trusting CI or deployment environment variables without validation.

## Debugging Checklist

- Ask whether any variable is being parsed as shell syntax.
- Replace command strings with arrays.
- Add tests with spaces, quotes, semicolons, globs, and leading dashes.
- Print arguments with `printf '<%s>\n' "${cmd[@]}"`.
- Run ShellCheck and review SC2086, SC2046, and eval-related warnings.
- Review secret handling in logs, environment, and process arguments.

## Related Topics

- [Quoting](quoting.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)
- [Advanced File Descriptors](advanced-file-descriptors.md)

## References

- [GNU Bash Reference Manual: Shell Expansions](https://www.gnu.org/software/bash/manual/html_node/Shell-Expansions.html)
- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
- [ShellCheck SC2086](https://www.shellcheck.net/wiki/SC2086)
- [ShellCheck SC2046](https://www.shellcheck.net/wiki/SC2046)
