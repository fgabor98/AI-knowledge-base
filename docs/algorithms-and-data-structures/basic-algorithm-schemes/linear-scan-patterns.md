---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Linear Scan Patterns

A linear scan processes elements from beginning to end. It is the simplest and most common algorithmic shape. Many scans are O(n), O(1) extra memory, easy to review, and fast enough when maximum input size is bounded.

The skill is not merely writing `for (i = 0; i < n; i++)`. The skill is knowing which state to maintain, when early exit is valid, and what must be true after each processed prefix.

## Scan Pattern Template

Most linear scans follow this shape:

```text
validate inputs
initialize state
for each element:
    validate or classify current element
    update state
write output
return status
```

The loop invariant usually describes what the state means for the already-processed prefix.

## Pattern: Counting

Counting tracks how many elements satisfy a predicate.

Invariant:

> After processing the first `i` elements, `count` equals the number of matching elements in that prefix.

Use counting when the caller needs a number, not the matching elements themselves.

## Pattern: Summation

Summation combines values.

Invariant:

> After processing the first `i` elements, `sum` equals the sum of that prefix.

In C, summation often needs overflow policy. A larger accumulator type may not be enough if input count and value range are large.

## Pattern: Minimum And Maximum

Minimum and maximum search track the best value seen so far.

Empty input policy is required. A common C shape is:

- reject empty input
- initialize best value from the first element
- scan from index zero or one

Initializing `min_value` to `0` is usually wrong unless `0` is a documented sentinel outside the data range.

## Pattern: Existence And Decision Checks

Existence checks can stop early:

> Does any reading exceed the threshold?

Universal decision checks can also stop early:

> Are all readings inside the valid range?

Early exit is correct when later elements cannot change the decision.

## Pattern: Selection

Selection chooses an element according to policy:

- first match
- last match
- smallest valid element
- largest valid element
- highest priority element
- closest element under a limit

Selection requires tie policy. If two elements are equally good, document which one wins.

## Pattern: Filtering

Filtering emits all matching elements. Unlike counting, filtering needs output capacity policy:

- fail when output is full
- truncate and report partial output
- run once to count, allocate, then run again to fill
- stream matches to a callback

For embedded C, caller-owned output buffers are often the clearest option.

## Pattern: Linear Search

Linear search finds a target by scanning until a match appears.

Best case: O(1). Worst case: O(n). Extra memory: O(1).

It is often the right choice when:

- data is small
- data is unsorted
- setup cost for indexing is not justified
- the search happens rarely

## Pattern: Ordered Linear Search

If data is sorted, a linear search can stop early once the current element exceeds the target.

This is still O(n) worst case, but it can reduce work for absent targets.

Use it for small sorted arrays where binary search complexity is unnecessary or where sequential access is cheaper than random access.

## Sentinels

A sentinel is an extra value placed so the loop can avoid checking a bound each iteration.

Sentinels can be useful in low-level code, but they are dangerous when:

- there is no spare slot
- the input is `const`
- the sentinel value can collide with real data
- mutation of input is not allowed
- concurrent readers can observe the sentinel

Use sentinels only when the data model explicitly reserves safe storage and mutation is acceptable.

## Programming Examples

### C: Count, Sum, Min, Max In One Pass

```c
#include <stddef.h>
#include <stdint.h>

enum scan_summary_status {
    SCAN_SUMMARY_OK = 0,
    SCAN_SUMMARY_ERR_NULL,
    SCAN_SUMMARY_ERR_EMPTY,
    SCAN_SUMMARY_ERR_RANGE
};

struct scan_summary {
    size_t count_above_threshold;
    int min_value;
    int max_value;
    int average;
};

enum scan_summary_status scan_readings(const int *readings,
                                       size_t count,
                                       int min_allowed,
                                       int max_allowed,
                                       int threshold,
                                       struct scan_summary *out)
{
    size_t count_above = 0;
    int min_value;
    int max_value;
    int64_t sum = 0;

    if (out == NULL)
        return SCAN_SUMMARY_ERR_NULL;
    if (readings == NULL && count > 0)
        return SCAN_SUMMARY_ERR_NULL;
    if (count == 0)
        return SCAN_SUMMARY_ERR_EMPTY;
    if (min_allowed > max_allowed)
        return SCAN_SUMMARY_ERR_RANGE;
    if (threshold < min_allowed || threshold > max_allowed)
        return SCAN_SUMMARY_ERR_RANGE;

    min_value = readings[0];
    max_value = readings[0];

    for (size_t i = 0; i < count; i++) {
        int value = readings[i];

        if (value < min_allowed || value > max_allowed)
            return SCAN_SUMMARY_ERR_RANGE;

        if (value > threshold)
            count_above++;
        if (value < min_value)
            min_value = value;
        if (value > max_value)
            max_value = value;

        sum += value;
    }

    out->count_above_threshold = count_above;
    out->min_value = min_value;
    out->max_value = max_value;
    out->average = (int)(sum / (int64_t)count);
    return SCAN_SUMMARY_OK;
}
```

This combines multiple scan outputs in one pass. That reduces memory traffic compared with separate passes, but it also makes the loop state richer. Keep the invariant clear.

### C: First Match With Explicit Not-Found

```c
#include <stddef.h>

enum find_status {
    FIND_OK = 0,
    FIND_NOT_FOUND,
    FIND_ERR_NULL
};

enum find_status find_first_at_or_above(const int *values,
                                        size_t count,
                                        int threshold,
                                        size_t *out_index)
{
    if (out_index == NULL)
        return FIND_ERR_NULL;
    if (values == NULL && count > 0)
        return FIND_ERR_NULL;

    for (size_t i = 0; i < count; i++) {
        if (values[i] >= threshold) {
            *out_index = i;
            return FIND_OK;
        }
    }

    return FIND_NOT_FOUND;
}
```

The output index is written only when a match exists.

### C: Ordered Linear Search

```c
#include <stddef.h>

enum ordered_find_status {
    ORDERED_FIND_OK = 0,
    ORDERED_FIND_NOT_FOUND,
    ORDERED_FIND_ERR_NULL
};

enum ordered_find_status ordered_find_int(const int *sorted_values,
                                          size_t count,
                                          int target,
                                          size_t *out_index)
{
    if (out_index == NULL)
        return ORDERED_FIND_ERR_NULL;
    if (sorted_values == NULL && count > 0)
        return ORDERED_FIND_ERR_NULL;

    for (size_t i = 0; i < count; i++) {
        if (sorted_values[i] == target) {
            *out_index = i;
            return ORDERED_FIND_OK;
        }
        if (sorted_values[i] > target)
            break;
    }

    return ORDERED_FIND_NOT_FOUND;
}
```

Precondition: `sorted_values` is sorted in ascending order. If that precondition is false, early exit can produce an incorrect not-found result.

### C: Filter With Required-Capacity Reporting

This variant reports how many output slots are required even when the provided buffer is too small.

```c
#include <stddef.h>

enum collect_status {
    COLLECT_OK = 0,
    COLLECT_ERR_NULL,
    COLLECT_ERR_CAPACITY
};

enum collect_status collect_nonzero(const int *input,
                                    size_t input_count,
                                    int *output,
                                    size_t output_capacity,
                                    size_t *out_written,
                                    size_t *out_required)
{
    size_t written = 0;
    size_t required = 0;
    enum collect_status status = COLLECT_OK;

    if (out_written == NULL || out_required == NULL)
        return COLLECT_ERR_NULL;
    if (input == NULL && input_count > 0)
        return COLLECT_ERR_NULL;
    if (output == NULL && output_capacity > 0)
        return COLLECT_ERR_NULL;

    for (size_t i = 0; i < input_count; i++) {
        if (input[i] == 0)
            continue;

        required++;
        if (written < output_capacity) {
            output[written++] = input[i];
        } else {
            status = COLLECT_ERR_CAPACITY;
        }
    }

    *out_written = written;
    *out_required = required;
    return status;
}
```

This API is useful when the caller may retry with a larger buffer.

### Python: Reference Scans

```python
def first_at_or_above(values, threshold):
    for index, value in enumerate(values):
        if value >= threshold:
            return index
    return None


def ordered_find(values, target):
    for index, value in enumerate(values):
        if value == target:
            return index
        if value > target:
            return None
    return None


def collect_nonzero(values):
    return [value for value in values if value != 0]
```

These are useful as test oracles for the C functions, especially for boundary cases.

## Scan Review Checklist

For every scan, check:

- What is the prefix invariant?
- Is empty input valid?
- Does the loop need to examine every element?
- Is early exit correct?
- Are outputs written only when valid?
- Is the predicate cheap or expensive?
- Can several passes be combined without making the code unclear?
- Does the output buffer have enough capacity?
- Is the input sorted, and does the code depend on that?

## Embedded And Systems Angle

- one-pass scans are often the right choice for bounded inputs
- sentinel techniques must not compromise memory safety
- keep comparison and predicate costs visible
- prefer explicit capacity reporting for filters that may be retried
- avoid hidden allocation in basic collection routines

## Related Topics

- [Basic Algorithm Schemes](index.md)
- [Loop Invariants And Termination](../control-flow-and-recursion/loop-invariants-and-termination.md)
- [Sorting Fundamentals](../sorting-and-ordering/sorting-fundamentals.md)
