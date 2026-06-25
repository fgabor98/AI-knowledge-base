---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Errexit And ERR Traps

## What Problem Does This Solve?

`set -e`, `ERR`, and `set -E` are often used to make scripts fail fast, but their rules are subtle. This page explains where failure is automatic, where it is suppressed, and how to write explicit error handling when the default behavior is not enough.

## Core Concepts

- `set -e` enables `errexit`.
- `errexit` does not apply uniformly in every syntactic context.
- Commands used by `if`, `while`, `until`, `&&`, `||`, and `!` are common suppression contexts.
- `trap 'handler' ERR` runs a handler before the shell exits because of an uncaught command failure.
- `set -E` enables `errtrace`, which lets `ERR` traps propagate into functions, command substitutions, and subshell environments.
- `pipefail` changes which pipeline failures matter to `errexit`.
- Explicit `if command; then ... else ... fi` remains clearer for expected failure.

## Mental Model

Treat `set -e` as a last-resort guard, not as your main error handling strategy. Use explicit status checks for failures you expect and want to interpret. Use `errexit` for unexpected failures that should stop the script.

## Syntax / API / Mechanism

Common strict-mode base:

```bash
set -Eeuo pipefail
trap 'printf "failed at line %d\n" "$LINENO" >&2' ERR
```

Suppression example:

```bash
if grep -q -- "$pattern" "$file"; then
    printf 'found\n'
else
    printf 'not found or grep failed\n'
fi
```

The `grep` failure does not trigger `errexit` because it is the condition of an `if`.

## Minimal Example

```bash
set -Eeuo pipefail
trap 'printf "ERR at line %d\n" "$LINENO" >&2' ERR

false
printf 'this line is not reached\n'
```

For a runnable demonstration that does not abort your shell, see `examples/bash/errexit-err-trap-demo.sh`.

## Real-World Example

Use explicit handling for an expected condition and let unexpected failures stop the script:

```bash
set -Eeuo pipefail

if grep -q -- "$pattern" "$file"; then
    printf 'pattern exists\n'
elif [[ $? -eq 1 ]]; then
    printf 'pattern missing\n'
else
    printf 'grep failed\n' >&2
    exit 1
fi

cp -- "$file" "$backup_dir/"
```

The `grep` result is interpreted. The `cp` failure is unexpected and can be handled by `errexit`.

## Common Mistakes

- Assuming `set -e` triggers on every non-zero status.
- Expecting `ERR` traps to propagate into functions without `set -E`.
- Hiding important failures with `cmd || true`.
- Using `set -e` around commands whose non-zero statuses are meaningful data.
- Forgetting `pipefail`, so upstream pipeline failure is ignored.
- Writing a trap that fails under `set -u` because variables are unset.

## Debugging Checklist

- Reduce the failing command to a small script.
- Check whether the command is inside `if`, `while`, `&&`, `||`, `!`, or a pipeline.
- Temporarily print `BASH_COMMAND`, `LINENO`, and `FUNCNAME` in the `ERR` trap.
- Test the same failure with and without `set -E`.
- Use explicit status handling for expected non-zero statuses.
- Run ShellCheck and inspect warnings about masked or ignored failures.

## Related Topics

- [Exit Codes](exit-codes.md)
- [Robust Scripts](robust-scripts.md)
- [Advanced Debugging And Tracing](advanced-debugging-and-tracing.md)
- [Redirection And Pipes](redirection-and-pipes.md)

## References

- [GNU Bash Reference Manual: The Set Builtin](https://www.gnu.org/software/bash/manual/html_node/The-Set-Builtin.html)
- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
- [GNU Bash Reference Manual: Exit Status](https://www.gnu.org/software/bash/manual/html_node/Exit-Status.html)
- [GNU Bash Reference Manual: Pipelines](https://www.gnu.org/software/bash/manual/html_node/Pipelines.html)
