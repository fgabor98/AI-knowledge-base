---
status: draft
reviewed: false
domain: bash
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Parallelism And Background Jobs

## What Problem Does This Solve?

Bash can run multiple commands concurrently, which is useful for independent build, test, download, or probing tasks. The hard part is bounding concurrency, collecting statuses, and cleaning up correctly.

## Core Concepts

- `command &` runs a command in the background.
- `$!` captures the background PID.
- `wait "$pid"` returns the child's exit status.
- `wait -n` waits for the next child to finish in newer Bash versions.
- Background jobs still need error handling.
- Bounded parallelism prevents resource exhaustion.
- Concurrent output can interleave unless redirected.

## Mental Model

Backgrounding starts work. `wait` is where the script learns whether the work succeeded. A parallel script is not correct until it collects every child status.

## Syntax / API / Mechanism

Run two jobs:

```bash
task_a &
pid_a=$!
task_b &
pid_b=$!

wait "$pid_a"
wait "$pid_b"
```

Collect failures:

```bash
status=0
for pid in "${pids[@]}"; do
    wait "$pid" || status=1
done
exit "$status"
```

## Minimal Example

```bash
pids=()
for item in a b c; do
    process "$item" &
    pids+=("$!")
done

for pid in "${pids[@]}"; do
    wait "$pid"
done
```

For a runnable bounded-concurrency example, see `examples/bash/bounded-parallelism-demo.sh`.

## Real-World Example

Run test shards in parallel, but report failure if any shard fails:

```bash
pids=()
status=0

for shard in 1 2 3 4; do
    run_shard "$shard" >"logs/shard-$shard.log" 2>&1 &
    pids+=("$!")
done

for pid in "${pids[@]}"; do
    if ! wait "$pid"; then
        status=1
    fi
done

exit "$status"
```

## Common Mistakes

- Starting jobs and only waiting for the last one.
- Ignoring `wait` failures.
- Letting parallel jobs write interleaved logs to the same stream.
- Starting unbounded jobs in a large loop.
- Forgetting to kill remaining jobs when one job fails early.
- Assuming `wait -n` is available on older Bash installations.

## Debugging Checklist

- Print PIDs and item names when jobs start.
- Redirect each job to a separate log while debugging.
- Test one failing child and confirm the script fails.
- Test interruption and confirm children are cleaned up.
- Keep concurrency low until correctness is clear.
- Check target Bash version before using `wait -n`.

## Related Topics

- [Signals, Process Groups, And Child Processes](signals-process-groups-and-child-processes.md)
- [Exit Codes](exit-codes.md)
- [Traps And Cleanup](traps-and-cleanup.md)
- [Advanced File Descriptors](advanced-file-descriptors.md)

## References

- [GNU Bash Reference Manual: Lists of Commands](https://www.gnu.org/software/bash/manual/html_node/Lists.html)
- [GNU Bash Reference Manual: Job Control Builtins](https://www.gnu.org/software/bash/manual/html_node/Job-Control-Builtins.html)
- [GNU Bash Reference Manual: Bash Builtin Commands](https://www.gnu.org/software/bash/manual/html_node/Bash-Builtins.html)
