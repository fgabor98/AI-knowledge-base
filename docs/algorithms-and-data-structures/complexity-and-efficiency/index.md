---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Complexity And Efficiency

Roadmap for reasoning about algorithm cost.

## Coverage

- runtime cost
- memory cost
- Big-O notation
- constant factors
- best-case, average-case, and worst-case behavior
- input-size growth
- time complexity
- space complexity
- reducing execution time
- reducing memory use
- reducing complexity by changing the algorithm
- reducing complexity by changing the data model

## Scaffold Pages

- [Big-O And Growth](big-o-and-growth.md)
- [Time And Space Complexity](time-and-space-complexity.md)
- [Constant Factors And Cache Effects](constant-factors-and-cache-effects.md)

## Programming Examples

- C: add the primary implementation example, with bounds, error handling, and memory behavior visible.
- Python: add a reference implementation when it helps explain the algorithm, generate test cases, or compare behavior.

## Embedded And Systems Angle

- distinguish asymptotic cost from cache, branch, syscall, and allocation costs
- prefer bounded worst-case behavior for real-time paths
- account for stack, heap, DMA buffers, and persistent storage separately
- measure hot paths instead of assuming Big-O tells the whole story

## Related Topics

- [Algorithmic Foundations](../algorithmic-foundations/index.md)
- [Sorting And Ordering](../sorting-and-ordering/index.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)
