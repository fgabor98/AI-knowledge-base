---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Synchronization Costs And Result Merging

Parallelism changes an algorithm's cost model. Work may decrease per worker while synchronization, queue traffic, cache invalidation, barriers, context switches, and result merging add overhead. A parallel algorithm is useful only when the saved work exceeds these costs under the actual workload and hardware.

## Work And Span

Work is the total amount of computation performed across all workers. Span, or critical-path length, is the shortest possible elapsed time with unlimited workers.

A practical lower-bound model is:

```text
elapsed time >= max(work / usable_workers, span) + synchronization_cost
```

The number of CPU cores is not the number of usable workers. Interrupt load, cache contention, thermal limits, and other processes reduce capacity.

## Synchronization Costs

Synchronization may include:

- mutex or spinlock acquisition
- atomic read-modify-write operations
- memory barriers and cache-coherence traffic
- condition-variable wakeups
- barriers that wait for the slowest worker
- queue push/pop and ownership transfer
- retries after contention

Model these costs as part of the algorithm. A shared counter updated for every item can be slower and less deterministic than one local counter per worker followed by a final reduction.

## Local State Before Shared State

Prefer:

1. immutable input shared by readers
2. worker-local state and buffers
3. one synchronization point for merging partial results

This reduces contention and makes ownership easier to audit. Shared mutable state may still be necessary for a queue, cancellation flag, or bounded resource, but keep the shared surface small.

## Deterministic Merging

A merge is deterministic when the same input produces the same output ordering and values regardless of worker completion order. Techniques include:

- assign each worker a stable partition index
- store one result per partition
- merge partitions in index order
- use a fixed reduction tree
- attach sequence numbers to streamed results

Deterministic merging can require more buffering than opportunistic merging. That is a deliberate tradeoff, not a free property.

## Programming Examples

### C: Local Counts And One Merge

This example avoids a shared increment for every input item. Each worker writes one slot, and the caller merges those slots in a fixed order.

```c
#include <stddef.h>
#include <stdint.h>

enum count_status {
    COUNT_OK = 0,
    COUNT_ERR_NULL,
    COUNT_ERR_WORKERS
};

struct count_result {
    size_t matches;
};

typedef int (*count_predicate)(int value, void *context);

enum count_status count_partition(const int *values,
                                  size_t count,
                                  size_t worker_count,
                                  count_predicate predicate,
                                  void *context,
                                  struct count_result *partials)
{
    if ((values == NULL && count > 0) ||
        predicate == NULL || partials == NULL)
        return COUNT_ERR_NULL;
    if (worker_count == 0)
        return COUNT_ERR_WORKERS;

    for (size_t worker = 0; worker < worker_count; worker++) {
        size_t start = worker * count / worker_count;
        size_t end = (worker + 1) * count / worker_count;

        partials[worker].matches = 0;
        for (size_t i = start; i < end; i++) {
            if (predicate(values[i], context))
                partials[worker].matches++;
        }
    }
    return COUNT_OK;
}

enum count_status merge_counts(const struct count_result *partials,
                               size_t worker_count,
                               size_t *out_matches)
{
    size_t total = 0;

    if (partials == NULL || out_matches == NULL)
        return COUNT_ERR_NULL;
    if (worker_count == 0)
        return COUNT_ERR_WORKERS;

    for (size_t worker = 0; worker < worker_count; worker++) {
        if (SIZE_MAX - total < partials[worker].matches)
            return COUNT_ERR_WORKERS;
        total += partials[worker].matches;
    }
    *out_matches = total;
    return COUNT_OK;
}
```

The example is sequential so the partition and merge contract is visible. In a concurrent implementation, each worker can write only its own `partials[worker]` slot, and the merge waits until all required slots are complete.

### C: Shared Counter Tradeoff

```c
/* Conceptual only: every matching item contends on the same location. */
void count_with_shared_counter(const int *values,
                               size_t count,
                               size_t *shared_matches)
{
    for (size_t i = 0; i < count; i++) {
        if (values[i] > 0)
            (*shared_matches)++; /* requires synchronization in parallel code */
    }
}
```

The code is not thread-safe as written. That visibility is the point: parallelizing it requires an atomic operation or lock on every match, and that cost belongs in the design comparison.

### Python: Deterministic Merge

```python
def merge_partition_counts(partials):
    total = 0
    for partition_index, count in enumerate(partials):
        if count < 0:
            raise ValueError(f"invalid result in partition {partition_index}")
        total += count
    return total
```

Merging a list in partition order gives deterministic behavior even if workers completed in a different order.

## Barriers And Pipeline Stages

A barrier is simple but can waste time when one worker finishes early. A pipeline may avoid global barriers by allowing independent stages to continue, but then queue capacity and shutdown become more complex.

Use a barrier when:

- the next phase needs every partial result
- phase boundaries simplify correctness
- the slowest-worker wait is acceptable

Use streaming or staged fan-in when:

- results can be consumed incrementally
- buffering is bounded
- partial failure and cancellation have clear semantics

## Parallel Sorting

Parallel sorting commonly sorts partitions independently and merges sorted runs. The merge stage can dominate memory and synchronization costs. If stability matters, preserve partition order and stable local ordering; if equal keys cross partitions, define which partition wins.

For small arrays, a single-threaded sort often wins because thread setup and synchronization exceed the saved comparisons.

## Common Mistakes

- Measuring only worker computation and ignoring queue, barrier, and merge costs.
- Updating shared counters in a hot loop without a contention budget.
- Assuming completion order is a valid output order.
- Letting one slow or failed worker leave the merger waiting forever.
- Allocating temporary result storage without a maximum.
- Parallelizing a workload too small to amortize worker startup.

## Embedded And Systems Angle

- model synchronization as part of algorithm cost
- prefer local state and one bounded merge over frequent shared mutation
- define deterministic merge order when output stability matters
- include cancellation, timeout, and worker-failure states
- measure contention and cache effects on the target hardware

## Related Topics

- [Parallel And Dataflow Algorithms](index.md)
- [Partitioning Reductions And Fan-In](partitioning-reductions-and-fan-in.md)
- [Deterministic Runtime And Real-Time Tradeoffs](../embedded-linux-algorithmic-constraints/deterministic-runtime-and-real-time-tradeoffs.md)
- [Constant Factors And Cache Effects](../complexity-and-efficiency/constant-factors-and-cache-effects.md)
