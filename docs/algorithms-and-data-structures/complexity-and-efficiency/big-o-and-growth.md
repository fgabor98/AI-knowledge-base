---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Big-O And Growth

Big-O notation describes how algorithm cost grows as input size grows. It ignores constant factors and lower-order terms so you can compare the shape of algorithms.

Big-O is not a stopwatch. It does not tell you which implementation is faster for a small fixed input. It tells you what happens as the input grows.

## What Big-O Means

If an algorithm is O(n), its dominant work grows linearly with input size. Doubling input roughly doubles that dominant work.

If an algorithm is O(n^2), doubling input roughly quadruples that dominant work.

If an algorithm is O(log n), input can grow substantially while the number of steps grows slowly.

The variable `n` must be defined. It might mean:

- number of readings
- number of bytes
- number of vertices
- number of edges
- number of queued items
- number of possible states

For graph algorithms, there may be more than one input-size variable, such as `V` for vertices and `E` for edges.

## Dropping Constants

Big-O ignores constant multipliers:

- `3n` is O(n)
- `100n` is O(n)
- `n / 2` is O(n)

It also ignores lower-order terms:

- `n^2 + n` is O(n^2)
- `n log n + n` is O(n log n)

This is useful because growth dominates eventually. It can be misleading for small bounded workloads where constants dominate.

## Common Growth Patterns

O(1):
: Direct access, fixed number of checks, pushing into a non-full fixed slot.

O(log n):
: Repeated halving, such as binary search over sorted data.

O(n):
: One scan over all elements.

O(n log n):
: Many comparison sorts and balanced divide-and-conquer algorithms.

O(n^2):
: Nested loops over pairs.

O(2^n):
: Exhaustive choice search where each additional input doubles the search space.

## Best, Average, Worst

Big-O should usually be qualified:

- Best-case O(1), worst-case O(n)
- Average-case O(1), worst-case O(n)
- Worst-case O(log n)

Example: linear search has best-case O(1) if the target is first, but worst-case O(n) if the target is absent or last.

Worst-case behavior is the default concern for hard bounds, deadline-sensitive code, and hostile input.

## Bounded Inputs

Big-O does not replace capacity analysis.

If `n <= 8`, an O(n^2) algorithm has at most 28 pair comparisons. That might be clearer and safer than a hash table.

If `n <= 4096`, the same O(n^2) algorithm can perform over eight million pair comparisons. That may be unacceptable.

Always pair growth class with maximum size.

## Programming Examples

### C: Linear Search, O(n)

```c
#include <stddef.h>

int find_int_linear(const int *values,
                    size_t count,
                    int target,
                    size_t *out_index)
{
    if (out_index == NULL)
        return -1;
    if (values == NULL && count > 0)
        return -1;

    for (size_t i = 0; i < count; i++) {
        if (values[i] == target) {
            *out_index = i;
            return 0;
        }
    }

    return 1;
}
```

Best case: O(1), target at index zero. Worst case: O(n), target absent or at the last index.

### C: Binary Search, O(log n)

Binary search changes the problem model. It requires sorted input.

```c
#include <stddef.h>

int find_int_binary(const int *values,
                    size_t count,
                    int target,
                    size_t *out_index)
{
    size_t lo = 0;
    size_t hi = count;

    if (out_index == NULL)
        return -1;
    if (values == NULL && count > 0)
        return -1;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if (values[mid] == target) {
            *out_index = mid;
            return 0;
        }
        if (values[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }

    return 1;
}
```

Worst case: O(log n), but only if the array is sorted according to the same ordering used by the search.

### C: Pair Comparison, O(n^2)

```c
#include <stddef.h>

size_t count_equal_pairs(const int *values, size_t count)
{
    size_t pairs = 0;

    if (values == NULL && count > 0)
        return 0;

    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (values[i] == values[j])
                pairs++;
        }
    }

    return pairs;
}
```

The number of comparisons is `n * (n - 1) / 2`, which is O(n^2).

### Python: Operation Count Table

```python
import math


def growth_table(sizes):
    for n in sizes:
        print(
            f"{n:5d}",
            f"O(log n)~{math.ceil(math.log2(max(n, 1))):5d}",
            f"O(n)={n:7d}",
            f"O(n^2)={n * n:10d}",
        )


growth_table([1, 2, 4, 8, 16, 32, 64, 128, 1024])
```

This is useful for intuition. It is not a replacement for target measurements.

## Comparing Algorithms

When comparing two algorithms:

- state the input model
- state preconditions, such as sortedness
- compare worst-case growth
- compare extra memory
- include setup cost
- include update cost if the data changes over time

Binary search is not simply "better than linear search." If data arrives unsorted and is searched once, sorting first may cost more than scanning once.

## Common Mistakes

- Saying "this is O(n)" without defining `n`.
- Comparing algorithms with different preconditions.
- Ignoring setup cost, such as sorting or building a table.
- Using average-case behavior where worst-case behavior matters.
- Rejecting a simple O(n^2) algorithm even though `n` is tiny and fixed.
- Choosing a complex O(log n) structure when O(n) scanning is clearer and fast enough.

## Embedded And Systems Angle

- combine growth-rate reasoning with fixed maximum sizes
- use worst-case behavior for latency-sensitive and real-time paths
- do not let Big-O hide memory traffic or I/O costs
- record algorithmic cost beside capacity constants

## Related Topics

- [Complexity And Efficiency](index.md)
- [Time And Space Complexity](time-and-space-complexity.md)
- [Constant Factors And Cache Effects](constant-factors-and-cache-effects.md)
