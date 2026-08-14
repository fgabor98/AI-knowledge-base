---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Sorting And Ordering

Overview of ordering as both an algorithmic operation and a design choice.

## Coverage

- why ordering changes algorithms
- comparison functions
- stable vs unstable sorting
- in-place vs out-of-place sorting
- insertion sort as a learning algorithm
- selection sort as a learning algorithm
- bubble sort as a learning algorithm
- merge-style thinking
- sorting as preprocessing
- ordered lookup
- maintaining sorted data
- partial ordering and priority

## Pages In This Section

- [Sorting Fundamentals](sorting-fundamentals.md)
- [Maintaining Sorted Data](maintaining-sorted-data.md)
- [Priority And Partial Ordering](priority-and-partial-ordering.md)

## Programming Examples

- C: use bounded implementations with explicit comparison, error, and memory behavior.
- Python: use reference implementations and test oracles where helpful.

## Embedded And Systems Angle

- prefer predictable in-place algorithms for small fixed arrays
- understand when sorting once enables simpler repeated lookup
- keep comparison functions total, deterministic, and side-effect free
- watch stack and temporary-buffer use in recursive or merge-based algorithms

## Related Topics

- [Basic Algorithm Schemes](../basic-algorithm-schemes/index.md)
- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
- [Data Structures For Algorithms](../data-structures-for-algorithms/index.md)
