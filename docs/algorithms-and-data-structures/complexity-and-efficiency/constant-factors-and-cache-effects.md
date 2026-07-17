---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Constant Factors And Cache Effects

Big-O hides constants. Real machines do not. For bounded systems workloads, memory layout, cache locality, branches, function-call overhead, allocation, and I/O can dominate the runtime even when two algorithms have the same Big-O class.

This page is not about premature micro-optimization. It is about knowing which practical costs Big-O does not show.

## Constant Factors

Two O(n) algorithms can have very different costs.

Examples of constant-factor costs:

- extra passes over memory
- expensive comparisons
- unpredictable branches
- function calls inside hot loops
- dynamic allocation
- cache misses
- pointer chasing
- system calls
- locks and atomics
- copying large elements instead of indexes or pointers

Constant factors matter most when input sizes are bounded and known.

## Cache Locality

Modern CPUs move memory in cache lines, not individual bytes. Sequential access to contiguous memory is usually much faster than chasing pointers through scattered allocations.

Good locality:

- arrays
- packed records
- sequential scans
- ring buffers
- bitmaps

Poor locality:

- linked lists with separately allocated nodes
- pointer-heavy graphs
- hash tables with collision chains
- structures much larger than the fields used by the hot loop

## Memory Access Patterns

Sequential access:
: Predictable, cache-friendly, often prefetchable.

Strided access:
: May waste cache lines if the stride skips most loaded data.

Random access:
: Often limited by cache misses and memory latency.

Pointer chasing:
: Random access plus dependency between loads, making it hard for the CPU to look ahead.

For many systems algorithms, the best optimization is a better layout.

## Branch Costs

Branches are cheap when predictable and expensive when unpredictable in hot loops.

Example:

- checking a rare error branch is usually cheap
- branching on random data in every iteration can be costly
- sorting or partitioning data can sometimes improve branch predictability

Do not contort code for branch behavior unless measurement shows it matters.

## Data Model Changes

Changing the representation can improve both Big-O and constants.

Examples:

- bitmap instead of array of booleans for compact visited state
- array of structs to struct of arrays when scanning one field dominates
- sorted array instead of tree for small read-mostly collections
- ring buffer instead of linked queue for producer-consumer handoff

These changes also add representation-specific invariants.

## Programming Examples

### C: Contiguous Array Scan

This is cache-friendly because values are contiguous.

```c
#include <stddef.h>
#include <stdint.h>

int64_t sum_array(const int *values, size_t count)
{
    int64_t sum = 0;

    if (values == NULL && count > 0)
        return 0;

    for (size_t i = 0; i < count; i++)
        sum += values[i];

    return sum;
}
```

Runtime is O(n). Constants are favorable because memory access is sequential.

### C: Linked List Scan

This is also O(n), but each node may live in a different cache line.

```c
#include <stdint.h>

struct int_node {
    int value;
    struct int_node *next;
};

int64_t sum_list(const struct int_node *node)
{
    int64_t sum = 0;

    while (node != 0) {
        sum += node->value;
        node = node->next;
    }

    return sum;
}
```

The list is not automatically wrong. It may be justified by insertion/removal or ownership requirements. But for pure traversal, a contiguous array is usually better.

### C: Compact Bitmap Membership

For dense bounded IDs, a bitmap can reduce both memory footprint and cache traffic.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    MAX_ID = 255,
    BITMAP_WORD_BITS = 32,
    BITMAP_WORDS = (MAX_ID + 1 + BITMAP_WORD_BITS - 1) / BITMAP_WORD_BITS
};

struct id_bitmap {
    uint32_t words[BITMAP_WORDS];
};

void id_bitmap_set(struct id_bitmap *bitmap, unsigned int id)
{
    if (bitmap == 0 || id > MAX_ID)
        return;

    bitmap->words[id / BITMAP_WORD_BITS] |=
        (uint32_t)1u << (id % BITMAP_WORD_BITS);
}

int id_bitmap_test(const struct id_bitmap *bitmap, unsigned int id)
{
    if (bitmap == 0 || id > MAX_ID)
        return 0;

    return (bitmap->words[id / BITMAP_WORD_BITS] &
            ((uint32_t)1u << (id % BITMAP_WORD_BITS))) != 0;
}
```

The cost is O(1), but the bigger win may be compactness and predictable memory access.

### Python: Rough Measurement Harness

Python timing cannot predict C target performance, but it can demonstrate measurement discipline.

```python
import time


def time_call(fn, *args, repeat=5):
    best = None
    for _ in range(repeat):
        start = time.perf_counter()
        result = fn(*args)
        elapsed = time.perf_counter() - start
        best = elapsed if best is None else min(best, elapsed)
    return best, result


def sum_values(values):
    total = 0
    for value in values:
        total += value
    return total


values = list(range(100_000))
elapsed, result = time_call(sum_values, values)
print(elapsed, result)
```

Use target-side measurements for final decisions. Python is useful for practicing controlled measurement and sanity-checking expected behavior.

## Measurement Checklist

Before trusting a measurement:

- build with representative compiler options
- use representative input sizes and distributions
- separate setup cost from measured hot path
- run enough iterations to reduce noise
- prevent dead-code elimination in microbenchmarks
- measure worst-case and common-case inputs separately
- check cache-warm and cache-cold behavior when relevant
- compare against a simple baseline
- record the target hardware and build configuration

For embedded Linux, tools may include timestamps, `perf`, tracing, GPIO toggles, hardware counters, or domain-specific instrumentation.

## Layout Checklist

When evaluating layout:

- Are hot fields contiguous?
- Does the loop touch only a small part of a large struct?
- Can indexes replace pointers?
- Is the data traversed in allocation order or logical order?
- Is alignment required by hardware?
- Is the buffer shared with DMA?
- Would a bitmap or fixed array be simpler than a pointer-heavy structure?

## Common Mistakes

- Assuming two O(n) implementations are equivalent.
- Optimizing constants before choosing the right algorithm.
- Measuring on a development machine and assuming target behavior.
- Benchmarking only tiny inputs when deployed inputs are larger.
- Ignoring cache effects from unrelated fields in a struct.
- Using linked structures for traversal-heavy workloads without a reason.
- Making code unreadable for an unmeasured micro-optimization.

## Embedded And Systems Angle

- prefer contiguous data when traversal dominates
- account for cache lines, DMA constraints, and alignment
- measure representative workloads before over-specializing code
- keep simple baselines so optimizations have something honest to beat

## Related Topics

- [Complexity And Efficiency](index.md)
- [Arrays Buffers And Records](../data-structures-for-algorithms/arrays-buffers-and-records.md)
- [Cache-Aware And DMA-Friendly Layouts](../embedded-linux-algorithmic-constraints/cache-aware-and-dma-friendly-layouts.md)
