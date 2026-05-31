---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Execution Model

## What Problem Does This Solve?

The Bash execution model explains what the shell does before a command actually runs. This is the foundation for understanding quoting, variables, redirection, pipelines, subshells, and exit status.

Without this model, Bash looks like a small programming language. In practice, it is a command interpreter that repeatedly transforms text into command invocations.

## Core Concepts

- Bash reads input from a script, `-c` argument, or interactive prompt.
- Bash parses input into commands and compound commands.
- Bash performs expansions before executing a simple command.
- Builtins run inside the current shell process.
- External commands run as separate processes.
- Functions run in the current shell unless called through a subshell or pipeline context.
- Redirections are prepared before the command runs.
- The exit status of each command drives control flow.
- Environment variables are inherited by child processes.
- Shell variables are not automatically exported.

## Mental Model

Think of Bash as a loop:

1. Read a command.
2. Parse it.
3. Expand parts of it.
4. Apply redirections.
5. Run a builtin, function, or external program.
6. Record an exit status.
7. Use that status to decide what happens next.

The command being run often receives only a final argument vector and inherited environment. It does not know what quoting, variables, or glob syntax appeared in the original script.

## Syntax / API / Mechanism

Command lookup order is roughly:

1. shell reserved words
2. aliases in interactive contexts where aliases are enabled
3. shell functions
4. shell builtins
5. executable files found through `PATH`

Useful commands for inspecting lookup:

```bash
type printf
type cd
type grep
command -v bash
```

Variable inheritance:

```bash
name=local-only
export exported_name=visible-to-children
```

`name` is a shell variable. `exported_name` is also placed in the environment of external commands.

## Minimal Example

```bash
#!/usr/bin/env bash
set -euo pipefail

message="hello from shell"
export exported_message="hello from environment"

bash -c 'printf "message=<%s>\n" "${message-}"'
bash -c 'printf "exported_message=<%s>\n" "$exported_message"'
```

The first child shell does not see `message` because it was not exported. The second child shell sees `exported_message`.

For a runnable version, see `examples/bash/execution-model-demo.sh`.

## Real-World Example

Use `command -v` to check dependencies before a deployment or build script starts doing work:

```bash
need_command() {
    command -v "$1" >/dev/null 2>&1 || {
        printf 'missing required command: %s\n' "$1" >&2
        exit 1
    }
}

need_command git
need_command make
need_command rsync
```

This uses shell command lookup directly instead of assuming a command exists somewhere in `PATH`.

## Common Mistakes

- Assuming shell variables are visible to external commands without `export`.
- Forgetting that `cd` must be a builtin because it changes the current shell.
- Expecting functions or variable changes inside a subshell to affect the parent shell.
- Confusing command lookup with filesystem paths.
- Treating aliases as reliable script features. Prefer functions in scripts.
- Assuming the text of a command after expansion still resembles the source line.

## Debugging Checklist

- Use `type name` to see what Bash will run.
- Use `command -v name` to check whether a command is available.
- Use `env | sort` to inspect exported environment values.
- Use `declare -p var` to inspect a shell variable.
- Use `set -x` temporarily to trace command execution.
- Print process IDs with `printf '%s\n' "$$ $BASHPID"` when debugging subshell behavior.

## Related Topics

- [Variables And Expansion](variables-and-expansion.md)
- [Quoting](quoting.md)
- [Exit Codes](exit-codes.md)
- [Subshells](subshells.md)

## References

- [GNU Bash Reference Manual: Shell Operation](https://www.gnu.org/software/bash/manual/html_node/Shell-Operation.html)
- [GNU Bash Reference Manual: Command Execution Environment](https://www.gnu.org/software/bash/manual/html_node/Command-Execution-Environment.html)
- [GNU Bash Reference Manual: Shell Parameters](https://www.gnu.org/software/bash/manual/html_node/Shell-Parameters.html)
- [POSIX.1-2024: Shell Command Language](https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html)
