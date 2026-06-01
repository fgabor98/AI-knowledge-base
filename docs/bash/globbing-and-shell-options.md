---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Globbing And Shell Options

## What Problem Does This Solve?

Globbing is how Bash turns patterns such as `*.log` into matching filenames. Shell options control important details of that behavior. Understanding them prevents scripts from accidentally processing literal patterns, skipping hidden files, walking too many directories, or accepting an unsafe no-match case.

## Core Concepts

- Globs are filename patterns expanded by the shell before a command runs.
- Globs are not regular expressions.
- Quote data, but leave intentional glob patterns unquoted.
- A no-match glob stays literal by default.
- `nullglob` makes no-match globs disappear.
- `failglob` makes no-match globs an error.
- `globstar` makes `**` recurse through directories.
- `extglob` enables extended pattern forms such as `@(a|b)` and `!(*.tmp)`.
- `dotglob` lets `*` match names beginning with `.`.
- `shopt` enables and disables Bash-specific shell options.

## Mental Model

A glob is expanded into a list of pathnames before the command receives its arguments:

```bash
printf '<%s>\n' *.txt
```

If `a.txt` and `b.txt` exist, `printf` receives two arguments. If no `.txt` files exist, Bash normally passes the literal string `*.txt`.

That default is useful interactively, but dangerous in scripts. Decide explicitly whether a missing match should mean "empty list" or "error".

## Syntax / API / Mechanism

Inspect options:

```bash
shopt nullglob failglob globstar extglob dotglob
```

Enable an option:

```bash
shopt -s nullglob
```

Disable an option:

```bash
shopt -u nullglob
```

Common patterns:

| Pattern | Meaning |
| --- | --- |
| `*.log` | Names ending in `.log` in the current directory. |
| `src/*.c` | `.c` files directly under `src`. |
| `**/*.md` | Markdown files recursively when `globstar` is enabled. |
| `@(start|stop)` | Either `start` or `stop` when `extglob` is enabled. |
| `!(*.tmp)` | Anything except names ending in `.tmp` when `extglob` is enabled. |

Common options:

| Option | Effect | Typical Use |
| --- | --- | --- |
| `nullglob` | No-match patterns expand to nothing. | Optional file lists. |
| `failglob` | No-match patterns cause an expansion error. | Required file lists. |
| `globstar` | `**` recursively matches directories. | Tree searches without `find`. |
| `extglob` | Enables extended glob operators. | Pattern-based filtering. |
| `dotglob` | Wildcards match dotfiles. | Include hidden files deliberately. |

## Minimal Example

```bash
shopt -s nullglob

files=(*.txt)
for file in "${files[@]}"; do
    printf 'text file: %s\n' "$file"
done
```

Without `nullglob`, an empty directory would make the loop process the literal string `*.txt`.

For a runnable demonstration, see `examples/bash/globbing-options-demo.sh`.

## Real-World Example

Require at least one input file before running a batch operation:

```bash
shopt -s nullglob

inputs=("$input_dir"/*.csv)

((${#inputs[@]} > 0)) || {
    printf 'no CSV files found in %s\n' "$input_dir" >&2
    exit 1
}

for input in "${inputs[@]}"; do
    process_csv "$input"
done
```

This avoids both dangerous no-match literals and unquoted word splitting.

## Common Mistakes

- Quoting an intentional glob, for example `"*.log"`, and expecting it to expand.
- Leaving a no-match glob to become a literal filename argument.
- Using `for file in $(ls)` instead of globs, `find`, or arrays.
- Enabling `dotglob` globally and accidentally including hidden files later.
- Assuming `**` recurses without enabling `globstar`.
- Treating globs as regexes.
- Forgetting that `extglob` is Bash-specific and must be enabled before parsing some patterns.

## Debugging Checklist

- Print matches with `printf '<%s>\n' pattern`.
- Check `shopt` state before relying on special glob behavior.
- Test an empty directory.
- Test filenames containing spaces, newlines, and leading dashes.
- Store matches in an array before looping.
- Prefer `failglob` when a missing match should stop the script.
- Prefer `find -print0` for large recursive filesystem walks or complex predicates.

## Related Topics

- [Quoting](quoting.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Loops](loops.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)
- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)

## References

- [GNU Bash Reference Manual: Filename Expansion](https://www.gnu.org/software/bash/manual/html_node/Filename-Expansion.html)
- [GNU Bash Reference Manual: Pattern Matching](https://www.gnu.org/software/bash/manual/html_node/Pattern-Matching.html)
- [GNU Bash Reference Manual: The Shopt Builtin](https://www.gnu.org/software/bash/manual/html_node/The-Shopt-Builtin.html)
- [GNU Bash Reference Manual: Bash Conditional Expressions](https://www.gnu.org/software/bash/manual/html_node/Bash-Conditional-Expressions.html)
