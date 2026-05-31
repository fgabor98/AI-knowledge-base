---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Advanced File Descriptors

## What Problem Does This Solve?

Advanced file descriptor handling lets scripts separate logs from data, redirect whole command groups, keep files open across multiple commands, and send `set -x` traces somewhere other than normal stderr.

## Core Concepts

- File descriptors are small integers that refer to open files, pipes, terminals, or sockets.
- `exec` without a command changes the current shell's file descriptors.
- Bash can allocate descriptors with `exec {fd}>file`.
- Descriptor duplication copies the current destination of a descriptor.
- Descriptor lifetime matters; close descriptors explicitly when finished.
- `BASH_XTRACEFD` can redirect `set -x` trace output to a separate descriptor.

## Mental Model

Redirection on a command is temporary. Redirection with `exec` changes the current shell until you change it back or close the descriptor.

Use named descriptor variables when you need to avoid hard-coding descriptor numbers:

```bash
exec {log_fd}>script.log
printf 'message\n' >&"$log_fd"
exec {log_fd}>&-
```

## Syntax / API / Mechanism

Open and close a descriptor:

```bash
exec {fd}>out.log
printf 'hello\n' >&"$fd"
exec {fd}>&-
```

Redirect a whole block:

```bash
{
    printf 'stdout\n'
    printf 'stderr\n' >&2
} >out.log 2>err.log
```

Trace to a file:

```bash
exec {trace_fd}>trace.log
BASH_XTRACEFD=$trace_fd
set -x
```

## Minimal Example

```bash
exec {log_fd}>run.log
printf 'starting\n' >&"$log_fd"
exec {log_fd}>&-
```

For a runnable demonstration, see `examples/bash/advanced-fd-demo.sh`.

## Real-World Example

Keep machine-readable data on stdout while sending logs to a file:

```bash
exec {log_fd}>build.log

log() {
    printf '%s\n' "$*" >&"$log_fd"
}

log "starting build"
printf '{"status":"ok"}\n'
exec {log_fd}>&-
```

This matters when another program consumes stdout as data.

## Common Mistakes

- Forgetting to close descriptors.
- Sending logs to stdout when stdout is part of a data pipeline.
- Misunderstanding `2>&1` ordering.
- Reusing a descriptor number that another part of the script also uses.
- Leaving `BASH_XTRACEFD` pointing at a closed descriptor.
- Assuming descriptor allocation syntax is POSIX shell. It is Bash-specific.

## Debugging Checklist

- Print diagnostic output to stderr or a dedicated descriptor.
- Use `lsof -p "$$"` during debugging when available.
- Reduce descriptor changes to a small script.
- Close descriptors explicitly with `exec {fd}>&-`.
- Check whether stdout is data, logs, or both.
- Use `BASH_XTRACEFD` to separate traces from normal stderr.

## Related Topics

- [Redirection And Pipes](redirection-and-pipes.md)
- [Advanced Debugging And Tracing](advanced-debugging-and-tracing.md)
- [Robust Scripts](robust-scripts.md)
- [Bash Vs POSIX Sh](bash-vs-posix-sh.md)

## References

- [GNU Bash Reference Manual: Redirections](https://www.gnu.org/software/bash/manual/html_node/Redirections.html)
- [GNU Bash Reference Manual: Bash Variables](https://www.gnu.org/software/bash/manual/html_node/Bash-Variables.html)
- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
