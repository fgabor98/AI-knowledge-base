---
status: draft
reviewed: false
domain: bash
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Interactive Shell Usage

## What Problem Does This Solve?

Bash is used both for scripts and as an interactive command environment. Interactive usage has its own tools: startup files, history, command-line editing, aliases, functions, prompts, completion, and job control. Understanding these keeps daily shell customization separate from script behavior.

## Core Concepts

- Login shells and non-login interactive shells read different startup files.
- Interactive configuration belongs in files such as `~/.bashrc`.
- Script behavior should not depend on aliases or interactive-only settings.
- Readline handles command-line editing and keybindings.
- History is a productivity tool, but may record sensitive commands.
- Aliases are convenient interactively; functions are usually better for reusable logic.
- `PS1` controls the interactive prompt.
- Job control lets a terminal user stop, resume, foreground, and background jobs.
- Programmable completion improves interactive commands, not non-interactive scripts.

## Mental Model

Keep two worlds separate:

- interactive shell customization for humans at a terminal
- non-interactive scripts that must run predictably in CI, cron, containers, and remote systems

Interactive Bash is allowed to be convenient. Script Bash should be explicit.

## Syntax / API / Mechanism

Common startup files:

| File | Common Role |
| --- | --- |
| `~/.bash_profile` | User login-shell setup. Often sources `~/.bashrc`. |
| `~/.bash_login` | Login-shell fallback if `~/.bash_profile` is absent. |
| `~/.profile` | POSIX-style login setup, often shared with other shells. |
| `~/.bashrc` | Interactive non-login Bash setup. |
| `~/.bash_logout` | Commands run when a login shell exits. |

Check shell mode:

```bash
case $- in
    *i*) printf 'interactive\n' ;;
    *) printf 'non-interactive\n' ;;
esac
```

Guard interactive-only configuration:

```bash
case $- in
    *i*) ;;
    *) return ;;
esac
```

History and editing:

```bash
history
bind -P
bind '"\C-x\C-r": redraw-current-line'
```

Job control:

```bash
jobs
fg %1
bg %1
```

## Minimal Example

A small `~/.bashrc` pattern:

```bash
case $- in
    *i*) ;;
    *) return ;;
esac

alias ll='ls -alF'

mkcd() {
    mkdir -p -- "$1" && cd -- "$1"
}
```

The guard prevents non-interactive shells from loading interactive-only customization.

## Real-World Example

Keep convenience aliases out of scripts:

```bash
alias gs='git status --short'

git_root() {
    git rev-parse --show-toplevel
}
```

Use `gs` at the terminal. Use `git status --short` directly in scripts. Use `git_root` interactively or move it into a script if other automation depends on it.

## Common Mistakes

- Putting interactive prompts, `stty`, or `bind` calls in files used by non-interactive shells.
- Assuming aliases expand in scripts.
- Hiding important script behavior inside a personal `~/.bashrc`.
- Storing secrets in shell history.
- Making `PS1` run slow commands on every prompt draw.
- Confusing job control with process management in scripts.
- Forgetting that Ctrl-C sends `SIGINT` to the foreground job, not just to Bash.

## Debugging Checklist

- Run `printf '%s\n' "$-"` to see whether the shell is interactive.
- Use `type name` to check whether a name is an alias, function, builtin, or executable.
- Temporarily start with `bash --noprofile --norc` to isolate startup-file behavior.
- Check `HISTFILE`, `HISTCONTROL`, and `HISTIGNORE` when debugging history.
- Run `bind -P` to inspect Readline bindings.
- Use `jobs -l` to inspect stopped or background jobs.
- Keep prompt customizations fast and failure-tolerant.

## Related Topics

- [When To Use Bash](when-to-use-bash.md)
- [Execution Model](execution-model.md)
- [Shell Builtins And Command Lookup](shell-builtins-and-command-lookup.md)
- [Programmable Completion](programmable-completion.md)
- [Signals, Process Groups, And Child Processes](signals-process-groups-and-child-processes.md)

## References

- [GNU Bash Reference Manual: Bash Startup Files](https://www.gnu.org/software/bash/manual/html_node/Bash-Startup-Files.html)
- [GNU Bash Reference Manual: Interactive Shells](https://www.gnu.org/software/bash/manual/html_node/Interactive-Shells.html)
- [GNU Bash Reference Manual: Bash History Facilities](https://www.gnu.org/software/bash/manual/html_node/Bash-History-Facilities.html)
- [GNU Bash Reference Manual: Job Control Basics](https://www.gnu.org/software/bash/manual/html_node/Job-Control-Basics.html)
- [GNU Bash Reference Manual: Controlling the Prompt](https://www.gnu.org/software/bash/manual/html_node/Controlling-the-Prompt.html)
- [GNU Readline User Manual](https://tiswww.case.edu/php/chet/readline/rltop.html)
