---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Basic Algorithm Schemes

Roadmap for small reusable algorithm patterns.

## Coverage

- summation
- counting
- minimum search
- maximum search
- existence and decision checks
- selection
- filtering
- accumulation
- linear search
- sentinel search
- ordered linear search
- binary search
- recursive binary search

## Scaffold Pages

- [Linear Scan Patterns](linear-scan-patterns.md)
- [Binary Search](binary-search.md)

## Programming Examples

- C: add the primary implementation example, with bounds, error handling, and memory behavior visible.
- Python: add a reference implementation when it helps explain the algorithm, generate test cases, or compare behavior.

## Embedded And Systems Angle

- use simple schemes as the default for small bounded inputs
- avoid clever algorithms when input sizes are known and tiny
- use sentinels only when mutation and bounds are safe
- make failure returns explicit for searches and selections

## Related Topics

- [Control Flow And Recursion](../control-flow-and-recursion/index.md)
- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Data Structures For Algorithms](../data-structures-for-algorithms/index.md)
