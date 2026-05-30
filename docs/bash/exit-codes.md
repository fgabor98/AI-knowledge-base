---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Exit Codes

## What Problem Does This Solve?

Bash control flow is based on command exit status. Scripts become reliable when they treat exit codes as part of the interface of every command they call.

Exit status is how a script decides whether to continue, retry, report a failure, skip work, or branch into another path.

## Core Concepts

- Exit status `0` means success.
- Non-zero exit status means failure or another command-specific condition.
- `$?` contains the exit status of the most recent foreground pipeline.
- `if command; then` tests the command's exit status directly.
- `&&` runs the right-hand command only if the left-hand command succeeds.
- `||` runs the right-hand command only if the left-hand command fails.
- A pipeline normally returns the status of its last command.
- `set -o pipefail` makes a pipeline return the rightmost non-zero status when any command in the pipeline fails.
- `PIPESTATUS` contains the status of each command in the most recent foreground pipeline.
- Functions return the status of their last command unless they use `return`.

## Mental Model

In Bash, commands are truth tests. A command does not produce a boolean value for control flow. It exits with a status code, and Bash interprets `0` as true and non-zero as false in condition positions.

Prefer this:

```bash
if grep -q -- "needle" "$file"; then
    printf 'found\n'
fi
```

over this:

```bash
grep -q -- "needle" "$file"
if [ "$?" -eq 0 ]; then
    printf 'found\n'
fi
```

The first form is shorter and avoids accidentally checking the wrong command.

## Syntax / API / Mechanism

Common status patterns:

```bash
command
status=$?
```

Use `$?` immediately. Any simple command after the command you care about can overwrite it.

```bash
if command; then
    printf 'success\n'
else
    printf 'failure\n'
fi
```

Use `PIPESTATUS` immediately after a pipeline:

```bash
grep -r -- "needle" src | wc -l
status=$? parts=("${PIPESTATUS[@]}")

printf 'pipeline status: %d\n' "$status"
printf 'grep status: %d\n' "${parts[0]}"
printf 'wc status: %d\n' "${parts[1]}"
```

Important special cases:

- `grep` returns `0` for match, `1` for no match, and greater than `1` for an error.
- `test`, `[`, and `[[` return `0` when the condition is true and non-zero when it is false.
- `! command` inverts success and failure for control flow.
- Exit statuses are stored in the shell as values from `0` to `255`.
- Commands terminated by signals are commonly reported as `128 + signal_number`.

## Minimal Example

```bash
dir=$1

if mkdir -- "$dir"; then
    printf 'created: %s\n' "$dir"
else
    status=$?
    printf 'mkdir failed with status %d\n' "$status" >&2
    exit "$status"
fi
```

For runnable demonstrations, see:

- `examples/bash/exit-code-demo.sh`
- `examples/bash/pipefail-demo.sh`

## Real-World Example

Handle `grep` correctly when "not found" is expected but I/O errors are not:

```bash
if grep -q -- "$pattern" "$file"; then
    printf 'pattern found\n'
else
    status=$?
    case "$status" in
        1)
            printf 'pattern not found\n'
            ;;
        *)
            printf 'grep failed with status %d\n' "$status" >&2
            exit "$status"
            ;;
    esac
fi
```

This matters because a missing pattern and an unreadable file are different outcomes.

## Common Mistakes

- Checking `$?` after another command has already overwritten it.
- Assuming every non-zero status means the same kind of failure.
- Forgetting that a pipeline returns the last command's status by default.
- Using `cmd || true` and hiding failures that should be handled explicitly.
- Relying on `set -e` as the only error handling strategy.
- Writing `local output=$(command)` and accidentally masking the command substitution status in some cases.
- Ignoring command-specific status conventions such as `grep` returning `1` for no match.

## Debugging Checklist

- Print the status immediately after the command you care about.
- For pipelines, inspect both `$?` and `PIPESTATUS`.
- Turn on `set -o pipefail` when pipeline failures should fail the script.
- Check the called command's manual page for status meanings.
- Test expected failures as well as success paths.
- Avoid putting logging or assignment commands between a command and its status check.
- Use `set -x` temporarily to see the command path that led to the status.

## Related Topics

- [Quoting](quoting.md)
- [Robust Scripts](robust-scripts.md)
- `redirection-and-pipes.md`
- `conditionals.md`

## References

- `man bash`, sections `EXIT STATUS`, `PIPELINES`, and `SHELL BUILTIN COMMANDS`
- GNU Bash Reference Manual, sections `Exit Status` and `Pipelines`
- POSIX Shell Command Language, section on command exit status
- `grep(1)`, section `EXIT STATUS`
