---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Logging

## What Problem Does This Solve?

Logging makes scripts observable without corrupting their data output. A script should be clear about what is machine-readable stdout, what is diagnostic stderr, and how verbosity is controlled.

## Core Concepts

- stdout should usually be data.
- stderr should usually be diagnostics.
- Use `printf`, not `echo`, for predictable output.
- Provide quiet and verbose modes when useful.
- Include timestamps only when they help the consumer.
- Keep log format stable if other tools parse it.
- Avoid logging secrets.
- Use a dedicated file descriptor when logs must go to a file.

## Mental Model

Treat output streams as interfaces:

- stdout is for the caller's pipeline.
- stderr is for humans and CI logs.
- log files are for later diagnosis.

Do not mix these roles accidentally.

## Syntax / API / Mechanism

Simple logging helpers:

```bash
log() {
    printf '%s\n' "$*" >&2
}

die() {
    log "error: $*"
    exit 1
}
```

Verbose logging:

```bash
verbose=0

debug() {
    ((verbose)) || return 0
    printf 'debug: %s\n' "$*" >&2
}
```

Log to a file descriptor:

```bash
exec {log_fd}>script.log
printf 'started\n' >&"$log_fd"
exec {log_fd}>&-
```

## Minimal Example

```bash
printf 'result-data\n'
printf 'diagnostic message\n' >&2
```

For a runnable demonstration, see `examples/bash/logging-demo.sh`.

## Real-World Example

Keep JSON on stdout and logs on stderr:

```bash
log() {
    printf '%s\n' "$*" >&2
}

log "querying device"
printf '{"state":"ready"}\n'
```

This lets another tool consume stdout without parsing log messages.

## Common Mistakes

- Logging to stdout in a script used by a pipeline.
- Using `echo -e` and getting different behavior across shells.
- Printing secrets in debug logs.
- Making verbose mode default in CI without log-size control.
- Capturing command output and accidentally capturing logs too.
- Changing log format that another tool relies on.

## Debugging Checklist

- Decide whether each message belongs on stdout or stderr.
- Pipe stdout into another command and verify logs do not appear there.
- Test quiet and verbose modes.
- Inspect logs for secrets.
- Use separate file descriptors for trace or log files when needed.
- Keep machine-readable output minimal and stable.

## Related Topics

- [Redirection And Pipes](redirection-and-pipes.md)
- [Advanced File Descriptors](advanced-file-descriptors.md)
- [Advanced Debugging And Tracing](advanced-debugging-and-tracing.md)
- [Robust Scripts](robust-scripts.md)

## References

- [GNU Bash Reference Manual: Redirections](https://www.gnu.org/software/bash/manual/html_node/Redirections.html)
- [GNU Bash Reference Manual: Bash Builtin Commands](https://www.gnu.org/software/bash/manual/html_node/Bash-Builtins.html)
- [POSIX.1-2024: Utility Conventions](https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/basedefs/V1_chap12.html)
