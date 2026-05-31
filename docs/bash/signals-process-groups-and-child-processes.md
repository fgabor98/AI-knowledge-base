---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Signals, Process Groups, And Child Processes

## What Problem Does This Solve?

Scripts that start child processes must decide what happens when the script is interrupted, terminated, or times out. Signal and child-process handling prevents orphaned work, half-cleaned state, and misleading success statuses.

## Core Concepts

- `trap` can handle signals such as `INT` and `TERM`.
- `kill` sends signals to processes or process groups.
- `wait` collects child status.
- `$!` is the PID of the most recent background command.
- `jobs -p` lists background jobs known to the current shell.
- Process groups matter when commands start more commands.
- `SIGKILL` cannot be trapped.
- Signal-derived statuses are commonly represented as `128 + signal`.

## Mental Model

When a script launches background work, it becomes responsible for that work. A robust script records child PIDs, forwards termination when needed, waits for cleanup, and exits with a meaningful status.

## Syntax / API / Mechanism

Start and wait:

```bash
long_task &
pid=$!
wait "$pid"
```

Terminate children on exit:

```bash
cleanup() {
    kill "${children[@]}" 2>/dev/null || true
}
trap cleanup EXIT
```

Forward termination:

```bash
on_term() {
    kill "${children[@]}" 2>/dev/null || true
    wait || true
    exit 143
}
trap on_term INT TERM
```

## Minimal Example

```bash
sleep 10 &
pid=$!
printf 'started child pid=%s\n' "$pid"
wait "$pid"
```

For a runnable demonstration, see `examples/bash/process-management-demo.sh`.

## Real-World Example

Run a service-like helper while doing a foreground task:

```bash
children=()

start_child() {
    "$@" &
    children+=("$!")
}

cleanup() {
    kill "${children[@]}" 2>/dev/null || true
    wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

start_child helper --serve
run_tests
```

The cleanup function prevents the helper from continuing after the script exits.

## Common Mistakes

- Starting background jobs and never waiting for them.
- Losing child PIDs by running background commands in subshells.
- Assuming `kill "$pid"` terminates a whole process tree.
- Writing traps that recursively trigger themselves.
- Ignoring child exit statuses.
- Assuming `jobs` works the same in non-interactive scripts as in interactive shells.

## Debugging Checklist

- Print child PIDs when starting background work.
- Use `ps -o pid,ppid,pgid,stat,cmd` while testing.
- Force Ctrl-C and verify children exit.
- Test normal success, child failure, and interrupted execution.
- Check whether the child starts grandchildren.
- Use `wait "$pid"` and inspect the status.

## Related Topics

- [Traps And Cleanup](traps-and-cleanup.md)
- [Parallelism And Background Jobs](parallelism-and-background-jobs.md)
- [Exit Codes](exit-codes.md)
- [Subshells](subshells.md)

## References

- [GNU Bash Reference Manual: Job Control](https://www.gnu.org/software/bash/manual/html_node/Job-Control.html)
- [GNU Bash Reference Manual: Job Control Builtins](https://www.gnu.org/software/bash/manual/html_node/Job-Control-Builtins.html)
- [GNU Bash Reference Manual: Signals](https://www.gnu.org/software/bash/manual/html_node/Signals.html)
- [GNU Bash Reference Manual: Bourne Shell Builtins](https://www.gnu.org/software/bash/manual/html_node/Bourne-Shell-Builtins.html)
