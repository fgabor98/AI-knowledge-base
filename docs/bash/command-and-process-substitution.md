---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Command And Process Substitution

## What Problem Does This Solve?

Command substitution captures a command's output as text. Process substitution connects a command's output or input to another command as if it were a file. These features are powerful, but they are easy to misuse when output contains whitespace, trailing newlines, large data, or meaningful exit statuses.

## Core Concepts

- `$(command)` runs `command` and substitutes its stdout.
- Command substitution strips trailing newline characters.
- Quote command substitutions when the output is data.
- Command substitution runs in a subshell environment.
- The exit status of an assignment with command substitution is the substitution command's status.
- Process substitution uses `<(command)` or `>(command)`.
- Process substitution passes a path such as `/dev/fd/N` or a named pipe to another command.
- Process substitution is Bash-specific and depends on operating system support.
- Use pipelines or temporary files when process substitution is unavailable or too opaque.

## Mental Model

Command substitution turns stdout into a string:

```bash
root=$(git rev-parse --show-toplevel)
```

Process substitution turns a running command into something another command can open:

```bash
diff <(sort old.txt) <(sort new.txt)
```

Use command substitution when you need a small text value. Use process substitution when a command expects a filename or when you want to compare or join streams without creating named temporary files.

## Syntax / API / Mechanism

Command substitution:

```bash
value=$(command arg)
printf '<%s>\n' "$value"
```

Prefer `$(...)` over legacy backticks because it nests cleanly and is easier to read.

Preserve status:

```bash
if output=$(grep -n -- "$pattern" "$file"); then
    printf 'matched: %s\n' "$output"
else
    status=$?
    printf 'grep failed or did not match: %d\n' "$status" >&2
fi
```

Process substitution:

```bash
comm -12 <(sort left.txt) <(sort right.txt)
```

Output process substitution:

```bash
printf 'important log\n' > >(logger -t my-script)
```

## Minimal Example

```bash
line_count=$(wc -l < "$file")
printf '%s has %s lines\n' "$file" "$line_count"
```

For a runnable demonstration, see `examples/bash/substitution-demo.sh`.

## Real-World Example

Compare two generated file lists without writing intermediate files:

```bash
diff -u \
    <(find "$old_root" -type f -print | sort) \
    <(find "$new_root" -type f -print | sort)
```

For filenames that may contain newlines, prefer NUL-delimited tools where possible:

```bash
comm -z -12 \
    <(find "$old_root" -type f -print0 | sort -z) \
    <(find "$new_root" -type f -print0 | sort -z)
```

The NUL-delimited version depends on GNU-style options and is not portable to every userland.

## Common Mistakes

- Leaving `$(command)` unquoted and accidentally splitting or globbing its output.
- Capturing large streams into a variable instead of using a pipeline, file, or process substitution.
- Expecting command substitution to preserve trailing newlines.
- Ignoring the command's exit status after capture.
- Using command substitution for command output that is not line-oriented text.
- Assuming process substitution works in POSIX `sh`.
- Forgetting that process substitution failures may be less visible than normal command failures.

## Debugging Checklist

- Print captured values with `printf '<%s>\n' "$value"`.
- Check `$?` immediately after command substitution when status matters.
- Use `declare -p value` to inspect captured strings.
- Avoid command substitution around data that may contain NUL bytes.
- Replace process substitution with temporary files when debugging complex failures.
- Test scripts under the exact target shell before using process substitution.
- Use ShellCheck warnings around unquoted command substitution as review triggers.

## Related Topics

- [Execution Model](execution-model.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Redirection And Pipes](redirection-and-pipes.md)
- [Subshells](subshells.md)
- [Portability Matrix](portability-matrix.md)

## References

- [GNU Bash Reference Manual: Command Substitution](https://www.gnu.org/software/bash/manual/html_node/Command-Substitution.html)
- [GNU Bash Reference Manual: Process Substitution](https://www.gnu.org/software/bash/manual/html_node/Process-Substitution.html)
- [GNU Bash Reference Manual: Shell Expansions](https://www.gnu.org/software/bash/manual/html_node/Shell-Expansions.html)
- [GNU Bash Reference Manual: Redirections](https://www.gnu.org/software/bash/manual/html_node/Redirections.html)
- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
