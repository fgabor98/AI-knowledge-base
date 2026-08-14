---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Partitioning Reductions And Fan-In

Partitioning divides input work into independent portions. A reduction computes one result from partial results, and fan-in combines outputs from multiple workers or stages. These patterns are useful only when the partition boundaries and merge operation preserve the required meaning.

## Partitioning

For an input sequence of `n` items and `p` workers, a deterministic contiguous partition can assign worker `k`:

```text
start = floor(k * n / p)
end   = floor((k + 1) * n / p)
```

The ranges are disjoint and cover `[0, n)`. Contiguous ranges improve locality and make result order straightforward. Cyclic assignment can balance uneven item costs but complicates cache behavior and merge order.

Every partitioning scheme should define behavior for:

- zero input
- more workers than items
- a worker failure
- cancellation between partitions
- an input whose item costs are uneven

## Reduction Requirements

A reduction can merge partial results in arbitrary order only when the operation is associative:

```text
(a op b) op c == a op (b op c)
```

For deterministic parallel results, commutativity may also be required:

```text
a op b == b op a
```

Integer addition is mathematically associative, but fixed-width integer addition is not associative when overflow wraps or when overflow is treated as failure. Floating-point addition is also not exactly associative because rounding depends on grouping.

## Identity Values

A reduction needs an identity for empty partitions:

- sum: `0`
- product: `1`
- logical OR: false
- logical AND: true
- minimum: a documented maximum sentinel or an explicit empty result
- maximum: a documented minimum sentinel or an explicit empty result

Do not use a sentinel that can be a legitimate result without carrying an empty flag.

## Fan-In Policies

Fan-in can:

- concatenate partition outputs in partition order
- merge sorted partition outputs
- reduce partial summaries
- select the best result according to a comparator
- preserve original sequence numbers through a reorder stage

The merge policy determines both correctness and storage. A reduction may need one value per worker; concatenation may need the whole output or a streaming consumer.

## Programming Examples

### C: Deterministic Partitioned Sum

This example computes local sums for fixed contiguous partitions and combines them in worker order. It uses `int64_t` and checks accumulation overflow.

```c
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

enum reduction_status {
    REDUCTION_OK = 0,
    REDUCTION_ERR_NULL,
    REDUCTION_ERR_WORKERS,
    REDUCTION_ERR_OVERFLOW
};

static int add_i64_checked(int64_t left, int64_t right, int64_t *out)
{
    if (right > 0 && left > INT64_MAX - right)
        return 0;
    if (right < 0 && left < INT64_MIN - right)
        return 0;
    *out = left + right;
    return 1;
}

enum reduction_status partitioned_sum(const int *values,
                                      size_t count,
                                      size_t worker_count,
                                      int64_t *out_sum)
{
    int64_t total = 0;

    if ((values == NULL && count > 0) || out_sum == NULL)
        return REDUCTION_ERR_NULL;
    if (worker_count == 0)
        return REDUCTION_ERR_WORKERS;

    for (size_t worker = 0; worker < worker_count; worker++) {
        size_t start = worker * count / worker_count;
        size_t end = (worker + 1) * count / worker_count;
        int64_t partial = 0;

        for (size_t i = start; i < end; i++) {
            if (!add_i64_checked(partial, values[i], &partial))
                return REDUCTION_ERR_OVERFLOW;
        }
        if (!add_i64_checked(total, partial, &total))
            return REDUCTION_ERR_OVERFLOW;
    }

    *out_sum = total;
    return REDUCTION_OK;
}
```

The range formulas use integer division and do not require `count` to be divisible by `worker_count`. The multiplication can overflow if arbitrary `size_t` values are accepted; a production API should validate maximum count and worker count or use a division-based partition helper.

### C: Partial Minimum With An Empty Flag

```c
struct partial_minimum {
    int value;
    int has_value;
};

static struct partial_minimum minimum_merge(struct partial_minimum left,
                                            struct partial_minimum right)
{
    if (!left.has_value)
        return right;
    if (!right.has_value)
        return left;
    return left.value < right.value ? left : right;
}
```

The explicit flag handles empty partitions without reserving an invalid integer sentinel.

### Python: Partition And Reduce

```python
def partitions(values, worker_count):
    if worker_count <= 0:
        raise ValueError("worker_count must be positive")
    count = len(values)
    for worker in range(worker_count):
        start = worker * count // worker_count
        end = (worker + 1) * count // worker_count
        yield values[start:end]


def partitioned_sum(values, worker_count):
    return sum(sum(part) for part in partitions(values, worker_count))
```

The Python model makes the partition coverage easy to test. Its arbitrary-precision integers hide the overflow behavior that the C implementation must define explicitly.

## Uneven Work

Equal item counts do not guarantee equal work. If item cost varies substantially, use measured cost estimates, dynamic work stealing, or smaller chunks. Those approaches improve balance but introduce scheduling overhead, synchronization, and less predictable output order.

For embedded systems, deterministic contiguous chunks may be preferable even when average utilization is lower. The acceptable choice depends on deadline, power, and reproducibility requirements.

## Floating-Point Reductions

Parallel floating-point sums can differ from sequential sums because grouping changes rounding. Options include:

- document a tolerance rather than exact equality
- use a deterministic fixed merge tree
- use compensated summation per partition
- use fixed-point or integer units when the domain permits

Do not call two bitwise-different floating results a data race automatically. First determine whether the algorithm contract requires reproducibility or only numerical bounds.

## Common Mistakes

- Overlapping or skipping partition boundaries.
- Assuming addition remains associative after fixed-width overflow.
- Using an invalid min/max sentinel for empty partitions.
- Merging results as workers finish when output order is required.
- Balancing by item count when item cost is highly uneven.
- Allocating one unbounded partial-result buffer per worker.

## Embedded And Systems Angle

- use associative operations when merge order can vary
- keep partition boundaries deterministic when reproducibility matters
- account for partial-result storage and worker failure
- choose fixed-point or checked integer reductions when exact results matter
- expose whether a result is complete, partially reduced, or cancelled

## Related Topics

- [Parallel And Dataflow Algorithms](index.md)
- [Priority And Partial Ordering](../sorting-and-ordering/priority-and-partial-ordering.md)
- [Synchronization Costs And Result Merging](synchronization-costs-and-result-merging.md)
- [Time And Space Complexity](../complexity-and-efficiency/time-and-space-complexity.md)
