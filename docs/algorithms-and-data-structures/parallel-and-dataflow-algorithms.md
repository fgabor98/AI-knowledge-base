---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Parallel And Dataflow Algorithms

Roadmap for algorithms that split work across stages or workers.

## Coverage

- data channels
- pipeline stages
- input partitioning
- fan-out and fan-in
- reductions
- associative operations
- parallel minimum and maximum search
- parallel sorting
- work distribution
- synchronization costs
- ordering constraints
- result merging

## Embedded And Systems Angle

- account for synchronization cost before parallelizing
- use associative reductions when partial results can be merged safely
- bound queues between stages to avoid memory growth
- design cancellation, timeout, and shutdown paths for pipelines

## Related Topics

- [Complexity And Efficiency](complexity-and-efficiency.md)
- [Data Structures For Algorithms](data-structures-for-algorithms.md)
- [Embedded Linux Algorithmic Constraints](embedded-linux-algorithmic-constraints.md)
