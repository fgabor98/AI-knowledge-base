---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Subshells

## What Problem Does This Solve?

Subshells explain why some variable changes, directory changes, traps, and shell options disappear. They are central to pipelines, command substitution, grouping, and isolated script work.

## Core Concepts

- A subshell is a separate shell execution environment.
- Changes in a subshell do not affect the parent shell.
- `( commands )` runs commands in a subshell.
- `{ commands; }` groups commands in the current shell.
- Command substitution `$( ... )` runs in a subshell environment.
- Pipeline elements usually run in subshells in Bash.
- `BASHPID` is useful for observing subshell process identity.
- `$$` usually expands to the original shell's process ID, not each subshell's process ID.

## Mental Model

A subshell starts with a copy of shell state. It can read copied variables, functions, options, and the current directory. When it exits, its changes are discarded.

Use a subshell when you want isolation:

```bash
(
    cd "$repo"
    make
)
```

Use a brace group when you want the current shell to change:

```bash
{
    cd "$repo"
    make
}
```

## Syntax / API / Mechanism

Subshell grouping:

```bash
(cd "$dir" && command)
```

Current-shell grouping:

```bash
{ command1; command2; } >combined.log
```

Command substitution:

```bash
version=$(git describe --tags --always)
```

Pipeline caveat:

```bash
count=0
printf '%s\n' a b c | while IFS= read -r _line; do
    ((count += 1))
done
printf 'count=%d\n' "$count"
```

In Bash, the loop is usually in a subshell, so `count` remains `0` afterward.

## Minimal Example

```bash
pwd
(cd /tmp && pwd)
pwd
```

The parent shell returns to the original directory after the subshell exits.

For a runnable demonstration, see `examples/bash/subshell-demo.sh`.

## Real-World Example

Build several projects without manually restoring directories:

```bash
for project in "$workspace"/*; do
    [[ -d "$project" ]] || continue

    (
        cd "$project"
        make all
    )
done
```

Each `cd` is isolated to the subshell for that iteration.

## Common Mistakes

- Incrementing a variable inside a pipeline-fed `while` loop and expecting the value afterward.
- Using `( ... )` when `{ ...; }` was needed to affect the current shell.
- Forgetting that command substitution strips trailing newlines.
- Expecting traps or shell options set in a subshell to remain active in the parent.
- Debugging with `$$` instead of `BASHPID`.

## Debugging Checklist

- Print `BASHPID` inside and outside the block.
- Replace a pipeline-fed loop with input redirection and compare behavior.
- Check whether parentheses or command substitution introduce a subshell.
- Avoid relying on side effects from pipeline elements.
- Use temporary files or process substitution when a pipeline would lose state.

## Related Topics

- [Redirection And Pipes](redirection-and-pipes.md)
- [Read Lines Safely](read-lines-safely.md)
- [Execution Model](execution-model.md)
- [Robust Scripts](robust-scripts.md)

## References

- [GNU Bash Reference Manual: Command Execution Environment](https://www.gnu.org/software/bash/manual/html_node/Command-Execution-Environment.html)
- [GNU Bash Reference Manual: Command Substitution](https://www.gnu.org/software/bash/manual/html_node/Command-Substitution.html)
- [GNU Bash Reference Manual: Pipelines](https://www.gnu.org/software/bash/manual/html_node/Pipelines.html)
- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
