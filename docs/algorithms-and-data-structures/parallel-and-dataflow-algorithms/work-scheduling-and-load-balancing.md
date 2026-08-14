---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: advanced
last_reviewed: null
---

# Work Scheduling And Load Balancing

Work scheduling assigns algorithmic tasks to workers while controlling imbalance, queueing, locality, and shutdown behavior. Parallel speedup is limited not only by the amount of work, but also by the longest dependency chain, scheduling overhead, synchronization, and the slowest worker.

## Work, Span, And Overhead

Use three quantities when reasoning about a parallel algorithm:

- **work `W`:** total useful operations across all workers
- **span `S`:** the length of the longest dependency chain
- **scheduler overhead:** partitioning, queueing, synchronization, and result merging

With `P` workers, an idealized lower bound is roughly `max(W / P, S)`. Real time is higher because tasks may be too small, workers may contend, and memory bandwidth may saturate. A scheduler that creates more tasks does not automatically improve utilization.

## Static Partitioning

Static scheduling assigns work before execution. A contiguous block per worker has low scheduling overhead and good locality. A cyclic assignment spreads regularly spaced expensive items across workers but can increase cache and DMA traffic.

Use static partitioning when:

- item cost is uniform or predictable
- the input is bounded
- reproducible ownership matters
- worker failure and cancellation are simple

### C: Deterministic Range Partitioning

```c
#include <stddef.h>

struct work_range {
    size_t begin;
    size_t end;
};

int make_range(size_t item_count,
               size_t worker_count,
               size_t worker_index,
               struct work_range *out_range)
{
    size_t base;
    size_t remainder;
    size_t begin;
    size_t length;

    if (out_range == NULL || worker_count == 0 ||
        worker_index >= worker_count)
        return -1;

    base = item_count / worker_count;
    remainder = item_count % worker_count;
    length = base + (worker_index < remainder ? 1 : 0);
    begin = worker_index * base +
            (worker_index < remainder ? worker_index : remainder);
    out_range->begin = begin;
    out_range->end = begin + length;
    return 0;
}
```

The first `remainder` workers receive one extra item. Every item belongs to exactly one range, ranges are adjacent, and the difference in item counts is at most one. The multiplication and addition are safe when `item_count` and `worker_count` are valid `size_t` values under the stated partition formula; validate externally supplied limits if the product can be close to the type maximum.

## Dynamic Chunking

Dynamic scheduling gives a worker another chunk after it completes its current chunk. It handles variable item cost better than static blocks, but adds an atomic or lock-protected counter and makes ownership less reproducible.

Chunk size is a tradeoff:

| Chunk choice | Advantage | Risk |
| --- | --- | --- |
| one item | fine balancing | high contention and scheduling overhead |
| fixed medium chunk | simple bounded behavior | can leave a straggler tail |
| decreasing chunks | good early utilization | more scheduling decisions |
| adaptive chunk | responds to measured cost | harder to reproduce and verify |

For a loop whose item cost is known, choose the largest chunk that keeps imbalance below the deadline budget. For unknown workloads, instrument queue depth and completion times rather than guessing from average throughput.

## Work Stealing

Work stealing gives each worker a local deque. A worker pushes and pops its own tasks at one end; an idle worker steals from the opposite end of another worker's deque. Local LIFO behavior tends to preserve cache locality and depth-first dependencies, while FIFO stealing obtains larger independent work units.

Stealing is useful for irregular recursive algorithms, but it requires careful ownership and shutdown rules:

- who owns a task while it is being popped?
- can a task be canceled after another worker steals it?
- when is the system quiescent?
- how are failed workers' tasks recovered?
- what is the maximum number of tasks and deque entries?

Unbounded stealing retries are not suitable for hard real-time paths. A bounded scheduler can stop after a fixed number of probes and return an incomplete status.

## Priority And Deadline Scheduling

Priority queues select urgent or valuable work, but they can starve low-priority tasks. Add aging, a maximum wait, or a reserved service share when starvation is unacceptable. A deadline queue should distinguish:

- work that is ready and before its deadline
- work that is ready but already late
- work that cannot finish before its deadline
- canceled or superseded work

Do not use a numeric priority as a substitute for a documented policy. Equal priorities need a stable sequence number if reproducibility matters.

## Stragglers And Locality

One worker holding a long task can determine total latency. Mitigations include smaller chunks, task splitting, speculative duplicate work, or a separate queue for known-heavy items. Speculation can increase total work and must not duplicate non-idempotent effects.

Data locality changes the best partition. Contiguous ranges help cache and DMA access, while cyclic work may balance a spatially regular but computationally irregular input. If workers share a memory bus, reducing cache misses may improve total throughput more than perfect arithmetic balance.

## Cancellation And Shutdown

Cancellation needs a protocol, not only a flag. Define when a worker checks it, whether in-flight work may finish, and whether partial results are valid. A clean shutdown generally has these phases:

1. stop accepting new work
2. mark queued work canceled or drain it according to policy
3. let workers leave their current bounded operation
4. merge completed results
5. release worker and queue resources

If a task owns a buffer or reference, cancellation must preserve its lifetime until the worker acknowledges completion.

## Python: Chunk Simulation

```python
def static_ranges(item_count, worker_count):
    if worker_count <= 0:
        raise ValueError("worker_count must be positive")
    base, remainder = divmod(item_count, worker_count)
    ranges = []
    begin = 0
    for worker in range(worker_count):
        length = base + (worker < remainder)
        ranges.append((begin, begin + length))
        begin += length
    return ranges


def simulated_completion(costs, worker_count, chunk_size):
    if worker_count <= 0 or chunk_size <= 0:
        raise ValueError("worker and chunk sizes must be positive")
    loads = [0] * worker_count
    chunks = [costs[i:i + chunk_size]
              for i in range(0, len(costs), chunk_size)]
    for chunk in chunks:
        worker = min(range(worker_count), key=lambda index: loads[index])
        loads[worker] += sum(chunk)
    return loads
```

The simulation is not a scheduler implementation. It is useful for comparing chunk sizes, estimating imbalance, and constructing regression cases where one heavy item dominates a static partition.

## Common Mistakes

- Balancing item counts when item costs are highly variable.
- Creating tasks smaller than the queue and synchronization overhead.
- Assuming a dynamic scheduler is deterministic because the input order is fixed.
- Stealing work without a task-lifetime or cancellation protocol.
- Duplicating side effects during speculative straggler mitigation.
- Letting high-priority work starve diagnostics or cleanup indefinitely.
- Reporting all worker exits as success when one task was canceled or failed.

## Embedded And Systems Angle

- bound task count, queue depth, worker stack, and result storage
- prefer deterministic static ranges for small uniform workloads
- select chunk sizes from cache, DMA, and deadline constraints
- make queue-full, worker-failure, cancellation, and late-work statuses explicit
- avoid unbounded steal retries and blocking in interrupt-adjacent paths
- preserve locality when memory traffic dominates arithmetic

## Review Checklist

- Is the work cost uniform enough for static partitioning?
- What are `W`, `S`, scheduler overhead, and the maximum pending tasks?
- Does each task have a clear owner and lifetime?
- Are priority, fairness, cancellation, and shutdown policies explicit?
- What happens to a straggler or failed worker?
- Are partial and late results distinguishable from successful completion?

## Related Topics

- [Parallel And Dataflow Algorithms](index.md)
- [Partitioning Reductions And Fan-In](partitioning-reductions-and-fan-in.md)
- [Priority And Partial Ordering](../sorting-and-ordering/priority-and-partial-ordering.md)
- [Deques](../data-structures-for-algorithms/deques.md)
- [Deterministic Runtime And Real-Time Tradeoffs](../embedded-linux-algorithmic-constraints/deterministic-runtime-and-real-time-tradeoffs.md)
