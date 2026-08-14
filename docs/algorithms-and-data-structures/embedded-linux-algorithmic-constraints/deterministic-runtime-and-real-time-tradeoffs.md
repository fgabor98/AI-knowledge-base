---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Deterministic Runtime And Real-Time Tradeoffs

Deterministic algorithm design makes upper bounds on work, memory, blocking, and retry behavior visible. A low average runtime is not enough for an interrupt, control loop, watchdog-sensitive service, or other path with a deadline.

## Latency And Throughput

Latency is the time for one item or operation to complete. Throughput is the amount completed per unit time. A pipeline may improve throughput while increasing individual latency; batching may improve throughput while creating deadline spikes.

State which matters for the caller. A control loop may need a bounded maximum latency even if total throughput is low. A logging system may prefer throughput and tolerate bounded drops.

## Worst-Case Work

For a bounded input, calculate an upper bound from:

- loop iterations
- nested loops and branching
- comparisons or memory accesses per iteration
- queue or heap operations
- retries and timeout handling
- synchronization and preemption points

Big-O notation describes growth but does not give a time in microseconds. For a fixed maximum, count operations and measure on the target or a representative worst-case platform.

## Sources Of Non-Determinism

- unbounded loops or search
- dynamic allocation and reclaim
- cache misses and memory contention
- lock contention and priority inversion
- variable-length input or parsing
- retries waiting on external state
- frequency scaling, thermal throttling, and interrupts

Avoiding all variability may be impossible. The design goal is to bound or isolate the variability that can violate the contract.

## Programming Examples

### C: Bounded Scan With A Work Limit

This search checks at most `max_items` values and reports whether it found a match or stopped because the operational budget ended.

```c
#include <stddef.h>

enum bounded_scan_status {
    BOUNDED_SCAN_FOUND = 0,
    BOUNDED_SCAN_NOT_FOUND,
    BOUNDED_SCAN_LIMIT,
    BOUNDED_SCAN_ERR_NULL
};

enum bounded_scan_status find_positive_bounded(const int *values,
                                              size_t count,
                                              size_t max_items,
                                              size_t *out_index)
{
    size_t inspected = 0;

    if ((values == NULL && count > 0) || out_index == NULL)
        return BOUNDED_SCAN_ERR_NULL;

    while (inspected < count && inspected < max_items) {
        if (values[inspected] > 0) {
            *out_index = inspected;
            return BOUNDED_SCAN_FOUND;
        }
        inspected++;
    }

    if (inspected < count)
        return BOUNDED_SCAN_LIMIT;
    return BOUNDED_SCAN_NOT_FOUND;
}
```

The limit status distinguishes “all inputs inspected and no match” from “the budget expired before proof was complete.” If the caller needs cancellation, add a callback or flag check at the same bounded point.

### C: Fixed-Step Work Degradation

```c
enum processing_status {
    PROCESSING_OK = 0,
    PROCESSING_DEGRADED,
    PROCESSING_ERR_NULL
};

enum processing_status process_samples(const int *samples,
                                       size_t count,
                                       size_t max_samples,
                                       int *out_sum)
{
    int sum = 0;
    size_t limit;

    if ((samples == NULL && count > 0) || out_sum == NULL)
        return PROCESSING_ERR_NULL;
    limit = count < max_samples ? count : max_samples;
    for (size_t i = 0; i < limit; i++)
        sum += samples[i];
    *out_sum = sum;
    return limit == count ? PROCESSING_OK : PROCESSING_DEGRADED;
}
```

Degradation must be meaningful to the caller. If dropping samples changes safety or correctness, return an error instead of presenting a partial sum as complete.

### Python: Budgeted Search Reference

```python
def find_positive(values, max_items):
    for index, value in enumerate(values[:max_items]):
        if value > 0:
            return "found", index
    if max_items < len(values):
        return "limit", None
    return "not_found", None
```

## Deadline Policy

A deadline-aware algorithm should define:

- the clock and time unit
- when the deadline is checked
- whether a result completed after the deadline is usable
- whether partial output is valid
- how cancellation reaches blocking or worker operations
- what telemetry records deadline misses

Checking time on every tiny operation may be expensive; checking too rarely may violate the bound. Choose a maximum unchecked work interval.

## Priority And Blocking

An algorithm can have bounded local work and still miss deadlines because it blocks behind lower-priority work. Analyze lock ownership, queue waits, page faults, I/O, and scheduler behavior. In userspace, real-time policy and memory locking may be relevant; in kernel code, context and locking rules are subsystem-specific.

## Common Mistakes

- Reporting a limit-hit search as an ordinary not-found result.
- Treating average timing as a deadline guarantee.
- Hiding allocation, I/O, or retries inside a supposedly bounded helper.
- Adding logging on the critical path without measuring its cost.
- Choosing a lock without analyzing priority inversion.
- Making a partial result look complete.

## Embedded And Systems Angle

- prefer known upper bounds in interrupt, control, and watchdog-sensitive paths
- avoid hidden resizing, unbounded search, and unbounded retries
- make degradation policy explicit when deadlines cannot be guaranteed
- measure worst-case behavior on target hardware and build configuration
- expose limit hits, queue waits, and deadline misses

## Related Topics

- [Embedded Linux Algorithmic Constraints](index.md)
- [Big-O And Growth](../complexity-and-efficiency/big-o-and-growth.md)
- [Pruning And Search Heuristics](../searching-and-backtracking/pruning-and-search-heuristics.md)
- [Synchronization Costs And Result Merging](../parallel-and-dataflow-algorithms/synchronization-costs-and-result-merging.md)
