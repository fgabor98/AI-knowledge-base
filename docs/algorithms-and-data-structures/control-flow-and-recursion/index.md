---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Control Flow And Recursion

Roadmap for the control patterns used to build algorithms.

## Coverage

- sequence
- branching
- iteration
- loop invariants
- loop termination
- recursion fundamentals
- base cases
- recursive cases
- recursive call stack
- recursion vs iteration
- divide-and-conquer thinking
- tail-recursive shape

## Scaffold Pages

- [Loop Invariants And Termination](loop-invariants-and-termination.md)
- [Recursion Fundamentals](recursion-fundamentals.md)
- [Divide And Conquer](divide-and-conquer.md)

## Programming Examples

- C: add the primary implementation example, with bounds, error handling, and memory behavior visible.
- Python: add a reference implementation when it helps explain the algorithm, generate test cases, or compare behavior.

## Embedded And Systems Angle

- understand stack-depth risk before using recursion
- prefer bounded iteration where stack use must be predictable
- use explicit stacks when recursion is unsafe but depth-first behavior is needed
- document termination conditions in low-level loops

## Related Topics

- [Basic Algorithm Schemes](../basic-algorithm-schemes/index.md)
- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)
