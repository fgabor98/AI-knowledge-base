---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Programmable Completion

## What Problem Does This Solve?

Programmable completion improves interactive CLI usability by completing commands, options, subcommands, filenames, hosts, or domain-specific values. It is useful when you maintain Bash-based tooling or developer scripts.

## Core Concepts

- Completion runs in an interactive shell.
- `complete` registers a completion function.
- `compgen` generates candidate completions.
- `COMPREPLY` is the array returned to Bash.
- `COMP_WORDS` and `COMP_CWORD` describe the current command line.
- Completion scripts should be fast and side-effect-light.
- Completion code is Bash-specific.

## Mental Model

Completion is not command execution. It is a query from the shell asking, "Given this partial command line, what words are valid here?"

## Syntax / API / Mechanism

Basic completion:

```bash
_mytool_complete() {
    local cur=${COMP_WORDS[COMP_CWORD]}
    COMPREPLY=($(compgen -W "start stop status" -- "$cur"))
}
complete -F _mytool_complete mytool
```

Subcommand-sensitive completion:

```bash
case "${COMP_WORDS[1]-}" in
    start|stop) COMPREPLY=() ;;
    *) COMPREPLY=($(compgen -W "start stop status" -- "$cur")) ;;
esac
```

## Minimal Example

```bash
source examples/bash/completion-demo.sh
complete -p mytool
```

The example registers completion for a hypothetical command named `mytool`.

## Real-World Example

Complete subcommands and options:

```bash
_deploy_complete() {
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}

    case "$prev" in
        --target)
            COMPREPLY=($(compgen -W "dev staging prod" -- "$cur"))
            return
            ;;
    esac

    COMPREPLY=($(compgen -W "plan apply destroy --target --verbose" -- "$cur"))
}
complete -F _deploy_complete deploy
```

## Common Mistakes

- Running slow commands on every completion attempt.
- Producing output on stdout instead of filling `COMPREPLY`.
- Forgetting that completion scripts are sourced into the user's shell.
- Using unquoted `COMP_WORDS` values in command execution.
- Assuming completion behavior matters in non-interactive scripts.
- Not guarding optional external dependencies.

## Debugging Checklist

- Source the completion script manually.
- Run `complete -p command` to inspect registration.
- Print debug logs to a temporary file, not stdout.
- Test empty input, partial options, and subcommands.
- Keep completion functions side-effect-free.
- Use `set -x` carefully because completion fires frequently.

## Related Topics

- [Functions](functions.md)
- [Associative Arrays](associative-arrays.md)
- [Advanced Debugging And Tracing](advanced-debugging-and-tracing.md)
- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)

## References

- [GNU Bash Reference Manual: Programmable Completion](https://www.gnu.org/software/bash/manual/html_node/Programmable-Completion.html)
- [GNU Bash Reference Manual: Programmable Completion Builtins](https://www.gnu.org/software/bash/manual/html_node/Programmable-Completion-Builtins.html)
- [GNU Bash Reference Manual: Bash Variables](https://www.gnu.org/software/bash/manual/html_node/Bash-Variables.html)
