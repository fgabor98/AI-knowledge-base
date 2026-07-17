---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Complexity And Efficiency

Complexity is how algorithm cost changes as input grows. Efficiency is how that cost behaves on real machines with real memory, caches, branches, allocation rules, I/O, and timing constraints.

For embedded and systems work, both views matter. Big-O helps compare growth. Concrete cost accounting tells you whether an algorithm fits a stack budget, frame deadline, interrupt policy, boot-time limit, or memory cap.

## Learning Goals

After this section, you should be able to:

- describe runtime and memory growth with Big-O notation
- distinguish best-case, average-case, and worst-case behavior
- identify the input size variable that drives cost
- compare simple algorithms by growth rate
- account for stack, heap, static, and temporary memory separately
- recognize when changing the data model reduces complexity
- explain why constant factors and cache effects can dominate bounded systems workloads
- measure hot paths without confusing measurement noise for algorithmic behavior

## Cost Questions

Before optimizing, answer:

- What is the input size?
- What is the maximum input size?
- Which operation dominates cost?
- Is worst-case behavior acceptable?
- How much temporary memory is needed?
- Does the algorithm allocate?
- Does it touch memory sequentially or randomly?
- Does it branch unpredictably?
- Does it call into the kernel, hardware, filesystem, network, or allocator?

The most important question is often not "what is the Big-O?" but "what is the worst thing this code can do on the target?"

## Runtime Cost

Runtime cost comes from work the algorithm performs:

- loop iterations
- comparisons
- arithmetic
- memory reads and writes
- branches
- function calls
- allocation
- copying
- I/O or hardware polling

Big-O usually counts the dominant operation and ignores constants. That is useful for growth-rate reasoning, but it is incomplete for systems code.

## Memory Cost

Memory cost includes:

- input storage
- output storage
- temporary buffers
- call stack
- heap allocations
- static storage
- alignment and padding
- cache footprint
- DMA-visible buffers

A function that uses O(1) extra memory may still be unacceptable if that constant is a 64 KiB stack buffer in a small thread.

## Common Growth Classes

| Class | Example | Meaning |
| --- | --- | --- |
| O(1) | read a struct field | constant work |
| O(log n) | binary search | work grows by repeated halving |
| O(n) | scan an array | work grows linearly |
| O(n log n) | comparison sorting | typical efficient sorting growth |
| O(n^2) | compare every pair | work grows quadratically |
| O(2^n) | exhaustive subset search | work doubles with each added choice |

The class describes growth, not absolute speed.

## Best, Average, And Worst Case

Best case:
: The cheapest valid input, such as finding a target at the first element.

Average case:
: Expected behavior under a defined input distribution.

Worst case:
: The most expensive valid input.

For latency-sensitive systems work, worst case usually matters most. Average-case behavior is useful only when the input distribution is known and missed deadlines are acceptable.

## Changing The Algorithm

Sometimes the direct algorithm is too expensive.

Example:

- Need to detect whether any reading appears twice.
- Direct approach: compare every pair, O(n^2).
- Better approach when value range is bounded: use a bitmap or frequency table, O(n) time and O(range) memory.

This is an algorithm change.

## Changing The Data Model

Sometimes the algorithm becomes simpler when the data is represented differently.

Example:

- Need to test whether sensor ID 37 is active.
- If active sensors are stored in an unsorted array, membership is O(n).
- If sensor IDs are dense and bounded, a bitmap makes membership O(1).
- If IDs are sparse and dynamic, a hash table might be appropriate.

This is a data-model change. It can reduce complexity, but it may increase memory use or implementation risk.

## Programming Examples

### C: Pairwise Duplicate Detection, O(n^2)

This direct implementation is simple and uses O(1) extra memory, but runtime grows quadratically.

```c
#include <stddef.h>

enum duplicate_status {
    DUPLICATE_OK = 0,
    DUPLICATE_ERR_NULL
};

enum duplicate_status has_duplicate_pairwise(const int *values,
                                             size_t count,
                                             int *out_has_duplicate)
{
    if (out_has_duplicate == NULL)
        return DUPLICATE_ERR_NULL;
    if (values == NULL && count > 0)
        return DUPLICATE_ERR_NULL;

    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (values[i] == values[j]) {
                *out_has_duplicate = 1;
                return DUPLICATE_OK;
            }
        }
    }

    *out_has_duplicate = 0;
    return DUPLICATE_OK;
}
```

This may be perfectly acceptable for a maximum of eight readings. It is a poor default for thousands of readings.

### C: Bounded Frequency Table, O(n)

When values are known to be in a small range, a table can reduce runtime to O(n) at the cost of O(range) memory.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    SENSOR_ID_MIN = 0,
    SENSOR_ID_MAX = 255,
    SENSOR_ID_RANGE = SENSOR_ID_MAX - SENSOR_ID_MIN + 1
};

enum duplicate_id_status {
    DUPLICATE_ID_OK = 0,
    DUPLICATE_ID_ERR_NULL,
    DUPLICATE_ID_ERR_RANGE
};

enum duplicate_id_status has_duplicate_sensor_id(const int *ids,
                                                 size_t count,
                                                 int *out_has_duplicate)
{
    uint8_t seen[SENSOR_ID_RANGE] = {0};

    if (out_has_duplicate == NULL)
        return DUPLICATE_ID_ERR_NULL;
    if (ids == NULL && count > 0)
        return DUPLICATE_ID_ERR_NULL;

    for (size_t i = 0; i < count; i++) {
        int id = ids[i];
        size_t offset;

        if (id < SENSOR_ID_MIN || id > SENSOR_ID_MAX)
            return DUPLICATE_ID_ERR_RANGE;

        offset = (size_t)(id - SENSOR_ID_MIN);
        if (seen[offset]) {
            *out_has_duplicate = 1;
            return DUPLICATE_ID_OK;
        }
        seen[offset] = 1;
    }

    *out_has_duplicate = 0;
    return DUPLICATE_ID_OK;
}
```

This is faster for larger `count`, but the fixed table is part of the memory contract. If the ID range becomes huge, the representation may no longer be appropriate.

### Python: Growth Comparison Model

Python can help show operation counts without tying the lesson to wall-clock timing.

```python
def pairwise_comparisons(n):
    return n * (n - 1) // 2


def table_operations(n):
    return n


for n in [4, 8, 16, 32, 64, 128]:
    print(n, pairwise_comparisons(n), table_operations(n))
```

This does not measure C performance. It makes growth visible.

## Efficiency Review Checklist

For each algorithm, record:

- input-size variable
- maximum input size
- best-case, average-case, and worst-case behavior
- time complexity
- extra space complexity
- allocations performed
- stack usage concerns
- memory access pattern
- likely constant-factor costs
- whether changing the data model would reduce cost

## Embedded And Systems Angle

- distinguish asymptotic cost from cache, branch, syscall, and allocation costs
- prefer bounded worst-case behavior for real-time paths
- account for stack, heap, DMA buffers, and persistent storage separately
- measure hot paths instead of assuming Big-O tells the whole story
- document capacity assumptions beside fixed-size tables and buffers

## Pages In This Section

- [Big-O And Growth](big-o-and-growth.md)
- [Time And Space Complexity](time-and-space-complexity.md)
- [Constant Factors And Cache Effects](constant-factors-and-cache-effects.md)

## Related Topics

- [Algorithmic Foundations](../algorithmic-foundations/index.md)
- [Sorting And Ordering](../sorting-and-ordering/index.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)
