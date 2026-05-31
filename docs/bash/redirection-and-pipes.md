---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Redirection And Pipes

## What Problem Does This Solve?

Redirection and pipes connect commands to files, devices, and other commands. They are the main reason Bash is useful as glue code, but small ordering mistakes can silently send output to the wrong place or hide failures.

## Core Concepts

- Standard input is file descriptor `0`.
- Standard output is file descriptor `1`.
- Standard error is file descriptor `2`.
- `>` redirects stdout to a file, replacing it.
- `>>` appends stdout to a file.
- `<` redirects stdin from a file.
- `2>` redirects stderr.
- `2>&1` duplicates stderr to wherever stdout currently points.
- A pipe connects stdout of one command to stdin of the next command.
- Pipeline status needs special care; see `pipefail` and `PIPESTATUS`.

## Mental Model

Redirection changes where file descriptors point before the command runs. Ordering matters because `2>&1` copies the current destination of stdout at that point in the command line.

These are different:

```bash
command >out.log 2>&1
command 2>&1 >out.log
```

The first sends stdout and stderr to `out.log`. The second sends stderr to the old stdout, then sends stdout to `out.log`.

## Syntax / API / Mechanism

Common forms:

```bash
command >out.txt
command >>out.txt
command <in.txt
command 2>err.txt
command >out.txt 2>err.txt
command >combined.log 2>&1
command &>combined.log
```

Pipes:

```bash
producer | filter | consumer
```

Read from a command without a pipeline subshell:

```bash
while IFS= read -r line; do
    printf '<%s>\n' "$line"
done < input.txt
```

Process substitution:

```bash
diff <(sort old.txt) <(sort new.txt)
```

Process substitution is Bash-specific and requires OS support such as `/dev/fd` or named pipes.

## Minimal Example

```bash
if command >out.log 2>err.log; then
    printf 'command succeeded\n'
else
    printf 'command failed; see err.log\n' >&2
fi
```

For a runnable demonstration, see `examples/bash/redirection-demo.sh`.

## Real-World Example

Capture stdout as data while preserving stderr for diagnostics:

```bash
if output=$(git rev-parse --show-toplevel); then
    repo_root=$output
else
    printf 'not inside a Git repository\n' >&2
    exit 1
fi
```

This captures stdout into `output`. Error messages from `git` still go to stderr, where a human or CI log can see them.

## Common Mistakes

- Reversing redirection order with `2>&1 >file`.
- Capturing stderr as data accidentally.
- Forgetting that each pipeline segment may run in a subshell.
- Assuming a pipeline failed when only the last command's status was checked.
- Using `cat file | while read ...` when input redirection would avoid a pipeline.
- Overwriting files with `>` when `>>` was intended.
- Assuming `&>` is portable. It is Bash-specific.

## Debugging Checklist

- Reduce redirections to stdout only, then add stderr handling.
- Use `set -o pipefail` when upstream pipeline failure matters.
- Inspect `PIPESTATUS` immediately after a pipeline.
- Print to stderr with `printf 'message\n' >&2`.
- Use temporary files when debugging complex descriptor flows.
- Check whether a loop is fed by a pipe and therefore runs in a subshell.

## Related Topics

- [Exit Codes](exit-codes.md)
- [Subshells](subshells.md)
- [Read Lines Safely](read-lines-safely.md)
- [Robust Scripts](robust-scripts.md)

## References

- `man bash`, sections `REDIRECTION` and `PIPELINES`
- GNU Bash Reference Manual, sections `Redirections` and `Pipelines`
