---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Advanced Debugging And Tracing

## What Problem Does This Solve?

Advanced tracing makes complex Bash scripts debuggable without mixing trace noise into normal output. It helps identify expansion results, function call paths, failing commands, and line numbers.

## Core Concepts

- `set -x` prints commands after expansion.
- `PS4` controls trace prefixes.
- `BASH_XTRACEFD` redirects xtrace output to a separate descriptor.
- `BASH_SOURCE`, `LINENO`, and `FUNCNAME` help identify call paths.
- `caller` reports function call stack information.
- `trap DEBUG` can run before commands, but it is easy to overuse.
- Trace output may contain secrets.

## Mental Model

Tracing is instrumentation. Turn it on around the smallest useful region, send it to a controlled destination, and treat trace logs as sensitive.

## Syntax / API / Mechanism

Useful `PS4`:

```bash
PS4='+ ${BASH_SOURCE##*/}:${LINENO}:${FUNCNAME[0]:-main}: '
```

Trace to file:

```bash
exec {trace_fd}>trace.log
BASH_XTRACEFD=$trace_fd
set -x
```

Call stack helper:

```bash
show_stack() {
    local i=0
    while caller "$i"; do
        ((i += 1))
    done
}
```

## Minimal Example

```bash
PS4='+ ${LINENO}: '
set -x
value="two words"
printf '<%s>\n' "$value"
set +x
```

For a runnable demonstration, see `examples/bash/tracing-demo.sh`.

## Real-World Example

Enable tracing only when requested:

```bash
if [[ ${TRACE:-0} == 1 ]]; then
    exec {trace_fd}>trace.log
    BASH_XTRACEFD=$trace_fd
    PS4='+ ${BASH_SOURCE##*/}:${LINENO}:${FUNCNAME[0]:-main}: '
    set -x
fi
```

This avoids noisy default logs while preserving a powerful diagnostic mode.

## Common Mistakes

- Leaving `set -x` enabled for an entire script.
- Sending traces to stderr when stderr is consumed by another tool.
- Tracing secrets such as tokens, passwords, or private URLs.
- Using `trap DEBUG` without understanding how often it fires.
- Forgetting to close the trace descriptor.
- Assuming trace output shows original source instead of post-expansion commands.

## Debugging Checklist

- Reproduce with `TRACE=1`.
- Set a useful `PS4` before `set -x`.
- Redirect trace output with `BASH_XTRACEFD`.
- Inspect `BASH_SOURCE`, `FUNCNAME`, and `LINENO`.
- Disable tracing around secret material.
- Minimize the traced region.

## Related Topics

- [Errexit And ERR Traps](errexit-and-err-traps.md)
- [Advanced File Descriptors](advanced-file-descriptors.md)
- [Robust Scripts](robust-scripts.md)
- [Execution Model](execution-model.md)

## References

- [GNU Bash Reference Manual: The Set Builtin](https://www.gnu.org/software/bash/manual/html_node/The-Set-Builtin.html)
- [GNU Bash Reference Manual: Bash Variables](https://www.gnu.org/software/bash/manual/html_node/Bash-Variables.html)
- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
