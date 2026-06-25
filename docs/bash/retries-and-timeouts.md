---
status: draft
reviewed: false
domain: bash
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Retries And Timeouts

## What Problem Does This Solve?

Retries and timeouts make automation scripts handle transient failures without hanging forever. They are useful for network calls, service startup checks, device enumeration, and CI steps that depend on external systems.

## Core Concepts

- Retry only operations that are safe to repeat.
- Add a maximum attempt count.
- Sleep between attempts.
- Preserve the final failure status when all attempts fail.
- Use `timeout` for commands that may hang.
- Treat timeouts differently from command failures when that matters.
- Log each retry attempt to stderr.
- Avoid hiding permanent failures behind infinite retry loops.

## Mental Model

Retry logic is a policy around a command. The policy should answer:

- Is it safe to run this command again?
- How many times should it run?
- How long should each attempt be allowed to run?
- What status should the caller receive if all attempts fail?

## Syntax / API / Mechanism

Simple retry function:

```bash
retry() {
    local attempts=$1
    local delay=$2
    shift 2

    local attempt status=0
    for ((attempt = 1; attempt <= attempts; attempt += 1)); do
        if "$@"; then
            return 0
        else
            status=$?
        fi

        printf 'attempt %d/%d failed with status %d\n' "$attempt" "$attempts" "$status" >&2

        if ((attempt < attempts)); then
            sleep "$delay"
        fi
    done

    return "$status"
}
```

Timeout:

```bash
timeout 10s command arg1 arg2
```

GNU `timeout` commonly returns `124` when a command times out.

## Minimal Example

```bash
retry 3 1 curl -fsS "$url"
```

For a runnable demonstration without network access, see `examples/bash/retry-demo.sh`.

## Real-World Example

Wait for a device node to appear:

```bash
wait_for_path() {
    local path=$1
    [[ -e "$path" ]]
}

if retry 20 0.5 wait_for_path /dev/ttyUSB0; then
    printf 'device appeared\n'
else
    printf 'device did not appear in time\n' >&2
    exit 1
fi
```

This is common during embedded bring-up where devices may appear asynchronously.

## Common Mistakes

- Retrying commands with side effects that are not safe to repeat.
- Infinite loops without a deadline.
- Losing the original failure status.
- Sleeping after the final attempt.
- Retrying syntax or configuration errors that will never succeed.
- Using retries to hide missing dependency checks.
- Assuming `timeout` exists on every minimal embedded root filesystem.

## Debugging Checklist

- Log attempt number and status.
- Test success on the first attempt.
- Test success after a later attempt.
- Test total failure.
- Test command timeout separately from command failure.
- Confirm whether the command is safe to repeat.
- Check whether target systems provide GNU `timeout`.

## Related Topics

- [Exit Codes](exit-codes.md)
- [Robust Scripts](robust-scripts.md)
- [Traps And Cleanup](traps-and-cleanup.md)
- [Safe Filesystem Operations](safe-filesystem-operations.md)

## References

- [GNU Coreutils manual: timeout invocation](https://www.gnu.org/software/coreutils/manual/html_node/timeout-invocation.html)
- [GNU Coreutils manual: sleep invocation](https://www.gnu.org/software/coreutils/manual/html_node/sleep-invocation.html)
- [GNU Bash Reference Manual: Shell Functions](https://www.gnu.org/software/bash/manual/html_node/Shell-Functions.html)
- [GNU Bash Reference Manual: Shell Arithmetic](https://www.gnu.org/software/bash/manual/html_node/Shell-Arithmetic.html)
- [GNU Bash Reference Manual: Exit Status](https://www.gnu.org/software/bash/manual/html_node/Exit-Status.html)
