---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Parallel And Dataflow Algorithms

Overview of algorithms that split work across stages or workers.

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

## Pages In This Section

- [Pipeline And Dataflow Algorithms](pipeline-and-dataflow-algorithms.md)
- [Partitioning Reductions And Fan-In](partitioning-reductions-and-fan-in.md)
- [Synchronization Costs And Result Merging](synchronization-costs-and-result-merging.md)
- [Work Scheduling And Load Balancing](work-scheduling-and-load-balancing.md)
- [Atomic And Lock-Free Algorithm Patterns](atomic-and-lock-free-algorithm-patterns.md)

## Programming Examples

- C: use bounded queues, explicit ownership, and visible synchronization policy.
- Python: use simulations and reference models where helpful.

## Embedded And Systems Angle

- account for synchronization cost before parallelizing
- use associative reductions when partial results can be merged safely
- bound queues between stages to avoid memory growth
- design cancellation, timeout, and shutdown paths for pipelines

## Related Topics

- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
- [Data Structures For Algorithms](../data-structures-for-algorithms/index.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)
