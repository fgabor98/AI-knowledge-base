---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Conditionals

## What Problem Does This Solve?

Conditionals let a Bash script branch based on command success, file state, strings, numbers, and pattern matches. They are reliable when you remember that Bash conditions are usually commands returning exit status.

## Core Concepts

- `if command; then` branches on command exit status.
- `[[ ... ]]` is Bash's safer conditional expression syntax.
- `[ ... ]` is the `test` command syntax and is more portable but more fragile.
- `(( ... ))` evaluates arithmetic expressions.
- `case` is useful for options, modes, and pattern dispatch.
- String comparison and numeric comparison use different operators.
- Pattern matching in `[[ ... ]]` is not the same as regular expressions.
- Regex matching uses `=~` inside `[[ ... ]]`.

## Mental Model

An `if` statement does not require brackets:

```bash
if grep -q -- "$pattern" "$file"; then
    printf 'found\n'
fi
```

`[[ ... ]]`, `[ ... ]`, `grep`, `mkdir`, and functions are all commands from the point of view of `if`.

## Syntax / API / Mechanism

String checks:

```bash
[[ -z "$value" ]]       # empty string
[[ -n "$value" ]]       # non-empty string
[[ "$a" == "$b" ]]      # equal strings
[[ "$name" == *.log ]]  # pattern match
```

File checks:

```bash
[[ -e "$path" ]]  # exists
[[ -f "$path" ]]  # regular file
[[ -d "$path" ]]  # directory
[[ -r "$path" ]]  # readable
[[ -x "$path" ]]  # executable or searchable
```

Arithmetic checks:

```bash
if ((count > 10)); then
    printf 'large count\n'
fi
```

Case dispatch:

```bash
case "$mode" in
    start|stop|restart)
        printf 'mode=%s\n' "$mode"
        ;;
    *)
        printf 'unknown mode: %s\n' "$mode" >&2
        exit 2
        ;;
esac
```

## Minimal Example

```bash
path=$1

if [[ -d "$path" ]]; then
    printf 'directory: %s\n' "$path"
elif [[ -f "$path" ]]; then
    printf 'file: %s\n' "$path"
else
    printf 'not a regular file or directory: %s\n' "$path"
fi
```

For a runnable demonstration, see `examples/bash/conditionals-demo.sh`.

## Real-World Example

Option parsing with `case`:

```bash
verbose=0

while (($#)); do
    case "$1" in
        -v|--verbose)
            verbose=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        -*)
            printf 'unknown option: %s\n' "$1" >&2
            exit 2
            ;;
        *)
            break
            ;;
    esac
done
```

This is clearer than deeply nested `if` statements for mode and option dispatch.

## Common Mistakes

- Writing `if [ grep ... ]` instead of `if grep ...`.
- Using string operators for numbers, for example `[[ "$n" > 10 ]]`.
- Forgetting spaces around `[` arguments.
- Using `[ "$a" == "$b" ]` in scripts that must be POSIX portable. POSIX uses `=`.
- Quoting the right-hand side of `[[ "$name" == *.log ]]` when a pattern match is intended.
- Treating `[[ "$text" =~ "$regex" ]]` as regex matching. Quoting the regex makes it literal in many practical cases.

## Debugging Checklist

- Print the values being compared before the condition.
- Check whether you need string, arithmetic, pattern, or regex comparison.
- Use `if command; then` directly for command success checks.
- Use `case` for option and mode dispatch.
- Run ShellCheck for warnings around condition syntax.
- Test empty values and values beginning with `-`.

## Related Topics

- [Exit Codes](exit-codes.md)
- [Variables And Expansion](variables-and-expansion.md)
- [Quoting](quoting.md)
- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)

## References

- `man bash`, sections `CONDITIONAL EXPRESSIONS`, `Compound Commands`, and `Pattern Matching`
- GNU Bash Reference Manual, sections `Bash Conditional Expressions` and `Conditional Constructs`
