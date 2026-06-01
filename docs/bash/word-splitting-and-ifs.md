---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Word Splitting And IFS

## What Problem Does This Solve?

Word splitting is one of the main reasons shell scripts break on spaces, tabs, and newlines. `IFS`, the internal field separator, controls some splitting behavior and is useful with `read`, but changing it globally can make scripts fragile.

## Core Concepts

- Word splitting happens after unquoted parameter, command, and arithmetic expansion.
- Quoted expansions are not word-split.
- Filename expansion can happen after word splitting.
- `IFS` controls which characters split words in some contexts.
- The default `IFS` contains space, tab, and newline.
- `IFS= read -r line` preserves leading and trailing whitespace when reading lines.
- Localize `IFS` changes to one command or a small scope.
- Use arrays when you need a list of arguments.
- Do not parse arbitrary data by relying on default word splitting.

## Mental Model

Unquoted expansion asks Bash to reinterpret data as shell syntax-like words. Quoted expansion passes data as one argument.

```bash
value='two words'
printf '<%s>\n' "$value"  # one argument
printf '<%s>\n' $value    # two arguments
```

The second form is rarely what you want for data. It is sometimes useful for deliberate splitting, but that case should be obvious and reviewed.

## Syntax / API / Mechanism

Default-safe line read:

```bash
while IFS= read -r line; do
    printf '<%s>\n' "$line"
done < input.txt
```

Split a simple delimiter-separated record with `read`:

```bash
IFS=, read -r name role rest <<< "$record"
```

Use an array for command arguments:

```bash
args=(grep -n -- "$pattern" "$file")
"${args[@]}"
```

Localize `IFS` for one command:

```bash
IFS=: read -r user _ uid _ <<< "$passwd_entry"
```

## Minimal Example

```bash
value='one two'

printf 'quoted:\n'
printf '<%s>\n' "$value"

printf 'split deliberately:\n'
IFS=' ' read -r first second <<< "$value"
printf 'first=%s second=%s\n' "$first" "$second"
```

For a runnable demonstration, see `examples/bash/ifs-word-splitting-demo.sh`.

## Real-World Example

Read colon-delimited account data without changing global shell behavior:

```bash
while IFS=: read -r user _ uid gid gecos home shell; do
    printf 'user=%s uid=%s home=%s shell=%s\n' "$user" "$uid" "$home" "$shell"
done < /etc/passwd
```

The `IFS=:` assignment applies to the `read` command. It does not permanently change splitting for the rest of the script.

## Common Mistakes

- Expanding variables unquoted and relying on "normal" filenames.
- Setting `IFS` globally near the top of a script.
- Using `for item in $items` for data that may contain whitespace.
- Treating command output as a safe list of arguments.
- Forgetting that unquoted splitting can be followed by glob expansion.
- Using `read line` without `-r`, which treats backslashes specially.
- Expecting shell word splitting to parse CSV, JSON, or other structured formats correctly.

## Debugging Checklist

- Print arguments with `printf '<%s>\n' "$@"`.
- Use `declare -p var array` to inspect values.
- Test values containing spaces, tabs, newlines, and glob characters.
- Run ShellCheck and inspect SC2086, SC2046, and SC2206.
- Prefer arrays for argument lists.
- Prefer `read` with explicit `IFS` for simple record splitting.
- Use a real parser for structured formats.

## Related Topics

- [Quoting](quoting.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Globbing And Shell Options](globbing-and-shell-options.md)
- [Read Lines Safely](read-lines-safely.md)
- [Read Builtin](read-builtin.md)

## References

- [GNU Bash Reference Manual: Word Splitting](https://www.gnu.org/software/bash/manual/html_node/Word-Splitting.html)
- [GNU Bash Reference Manual: Shell Expansions](https://www.gnu.org/software/bash/manual/html_node/Shell-Expansions.html)
- [GNU Bash Reference Manual: Bash Builtin Commands](https://www.gnu.org/software/bash/manual/html_node/Bash-Builtins.html)
- [ShellCheck SC2086](https://www.shellcheck.net/wiki/SC2086)
- [ShellCheck SC2046](https://www.shellcheck.net/wiki/SC2046)
- [ShellCheck SC2206](https://www.shellcheck.net/wiki/SC2206)
