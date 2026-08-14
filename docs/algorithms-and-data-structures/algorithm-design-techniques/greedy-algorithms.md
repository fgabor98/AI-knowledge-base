---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Greedy Algorithms

Roadmap for algorithms that build a solution by repeatedly making a locally best choice.

## Coverage

- greedy choice and optimal substructure
- exchange arguments
- interval scheduling
- deadline and resource selection
- canonical coin-change cases and counterexamples
- when greedy choices fail and dynamic programming is needed
- deterministic tie-breaking

## Core Questions

- What is the local choice?
- Why can an optimal solution be transformed to include that choice?
- What invariant describes the partial solution?
- Are choices irreversible?
- Which input properties make the greedy proof valid?

## Programming Examples

- C: add a bounded interval-scheduling or resource-selection implementation.
- Python: use a compact comparison against an exhaustive reference for small inputs.

## Embedded And Systems Angle

- define tie-breaking so scheduling and diagnostics are reproducible
- state whether a greedy result is optimal, feasible, or best effort
- avoid assuming coin-change or packing heuristics are universally optimal
- bound sorting and candidate storage before entering a critical path

## Future Material

- exchange-argument walkthrough
- interval scheduling with stable tie policy
- counterexamples for naïve greedy rules
- comparison with backtracking and dynamic programming

## Related Topics

- [Dynamic Programming](dynamic-programming.md)
- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Sorting And Ordering](../sorting-and-ordering/index.md)
- [Priority And Partial Ordering](../sorting-and-ordering/priority-and-partial-ordering.md)
