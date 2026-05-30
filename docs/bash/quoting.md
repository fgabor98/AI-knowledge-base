---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Quoting

## What Problem Does This Solve?

Quoting controls whether text stays as one argument after Bash performs expansion. It is the difference between a script that handles real filenames and user input correctly and a script that breaks on spaces, empty values, wildcard characters, or values that begin with `-`.

Most Bash bugs are not syntax errors. They are cases where a value unexpectedly becomes several words, disappears because it is empty, or expands as a glob pattern.

## Core Concepts

- Single quotes preserve text literally. Nothing expands inside them.
- Double quotes allow parameter expansion, command substitution, and arithmetic expansion, but suppress word splitting and pathname expansion on the expanded result.
- Unquoted parameter expansion is usually unsafe because Bash applies word splitting and pathname expansion afterward.
- `"$var"` preserves a scalar value as one argument, even when it is empty.
- `"${array[@]}"` preserves each array element as a separate argument.
- `"$@"` preserves each positional argument as a separate argument.
- Globs such as `*.log` must remain unquoted when you want pathname expansion.
- Quote variable parts of a path and leave intentional glob patterns unquoted, for example `"$dir"/*.log`.

## Mental Model

Bash does not pass the text you wrote directly to the command. It parses the command line, performs expansions, removes quotes, and then executes the command with a final argument vector.

A useful mental model is:

1. Write the command line.
2. Bash expands variables, command substitutions, arithmetic expressions, and globs.
3. Bash splits unquoted expansion results into words.
4. Bash removes quotes.
5. The command receives the final arguments.

Quoting is how you tell Bash which parts should survive expansion as data.

## Syntax / API / Mechanism

Common forms:

| Form | Meaning |
| --- | --- |
| `'literal text'` | Preserve everything literally until the next single quote. |
| `"text $var"` | Allow expansion, but preserve the result as part of the same word. |
| `\$` | Preserve a single special character with a backslash. |
| `"$var"` | Pass one scalar value as one argument. |
| `"${array[@]}"` | Pass each array element as its own argument. |
| `"$@"` | Pass each positional parameter as its own argument. |
| `$'line\n'` | Bash ANSI-C quoting; useful for escapes such as newline or tab. |

Use `--` before path arguments when the receiving command supports it:

```bash
rm -- "$path"
cp -- "$src" "$dst"
```

This prevents a value such as `-rf` from being interpreted as an option.

## Minimal Example

```bash
value="two words"

printf '<%s>\n' $value
printf '<%s>\n' "$value"
```

The first command passes two arguments: `two` and `words`.

The second command passes one argument: `two words`.

For a runnable demonstration, see `examples/bash/quoting-demo.sh`.

## Real-World Example

Process every `.log` file in a directory while supporting spaces in the directory name:

```bash
#!/usr/bin/env bash
set -euo pipefail

src_dir=$1
dst_dir=$2

mkdir -p -- "$dst_dir"

for path in "$src_dir"/*.log; do
    [ -e "$path" ] || continue
    cp -- "$path" "$dst_dir/"
done
```

The variable part is quoted: `"$src_dir"` and `"$dst_dir"`.

The glob part is not quoted: `*.log`.

If the whole expression were written as `"$src_dir/*.log"`, the `*` would be literal and no log files would be matched.

## Common Mistakes

- Writing `cp $src $dst` and breaking on paths containing spaces or glob characters.
- Writing `for file in $(ls)` and breaking on whitespace, newlines, and unusual filenames.
- Writing `"$dir/*.log"` and accidentally disabling the intended glob.
- Using `$*` when `"$@"` is needed for forwarding arguments.
- Forgetting that command substitution removes trailing newlines.
- Using `echo` to inspect values instead of `printf`, which makes empty and unusual values harder to see.
- Treating Bash quoting rules as if they were C, Python, or Make quoting rules.

## Debugging Checklist

- Reproduce the issue with a path containing spaces.
- Reproduce the issue with an empty value.
- Reproduce the issue with a value containing `*`, `?`, or `[`.
- Print arguments with `printf '<%s>\n' "$@"` instead of `echo`.
- Use `printf '%q\n' "$value"` to see a shell-escaped representation.
- Run ShellCheck and inspect warnings such as SC2086 and SC2046.
- Add `set -x` temporarily to see the command after expansion.

## Related Topics

- [Exit Codes](exit-codes.md)
- [Robust Scripts](robust-scripts.md)
- `variables-and-expansion.md`
- `redirection-and-pipes.md`

## References

- `man bash`, sections `QUOTING` and `EXPANSION`
- GNU Bash Reference Manual, sections `Quoting` and `Shell Expansions`
- POSIX Shell Command Language, sections on quoting and word expansion
- ShellCheck rules SC2086 and SC2046
