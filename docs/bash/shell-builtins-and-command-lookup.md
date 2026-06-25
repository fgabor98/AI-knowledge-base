---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Shell Builtins And Command Lookup

## What Problem Does This Solve?

Bash can run reserved words, aliases, functions, builtins, hashed commands, and external programs. Command lookup explains what actually runs when a script or terminal command says `printf`, `test`, `cd`, `echo`, or `grep`.

## Core Concepts

- Some commands are Bash builtins and run inside the current shell.
- Builtins such as `cd`, `read`, `alias`, `export`, and `ulimit` must affect the current shell.
- Functions can override external command names.
- `type` explains how Bash resolves a command name.
- `command` bypasses shell functions during lookup.
- `builtin` runs a Bash builtin directly.
- `hash` caches external command locations.
- `enable` can disable or re-enable some Bash builtins.
- Scripts should avoid relying on aliases.

## Mental Model

Command lookup is part of Bash execution. Before Bash can run a simple command, it decides what kind of command it is.

Rough lookup order:

1. reserved words
2. aliases in interactive contexts where alias expansion is enabled
3. functions
4. builtins
5. hashed or `PATH`-resolved external commands

## Syntax / API / Mechanism

Inspect names:

```bash
type cd
type printf
type grep
type -a test
```

Bypass functions:

```bash
command grep -n -- "$pattern" "$file"
```

Call a builtin explicitly:

```bash
builtin printf '%s\n' "message"
```

Inspect external command cache:

```bash
hash
hash -r
```

## Minimal Example

```bash
for name in cd printf test grep; do
    type "$name"
done
```

For a runnable demonstration, see `examples/bash/builtins-command-lookup-demo.sh`.

## Real-World Example

Avoid recursive function calls when wrapping a command:

```bash
grep() {
    command grep --color=auto "$@"
}
```

Without `command`, the function would call itself again.

In scripts, prefer not to shadow common command names unless there is a very specific reason. A clearly named helper is usually easier to review:

```bash
grep_with_color() {
    command grep --color=auto "$@"
}
```

## Common Mistakes

- Assuming `echo` behaves the same across all shells and systems.
- Forgetting that `cd` and `read` must be builtins to affect the current shell.
- Writing functions that accidentally shadow external commands.
- Using `which` instead of `type` or `command -v` for shell-aware lookup.
- Assuming aliases work in scripts.
- Forgetting that command hashing can keep an old path until `hash -r`.
- Using `command` when a shell function intentionally should be used.

## Debugging Checklist

- Run `type -a name` to see all possible resolutions.
- Use `command -v name` in scripts to check dependencies.
- Use `hash -r` after changing `PATH` during an interactive session.
- Check whether a name is a function with `declare -f name`.
- Temporarily run a command as `command name` to bypass a wrapper function.
- Prefer `printf` over `echo` when output behavior matters.

## Related Topics

- [Execution Model](execution-model.md)
- [Interactive Shell Usage](interactive-shell-usage.md)
- [Functions](functions.md)
- [Robust Scripts](robust-scripts.md)
- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)

## References

- [GNU Bash Reference Manual: Command Search and Execution](https://www.gnu.org/software/bash/manual/html_node/Command-Search-and-Execution.html)
- [GNU Bash Reference Manual: Bash Builtin Commands](https://www.gnu.org/software/bash/manual/html_node/Bash-Builtins.html)
- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
- [GNU Bash Reference Manual: Shell Functions](https://www.gnu.org/software/bash/manual/html_node/Shell-Functions.html)
