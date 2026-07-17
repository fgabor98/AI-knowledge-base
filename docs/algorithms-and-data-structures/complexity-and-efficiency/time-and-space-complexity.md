---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Time And Space Complexity

Time complexity describes how runtime grows. Space complexity describes how memory use grows. Many practical algorithm choices trade one for the other.

Systems code needs more detail than "time" and "space." You should know whether memory is on the stack, heap, static storage, DMA-visible memory, persistent storage, or inside a caller-owned buffer.

## Time Complexity

Time complexity usually counts the dominant operation:

- comparisons
- loop iterations
- memory accesses
- hash probes
- recursive calls
- queue operations

For a scan over `n` readings, time is O(n). For nested pair comparison, time is O(n^2). For binary search over sorted data, time is O(log n).

## Space Complexity

Space complexity counts additional memory used by the algorithm, not always the input itself.

Examples:

- scan with a few counters: O(1) extra space
- copy input into a temporary array: O(n) extra space
- recursive algorithm with depth `d`: O(d) stack space
- bitmap for IDs in range 0..255: O(1) with respect to input count, O(range) with respect to ID space

Be clear about which variable the memory depends on.

## Memory Categories

Stack:
: Automatic local variables and call frames. Fast, bounded by thread stack, risky for large buffers or recursion.

Heap:
: Dynamically allocated memory. Flexible, but can fail, fragment, block, or be forbidden in some contexts.

Static storage:
: Fixed lifetime storage. Predictable, but consumes memory for the entire program lifetime.

Caller-owned buffers:
: Memory provided by the caller. Often the best option for embedded APIs because ownership and failure policy are explicit.

DMA-visible buffers:
: Memory with hardware visibility and alignment/cache constraints.

Persistent storage:
: Flash, disk, or nonvolatile memory. Access cost and wear may dominate algorithm design.

## Peak Vs Steady-State Memory

Peak memory is the maximum memory used at one time. Steady-state memory is what remains after the operation completes.

A function that allocates a temporary 4 KiB buffer and frees it before returning has low steady-state memory but a 4 KiB peak. If the caller runs many such operations concurrently, peak memory matters.

## Time-Space Tradeoffs

Common tradeoffs:

- store a lookup table to avoid repeated scanning
- keep a running sum to avoid recalculating an average
- use a bitmap to speed membership checks
- use a cache to avoid recomputing expensive results
- sort once to support repeated binary searches

Each tradeoff adds state. Added state means added invariants and failure modes.

## Programming Examples

### C: Average By Scanning, O(n) Time And O(1) Space

```c
#include <stddef.h>
#include <stdint.h>

int average_scan(const int *values, size_t count, int *out_avg)
{
    int64_t sum = 0;

    if (out_avg == NULL)
        return -1;
    if (values == NULL || count == 0)
        return -1;

    for (size_t i = 0; i < count; i++)
        sum += values[i];

    *out_avg = (int)(sum / (int64_t)count);
    return 0;
}
```

This is simple and memory-efficient. Recomputing after every new reading costs O(n) each time.

### C: Running Average State, O(1) Query Time

This design stores extra state so average queries are O(1). Push is O(1), but the data structure has more invariants.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    AVG_HISTORY_CAPACITY = 16
};

struct avg_history {
    int values[AVG_HISTORY_CAPACITY];
    size_t count;
    size_t next;
    int64_t sum;
};

void avg_history_init(struct avg_history *history)
{
    if (history == NULL)
        return;
    history->count = 0;
    history->next = 0;
    history->sum = 0;
}

int avg_history_push(struct avg_history *history, int value)
{
    if (history == NULL)
        return -1;
    if (history->count > AVG_HISTORY_CAPACITY)
        return -1;
    if (history->next >= AVG_HISTORY_CAPACITY)
        return -1;

    if (history->count == AVG_HISTORY_CAPACITY) {
        history->sum -= history->values[history->next];
    } else {
        history->count++;
    }

    history->values[history->next] = value;
    history->sum += value;
    history->next = (history->next + 1) % AVG_HISTORY_CAPACITY;
    return 0;
}

int avg_history_average(const struct avg_history *history, int *out_avg)
{
    if (history == NULL || out_avg == NULL)
        return -1;
    if (history->count == 0)
        return -1;

    *out_avg = (int)(history->sum / (int64_t)history->count);
    return 0;
}
```

This improves repeated average queries but requires the invariant that `sum` equals the sum of the stored valid readings.

### C: Caller-Owned Temporary Buffer

Caller-owned buffers make memory cost explicit.

```c
#include <stddef.h>

int copy_positive_values(const int *input,
                         size_t input_count,
                         int *output,
                         size_t output_capacity,
                         size_t *out_count)
{
    size_t written = 0;

    if (out_count == NULL)
        return -1;
    if (input == NULL && input_count > 0)
        return -1;
    if (output == NULL && output_capacity > 0)
        return -1;

    for (size_t i = 0; i < input_count; i++) {
        if (input[i] <= 0)
            continue;
        if (written == output_capacity)
            return -1;
        output[written++] = input[i];
    }

    *out_count = written;
    return 0;
}
```

Runtime is O(n). Extra memory is provided by the caller, and capacity failure is explicit.

### Python: Reference Time-Space Tradeoff

```python
class RunningAverage:
    def __init__(self, capacity):
        self.capacity = capacity
        self.values = []
        self.total = 0

    def push(self, value):
        if len(self.values) == self.capacity:
            self.total -= self.values.pop(0)
        self.values.append(value)
        self.total += value

    def average(self):
        if not self.values:
            raise ValueError("empty history")
        return int(self.total / len(self.values))
```

This compact model is useful for expected behavior. The C implementation must still account for fixed storage and overflow.

## Accounting Template

For a function or data structure, document:

```text
Input size:
Maximum input size:
Time complexity:
Worst-case iterations:
Extra stack memory:
Heap allocations:
Static memory:
Caller-owned buffers:
Peak temporary memory:
Failure modes:
```

This template catches hidden memory and timing assumptions.

## Common Mistakes

- Saying O(1) memory while using a large stack buffer.
- Counting heap memory but ignoring call-stack recursion.
- Ignoring allocation failure paths.
- Trading memory for speed without adding invariants for the cached state.
- Using dynamic allocation where caller-owned or static bounded storage would be clearer.
- Forgetting that memory bandwidth can dominate runtime.

## Embedded And Systems Angle

- treat stack, heap, static storage, DMA buffers, and caches as different resources
- distinguish peak memory from steady-state memory
- include error paths and allocation failure in cost reasoning
- prefer caller-owned buffers when the caller should control memory policy

## Related Topics

- [Big-O And Growth](big-o-and-growth.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)
- [Data Modeling And Abstract Data Types](../algorithmic-foundations/data-modeling-and-abstract-data-types.md)
