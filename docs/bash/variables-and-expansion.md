---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Variables And Expansion

## What Problem Does This Solve?

Variables and expansion are how Bash turns script data into command arguments. Correct expansion is what makes scripts handle defaults, optional values, arrays, filenames, and user input without accidental word splitting or globbing.

## Core Concepts

- Assignments have no spaces around `=`.
- Shell variables are strings unless used in arithmetic or arrays.
- `local` limits a variable to a function scope.
- `export` makes a variable visible to child processes.
- `${var}` expands a variable.
- `${var:-default}` supplies a default when unset or empty.
- `${var:?message}` fails when a required value is missing.
- `"${array[@]}"` expands array elements as separate arguments.
- Command substitution captures stdout and strips trailing newlines.
- Arithmetic expansion uses `(( ... ))` or `$(( ... ))`.

## Mental Model

Expansion is not variable lookup alone. Bash expands a command line in phases. If you expand a variable without quotes, the resulting text can be split into multiple words and then treated as glob patterns.

Prefer to think in terms of command arguments:

```bash
args=(grep -n -- "$pattern" "$file")
"${args[@]}"
```

The array stores arguments as arguments. It does not require rebuilding a command string.

## Syntax / API / Mechanism

Common parameter expansion forms:

| Form | Meaning |
| --- | --- |
| `${var}` | Expand `var`. |
| `${var:-default}` | Use `default` if `var` is unset or empty. |
| `${var-default}` | Use `default` if `var` is unset. Empty still counts as set. |
| `${var:?message}` | Print message and fail if `var` is unset or empty. |
| `${var#prefix}` | Remove shortest matching prefix pattern. |
| `${var##prefix}` | Remove longest matching prefix pattern. |
| `${var%suffix}` | Remove shortest matching suffix pattern. |
| `${var%%suffix}` | Remove longest matching suffix pattern. |
| `${#var}` | Length of the value. |
| `${array[@]}` | Expand all array elements. Quote it in normal use. |

Array basics:

```bash
files=("one.txt" "two words.txt" "")
printf '<%s>\n' "${files[@]}"
```

Arithmetic:

```bash
count=0
((count += 1))
printf '%d\n' "$count"
```

## Minimal Example

```bash
name=${1:-world}
printf 'hello, %s\n' "$name"
```

For a runnable demonstration, see `examples/bash/expansion-demo.sh`.

## Real-World Example

Build command arguments with an array:

```bash
pattern=$1
file=$2
ignore_case=${IGNORE_CASE:-0}

cmd=(grep -n -- "$pattern" "$file")

if [[ "$ignore_case" == 1 ]]; then
    cmd=(grep -in -- "$pattern" "$file")
fi

"${cmd[@]}"
```

This avoids constructing a single string such as `cmd="grep -n $pattern $file"`, which is fragile and often leads to quoting bugs or unsafe `eval`.

## Common Mistakes

- Writing `name = value`, which Bash parses as a command named `name`.
- Using `$var` unquoted when the value is data.
- Treating arrays as portable POSIX shell.
- Using `cmd="$cmd --flag"` instead of an array for command arguments.
- Expecting command substitution to preserve trailing newlines.
- Using `${var:-default}` when an explicitly empty string should be preserved.
- Forgetting that `${var:?message}` exits a non-interactive shell.

## Debugging Checklist

- Use `declare -p var` or `declare -p array` to inspect variables accurately.
- Print arguments with `printf '<%s>\n' "$@"`.
- Use `set -u` to catch accidental unset variable use.
- Test unset, empty, and whitespace-containing values.
- Use ShellCheck warnings around SC2086, SC2154, and SC2206 as review triggers.
- Avoid `eval` unless there is a specific, reviewed reason.

## Related Topics

- [Execution Model](execution-model.md)
- [Quoting](quoting.md)
- [Conditionals](conditionals.md)
- [Robust Scripts](robust-scripts.md)

## References

- `man bash`, sections `PARAMETERS`, `EXPANSION`, `Shell Parameters`, and `Arrays`
- GNU Bash Reference Manual, sections `Shell Parameter Expansion`, `Arrays`, and `Shell Arithmetic`
