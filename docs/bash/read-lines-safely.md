---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Read Lines Safely

## What Problem Does This Solve?

Reading text line by line is common in automation scripts. Doing it incorrectly loses whitespace, treats backslashes specially, breaks on missing final newlines, or fails on filenames with unusual characters.

## Core Concepts

- Use `IFS= read -r line` for text lines.
- `IFS=` prevents trimming leading and trailing IFS whitespace.
- `-r` prevents backslash escapes from being interpreted.
- Use `while ...; do ...; done < file` to avoid pipeline subshell surprises.
- Handle the final line even when it has no trailing newline.
- Use NUL-delimited records for filenames when possible.
- `readarray` and `mapfile` are Bash-specific helpers.

## Mental Model

Text lines and filenames are different data types.

For ordinary text files, newline-delimited records are reasonable:

```bash
while IFS= read -r line; do
    printf '<%s>\n' "$line"
done < input.txt
```

For filenames, newline-delimited text is not fully safe because filenames may contain newlines. Prefer NUL delimiters:

```bash
find "$dir" -type f -print0 |
while IFS= read -r -d '' path; do
    printf '<%s>\n' "$path"
done
```

## Syntax / API / Mechanism

Read every line, including a final line without a newline:

```bash
while IFS= read -r line || [[ -n "$line" ]]; do
    printf '%s\n' "$line"
done < input.txt
```

Read NUL-delimited records:

```bash
while IFS= read -r -d '' path; do
    printf '%s\n' "$path"
done < paths.bin
```

Load lines into a Bash array:

```bash
mapfile -t lines < input.txt
printf '<%s>\n' "${lines[@]}"
```

`mapfile` is Bash-specific and can be memory-heavy for large files.

## Minimal Example

```bash
while IFS= read -r line || [[ -n "$line" ]]; do
    printf 'line=<%s>\n' "$line"
done < "$file"
```

For a runnable demonstration, see `examples/bash/read-lines-safely.sh`.

## Real-World Example

Process files from `find` safely:

```bash
find "$root" -type f -name '*.log' -print0 |
while IFS= read -r -d '' path; do
    gzip -- "$path"
done
```

This handles spaces, tabs, quotes, and newlines in filenames because records are separated by NUL bytes.

If the loop needs to update variables used after the loop, avoid the pipeline:

```bash
count=0
while IFS= read -r -d '' path; do
    ((count += 1))
    printf '%s\n' "$path"
done < <(find "$root" -type f -print0)
printf 'count=%d\n' "$count"
```

Process substitution is Bash-specific.

## Common Mistakes

- Writing `for line in $(cat file)` and losing whitespace.
- Omitting `-r` and treating backslashes as escapes.
- Omitting `IFS=` and trimming leading or trailing whitespace.
- Forgetting a final line without a newline.
- Reading filenames as newline-delimited text.
- Expecting variables changed inside a pipeline-fed loop to persist afterward.
- Using `mapfile` for very large input without considering memory.

## Debugging Checklist

- Test lines with leading spaces, trailing spaces, tabs, and backslashes.
- Test a file whose final line has no newline.
- Use `printf '<%s>\n' "$line"` to inspect exact values.
- Use NUL-delimited input for filenames.
- Check whether the loop is running in a subshell.
- Run ShellCheck and inspect read-loop warnings.

## Related Topics

- [Redirection And Pipes](redirection-and-pipes.md)
- [Subshells](subshells.md)
- [Quoting](quoting.md)
- [Robust Scripts](robust-scripts.md)

## References

- [GNU Bash Reference Manual: Bash Builtin Commands](https://www.gnu.org/software/bash/manual/html_node/Bash-Builtins.html)
- [GNU Bash Reference Manual: Redirections](https://www.gnu.org/software/bash/manual/html_node/Redirections.html)
- [GNU Findutils manual: Safe File Name Handling](https://www.gnu.org/software/findutils/manual/html_node/find_html/Safe-File-Name-Handling.html)
- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
