---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Read Builtin

## What Problem Does This Solve?

`read` turns stdin into shell variables. It is central to line-oriented scripts, prompts, simple record parsing, NUL-delimited filename handling, and reading from custom file descriptors. Small option choices determine whether whitespace, backslashes, delimiters, and timeouts are handled correctly.

## Core Concepts

- `read` is a Bash builtin.
- `read` returns non-zero on EOF, timeout, or some input errors.
- `-r` prevents backslash escape processing and is usually the right default.
- `IFS=` before `read` preserves leading and trailing whitespace.
- `-d DELIM` reads until a custom delimiter.
- `-a ARRAY` splits input into an array.
- `-n N` reads a fixed number of characters.
- `-s` suppresses terminal echo for sensitive interactive input.
- `-p PROMPT` prints a prompt before reading.
- `-t SECONDS` times out.
- `-u FD` reads from a specific file descriptor.

## Mental Model

`read` consumes bytes from stdin and assigns fields to shell variables. It is not a general parser. Use it for simple records, lines, and delimiters; use a real parser for rich formats.

For line reading, this is the default pattern:

```bash
while IFS= read -r line; do
    printf '<%s>\n' "$line"
done < input.txt
```

## Syntax / API / Mechanism

Read one line:

```bash
IFS= read -r line
```

Read comma-separated fields:

```bash
IFS=, read -r name role rest <<< "$record"
```

Read NUL-delimited records:

```bash
while IFS= read -r -d '' path; do
    printf 'path=%s\n' "$path"
done < <(find "$root" -type f -print0)
```

Read from a custom descriptor:

```bash
exec 3< config.txt
IFS= read -r first_line <&3
exec 3<&-
```

## Minimal Example

```bash
line='alpha beta gamma'
read -r first rest <<< "$line"
printf 'first=%s rest=%s\n' "$first" "$rest"
```

For a runnable demonstration, see `examples/bash/read-builtin-demo.sh`.

## Real-World Example

Handle a final line that lacks a trailing newline:

```bash
while IFS= read -r line || [[ -n "$line" ]]; do
    printf 'line=%s\n' "$line"
done < "$input"
```

The `|| [[ -n "$line" ]]` keeps the last partial line instead of dropping it when `read` reports EOF.

## Common Mistakes

- Omitting `-r` and accidentally treating backslashes as escapes.
- Omitting `IFS=` and losing leading or trailing whitespace.
- Forgetting that `read` can return non-zero after assigning a final unterminated line.
- Reading from a pipeline and expecting loop variables to remain visible after the loop.
- Using `read -p` in scripts that may run non-interactively.
- Reading passwords without `-s` or without controlling terminal behavior.
- Using `read` to parse CSV, JSON, or other formats with nested quoting rules.

## Debugging Checklist

- Print variables with `declare -p`.
- Test empty lines and lines with leading/trailing spaces.
- Test a file whose final line has no newline.
- Test backslashes with and without `-r`.
- Use `-d ''` for NUL-delimited filename streams.
- Check whether the loop is fed by a pipe or by redirection.
- Use `-u FD` when reading from multiple streams.

## Related Topics

- [Read Lines Safely](read-lines-safely.md)
- [Word Splitting And IFS](word-splitting-and-ifs.md)
- [Redirection And Pipes](redirection-and-pipes.md)
- [Advanced File Descriptors](advanced-file-descriptors.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)

## References

- [GNU Bash Reference Manual: Bash Builtin Commands](https://www.gnu.org/software/bash/manual/html_node/Bash-Builtins.html)
- [GNU Bash Reference Manual: Redirections](https://www.gnu.org/software/bash/manual/html_node/Redirections.html)
- [GNU Findutils manual: Safe File Name Handling](https://www.gnu.org/software/findutils/manual/html_node/find_html/Safe-File-Name-Handling.html)
