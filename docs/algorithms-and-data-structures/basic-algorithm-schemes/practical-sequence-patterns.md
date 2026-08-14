---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Practical Sequence Patterns

Many sequence problems can be solved by maintaining a small amount of state while a pair of indexes moves through the data. These patterns are useful between a simple linear scan and a full data structure. They are especially valuable when input is already ordered, arrives as a stream, or has a known maximum length.

The central design question is what each index and each piece of retained state means. Write that meaning before optimizing the loop.

## Two-Pointer Scans

Two pointers are useful when movement in one direction never needs to be undone. Common forms include:

- left and right ends of a sorted array
- a read index and a write index for in-place filtering
- two indexes advancing through two sorted sequences
- a slow index for the next accepted item and a fast index for inspection

If the sequence is sorted, a pair-sum search can move the left pointer right when the sum is too small and the right pointer left when it is too large. Each pointer moves at most `n` times, so the scan is O(n) after sorting.

### C: Compact Unique Values In Place

```c
#include <stddef.h>

enum compact_status {
    COMPACT_OK = 0,
    COMPACT_ERR_NULL
};

enum compact_status compact_sorted_unique(int *values,
                                          size_t count,
                                          size_t *out_count)
{
    size_t write = 0;

    if (out_count == NULL)
        return COMPACT_ERR_NULL;
    if (values == NULL && count > 0)
        return COMPACT_ERR_NULL;
    if (count == 0) {
        *out_count = 0;
        return COMPACT_OK;
    }

    for (size_t read = 1; read < count; read++) {
        if (values[read] == values[write])
            continue;
        values[++write] = values[read];
    }

    *out_count = write + 1;
    return COMPACT_OK;
}
```

The invariant is that `values[0..write]` contains the unique values from the processed prefix. The input must be sorted; without that precondition, equal values separated by other values are not removed.

## Sliding Windows

A sliding window represents a contiguous range `[left, right)`. Expand `right` as new data arrives and move `left` when the window violates a constraint. The pattern is effective when removing the leftmost item restores validity or when the constraint is monotonic under extension.

Typical uses include:

- longest substring or packet span with a bounded number of distinct values
- smallest interval whose sum reaches a threshold for non-negative values
- fixed-size moving average or maximum
- rate limiting over a time window

The non-negative-value condition matters for sum thresholds. If values can be negative, moving `left` may remove a value and make the sum larger or smaller unpredictably; a different algorithm is needed.

### C: Longest Window With A Sum Limit

```c
#include <stddef.h>
#include <stdint.h>

enum window_status {
    WINDOW_OK = 0,
    WINDOW_ERR_NULL,
    WINDOW_ERR_NEGATIVE,
    WINDOW_ERR_OVERFLOW
};

enum window_status longest_nonnegative_window(const uint32_t *values,
                                             size_t count,
                                             uint64_t limit,
                                             size_t *out_start,
                                             size_t *out_length)
{
    size_t left = 0;
    size_t best_start = 0;
    size_t best_length = 0;
    uint64_t sum = 0;

    if (out_start == NULL || out_length == NULL)
        return WINDOW_ERR_NULL;
    if (values == NULL && count > 0)
        return WINDOW_ERR_NULL;

    for (size_t right = 0; right < count; right++) {
        if (UINT64_MAX - sum < values[right])
            return WINDOW_ERR_OVERFLOW;
        sum += values[right];
        while (left <= right && sum > limit)
            sum -= values[left++];

        if (right + 1 - left > best_length) {
            best_start = left;
            best_length = right + 1 - left;
        }
    }

    *out_start = best_start;
    *out_length = best_length;
    return WINDOW_OK;
}
```

The window is always valid after the inner loop. Each index advances only, giving O(n) time and O(1) extra memory. Empty input returns a zero-length window at index zero.

## Prefix Sums

A prefix sum converts repeated range-sum queries into constant-time subtraction:

```text
prefix[0] = 0
prefix[i + 1] = prefix[i] + values[i]
sum(values[left:right]) = prefix[right] - prefix[left]
```

The extra element at `prefix[0]` removes special cases for ranges beginning at zero. Prefix values need a wider or checked type; a fast query is not useful if construction silently wraps.

### C: Checked Prefix Sums

```c
#include <limits.h>
#include <stddef.h>

enum prefix_status {
    PREFIX_OK = 0,
    PREFIX_ERR_NULL,
    PREFIX_ERR_OVERFLOW,
    PREFIX_ERR_RANGE
};

enum prefix_status build_prefix_sums(const int *values,
                                     size_t count,
                                     long long *prefix)
{
    if (prefix == NULL)
        return PREFIX_ERR_NULL;
    if (values == NULL && count > 0)
        return PREFIX_ERR_NULL;

    prefix[0] = 0;
    for (size_t i = 0; i < count; i++) {
        if ((values[i] > 0 && prefix[i] > LLONG_MAX - values[i]) ||
            (values[i] < 0 && prefix[i] < LLONG_MIN - values[i]))
            return PREFIX_ERR_OVERFLOW;
        prefix[i + 1] = prefix[i] + values[i];
    }
    return PREFIX_OK;
}

enum prefix_status prefix_range_sum(const long long *prefix,
                                    size_t count,
                                    size_t left,
                                    size_t right,
                                    long long *out_sum)
{
    if (prefix == NULL || out_sum == NULL)
        return PREFIX_ERR_NULL;
    if (left > right || right > count)
        return PREFIX_ERR_RANGE;
    *out_sum = prefix[right] - prefix[left];
    return PREFIX_OK;
}
```

Use a difference array when many range updates are followed by one final materialization. Add `delta[left] += amount` and `delta[right] -= amount`, then take a prefix sum. This changes each range update to O(1), but queries before materialization do not see the final values.

## Fast And Slow Pointers

Fast and slow pointers are useful for linked structures and periodic sequences. Advance the fast pointer twice as quickly; if the two pointers meet, a cycle exists. To find the cycle entry, reset one pointer to the head and advance both one step at a time.

For arrays, the same idea can detect repeated values when a function maps each index to another index, but the mapping and bounds must be part of the contract. Do not use pointer arithmetic beyond the allocated object merely because the fast pointer is “expected” to stop.

## Monotonic Stacks

A monotonic stack retains candidates in increasing or decreasing order. When a new value makes the top candidate impossible, pop it and resolve its next-greater, previous-smaller, or histogram boundary relationship.

Each element is pushed once and popped once, so the total work is O(n), even though one iteration may pop many elements. The stack stores indexes rather than only values when the distance or original position is part of the answer.

## Monotonic Queues

A monotonic deque supports the maximum or minimum of a moving fixed-size window. Before appending a new index, remove indexes from the back whose values cannot become the answer while the new value remains in the window. Before reading the front, remove indexes outside the window.

### Python: Moving Maximum

```python
from collections import deque


def moving_maximum(values, width):
    if width <= 0:
        raise ValueError("width must be positive")
    pending = deque()
    result = []
    for index, value in enumerate(values):
        while pending and pending[0] <= index - width:
            pending.popleft()
        while pending and values[pending[-1]] <= value:
            pending.pop()
        pending.append(index)
        if index + 1 >= width:
            result.append(values[pending[0]])
    return result
```

The deque indexes are increasing from front to back, and their values are decreasing. The front is therefore the maximum for the current window. The result is empty when the input is shorter than the requested width.

## Online Versus Offline Processing

An online algorithm consumes data without requiring the entire sequence first. Sliding windows, ring-buffer scans, and monotonic queues can operate online if the window bound is known. Prefix sums and sorting are usually offline because they require preparation before all queries or output can be answered.

For streaming systems, define whether a partial final window is emitted, discarded, or reported as incomplete. A batch algorithm must not be presented as online merely because it reads from an iterator.

## Common Mistakes

- Moving a window pointer without proving that the discarded prefix cannot help later.
- Applying the positive-sum window rule to sequences containing negative values.
- Allocating a prefix table without checking the count and arithmetic range.
- Storing values instead of indexes when expiration matters.
- Forgetting that a monotonic queue's front may have left the window.
- Calling amortized O(1) stack or deque work a per-iteration worst-case guarantee.
- Returning a partial stream window as a complete result.

## Embedded And Systems Angle

- size windows, deques, and prefix buffers from explicit input limits
- use checked fixed-width arithmetic for sums and differences
- keep indexes relative to the buffer when data can wrap or move
- bound the number of monotonic-stack pops per call when a hard deadline exists
- distinguish a completed window from a partial window at end-of-stream
- prefer one-pass online forms when rereading external or DMA-backed data is costly

## Review Checklist

- What does every pointer or index mean after each iteration?
- Is pointer movement monotonic, and why is discarded data irrelevant?
- Which value-domain assumptions make the window valid?
- Are prefix and difference arithmetic operations checked?
- Is auxiliary storage bounded and does it retain indexes when needed?
- Are empty, short, full, and partial-window cases specified?

## Related Topics

- [Basic Algorithm Schemes](index.md)
- [Linear Scan Patterns](linear-scan-patterns.md)
- [Binary Search](binary-search.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
- [Deques](../data-structures-for-algorithms/deques.md)
- [Priority And Partial Ordering](../sorting-and-ordering/priority-and-partial-ordering.md)
