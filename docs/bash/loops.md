---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Loops

## What Problem Does This Solve?

Loops repeat work over arguments, generated values, files, input lines, or command results. In Bash, the safest loop form depends heavily on the kind of data being processed.

## Core Concepts

- `for name in list; do ... done` iterates over words already produced by the shell.
- `while command; do ... done` repeats while a command succeeds.
- `until command; do ... done` repeats until a command succeeds.
- C-style `for ((...))` loops are useful for counters.
- `break` exits a loop.
- `continue` skips to the next iteration.
- Globs can be used safely when variable parts are quoted.
- Do not use `for item in $(command)` for arbitrary text or filenames.
- Use `while IFS= read -r ...` for line-oriented input.

## Mental Model

Choose the loop based on the data source:

- Fixed words or arrays: use `for`.
- Files matched by a glob: use `for path in "$dir"/*.log`.
- Lines from a file: use `while IFS= read -r line`.
- NUL-delimited filenames: use `while IFS= read -r -d '' path`.
- Counters: use `for ((i = 0; i < n; i += 1))`.

## Syntax / API / Mechanism

Array loop:

```bash
items=("one" "two words" "")
for item in "${items[@]}"; do
    printf '<%s>\n' "$item"
done
```

Glob loop:

```bash
for path in "$dir"/*.log; do
    [[ -e "$path" ]] || continue
    printf '%s\n' "$path"
done
```

Counter loop:

```bash
for ((i = 0; i < 3; i += 1)); do
    printf '%d\n' "$i"
done
```

## Minimal Example

```bash
for arg in "$@"; do
    printf 'arg=<%s>\n' "$arg"
done
```

For a runnable demonstration, see `examples/bash/loops-demo.sh`.

## Real-World Example

Process only matching log files while handling a directory path with spaces:

```bash
log_dir=$1

for path in "$log_dir"/*.log; do
    [[ -e "$path" ]] || continue
    gzip -- "$path"
done
```

The directory variable is quoted, while the intentional glob remains unquoted.

## Common Mistakes

- Writing `for file in $(ls)` and breaking on spaces, tabs, and newlines.
- Forgetting the no-match case for globs.
- Expecting variable changes inside a pipeline-fed loop to persist afterward.
- Omitting `IFS=` and `-r` when reading lines.
- Expanding arrays as `${array[*]}` when `"${array[@]}"` is needed.
- Using a loop where a single streaming tool would be simpler and faster.

## Debugging Checklist

- Print each value as `printf '<%s>\n' "$value"`.
- Test values with spaces and empty strings.
- Test glob loops when no files match.
- Check whether a loop is running in a subshell.
- Use ShellCheck warnings as review prompts.
- Consider whether a command like `find`, `xargs -0`, `awk`, or `sed` is a better fit.

## Related Topics

- [Quoting](quoting.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Read Lines Safely](read-lines-safely.md)
- [Subshells](subshells.md)
- [Performance Boundaries](performance-boundaries.md)

## References

- [GNU Bash Reference Manual: Looping Constructs](https://www.gnu.org/software/bash/manual/html_node/Looping-Constructs.html)
- [GNU Bash Reference Manual: Arrays](https://www.gnu.org/software/bash/manual/html_node/Arrays.html)
- [GNU Bash Reference Manual: Filename Expansion](https://www.gnu.org/software/bash/manual/html_node/Filename-Expansion.html)
- [ShellCheck SC2045](https://www.shellcheck.net/wiki/SC2045)
