---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Loop Invariants And Termination

A loop is correct when it does the intended work, stops at the right time, and preserves the facts the rest of the algorithm depends on. Loop invariants and termination arguments are the tools for reasoning about that.

In C, most algorithmic bugs in loops are ordinary but expensive: off-by-one indexes, missing progress, unchecked bounds, stale counters, and partial output after failure.

## Loop Anatomy

A well-structured loop has:

- initial state
- condition
- progress
- body work
- invariant
- termination behavior

Example:

```c
for (size_t i = 0; i < count; i++) {
    /* process readings[i] */
}
```

Initial state:
: `i` starts at `0`.

Condition:
: Continue while `i < count`.

Progress:
: `i` increases by one after each iteration.

Termination:
: The loop stops when `i == count`.

## Loop Invariants

A loop invariant is a statement that remains true before the first iteration and after each iteration.

For a counting loop:

> After processing the first `i` elements, `matches` equals the number of processed elements that satisfy the predicate.

This tells you:

- what `matches` means
- which portion of the array has been processed
- why the final value is correct when `i == count`

## Progress Measures

A progress measure must move toward termination.

Common progress measures:

- index increases toward `count`
- remaining byte count decreases toward zero
- pointer moves toward an end pointer
- queue count decreases toward zero
- retry budget decreases toward zero

If you cannot point to the progress measure, the loop may be unbounded or too hard to review.

## Off-By-One Boundaries

Prefer half-open ranges for indexes:

```text
[start, end)
```

This means `start` is included and `end` is excluded. In C arrays, a scan over `count` elements naturally uses:

```c
for (size_t i = 0; i < count; i++)
```

Avoid writing the same loop as `i <= count - 1` unless there is a strong reason. It is easier to get wrong when `count == 0`.

## Example: Counting Valid Readings

Problem:

> Count how many readings are inside a valid range.

Preconditions:

- `readings != NULL` when `count > 0`
- `out_valid != NULL`
- `min_value <= max_value`

Postcondition on success:

- `*out_valid` is between `0` and `count`
- `*out_valid` equals the number of in-range readings

Loop invariant:

> After processing the first `i` elements, `valid` is the number of in-range readings among `readings[0]` through `readings[i - 1]`.

Termination:

> `i` starts at `0`, increases by one each iteration, and stops when `i == count`.

## Programming Examples

### C: Counting With A Clear Invariant

```c
#include <stddef.h>

enum valid_count_status {
    VALID_COUNT_OK = 0,
    VALID_COUNT_ERR_NULL,
    VALID_COUNT_ERR_RANGE
};

enum valid_count_status count_valid_readings(const int *readings,
                                             size_t count,
                                             int min_value,
                                             int max_value,
                                             size_t *out_valid)
{
    size_t valid = 0;

    if (out_valid == NULL)
        return VALID_COUNT_ERR_NULL;
    if (readings == NULL && count > 0)
        return VALID_COUNT_ERR_NULL;
    if (min_value > max_value)
        return VALID_COUNT_ERR_RANGE;

    for (size_t i = 0; i < count; i++) {
        if (readings[i] >= min_value && readings[i] <= max_value)
            valid++;
    }

    *out_valid = valid;
    return VALID_COUNT_OK;
}
```

Why this loop terminates:

- `i` starts at `0`
- the loop condition requires `i < count`
- `i` increments once per iteration
- `count` does not change inside the loop

Why the result is correct:

- before the loop, no elements have been processed and `valid == 0`
- each iteration updates `valid` exactly when the current element is in range
- after the loop, all `count` elements have been processed

### C: Bounded Retry Loop

Loops that interact with hardware, files, or services should usually have an explicit budget.

```c
enum poll_result {
    POLL_READY = 0,
    POLL_TIMEOUT,
    POLL_ERROR
};

typedef int (*ready_fn)(void *ctx);

enum poll_result poll_until_ready(ready_fn is_ready,
                                  void *ctx,
                                  unsigned int max_attempts)
{
    if (is_ready == 0 || max_attempts == 0)
        return POLL_ERROR;

    for (unsigned int attempt = 0; attempt < max_attempts; attempt++) {
        int ready = is_ready(ctx);

        if (ready < 0)
            return POLL_ERROR;
        if (ready > 0)
            return POLL_READY;
    }

    return POLL_TIMEOUT;
}
```

The progress measure is `attempt`. The loop cannot run more than `max_attempts` times.

### Python: Invariant-Oriented Reference

```python
def count_valid_readings(readings, minimum, maximum):
    if readings is None:
        raise ValueError("readings must not be None")
    if minimum > maximum:
        raise ValueError("minimum must be <= maximum")

    valid = 0
    for i, value in enumerate(readings):
        if minimum <= value <= maximum:
            valid += 1
        assert valid == sum(
            1 for earlier in readings[: i + 1] if minimum <= earlier <= maximum
        )
    return valid
```

The assertion is not efficient, but it makes the invariant explicit for teaching and small reference tests.

## Termination Checklist

For each loop, ask:

- What is the progress measure?
- Does every path through the loop update or consume progress?
- Can the loop condition become false?
- Is the maximum iteration count known?
- What happens at zero iterations?
- What happens at exactly one iteration?
- What happens at the maximum expected count?
- Can any variable used in the condition overflow?
- Can an error path leave partial output or inconsistent state?

## Common Loop Bugs

- Using `i <= count` instead of `i < count`.
- Decrementing an unsigned index below zero.
- Changing `count` inside a loop that assumes it is fixed.
- Forgetting to advance a pointer on an error-retry path.
- Using a sentinel without reserving safe storage for it.
- Returning partial output without documenting it.
- Letting a retry loop run forever when hardware never becomes ready.

## Embedded And Systems Angle

- avoid unbounded loops in latency-sensitive paths
- make timeout, retry, and maximum-iteration policies explicit
- keep loop state inspectable during debugging
- prefer fixed maximum work per call when watchdogs or scheduling latency matter

## Related Topics

- [Control Flow And Recursion](index.md)
- [Binary Search](../basic-algorithm-schemes/binary-search.md)
- [Deterministic Runtime And Real-Time Tradeoffs](../embedded-linux-algorithmic-constraints/deterministic-runtime-and-real-time-tradeoffs.md)
