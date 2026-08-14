---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Algorithm Design Techniques

Algorithm design techniques are reusable ways to turn a problem statement into a state representation, a transition or choice rule, and a correctness argument. They sit between basic loop patterns and domain-specific data structures.

The important question is not “which named algorithm do I remember?” It is “what information must be retained so that the next decision is safe?” A good design makes that information, its bounds, and its failure policy visible.

## A Design Workflow

Use the following sequence when approaching a new problem:

1. Define the input, output, and invalid-input policy.
2. Identify the objective: feasibility, minimum cost, maximum value, count, ordering, or a certificate.
3. Write a small brute-force or reference solution for tiny inputs.
4. Choose a state representation that preserves the information needed by later decisions.
5. State an invariant or proof obligation before optimizing.
6. Calculate time, auxiliary memory, recursion depth, and maximum intermediate values.
7. Decide how partial work, cancellation, overflow, and allocation failure are reported.
8. Compare the optimized implementation with the reference on generated cases.

This workflow prevents a fast implementation of the wrong problem. It also separates the mathematical algorithm from the storage and scheduling decisions required by systems code.

## Choosing A Technique

| Problem shape | Useful technique | Main proof question |
| --- | --- | --- |
| A locally best choice appears irreversible | greedy algorithm | can every optimal solution be transformed to include the choice? |
| The same smaller subproblem occurs repeatedly | dynamic programming | what state contains all information needed for the transition? |
| Individual operations are occasionally expensive | amortized analysis | can the expensive work be charged across a sequence? |
| The input is recursive or split into subproblems | recurrence relation | how many subproblems and how much combine work occur at each level? |
| Correctness is easier to state than to observe directly | reference model and properties | what must remain true for every valid input? |
| Data arrives incrementally with boundaries and malformed cases | parser state machine | can every byte advance, reject, or complete without ambiguity? |

The categories overlap. A shortest-path implementation can use a greedy relaxation rule and a heap; a parser can use a finite-state machine and a ring buffer; a dynamic program can be optimized using a deque or bitset.

## Proof Before Optimization

For a greedy algorithm, write the exchange or cut argument. For dynamic programming, write the recurrence and explain why every valid solution is represented. For a parser, define the accepted language and the state invariant. For a data-processing loop, describe what the state means after the processed prefix.

If the proof requires an assumption such as non-negative weights, sorted input, bounded lengths, or a canonical denomination system, make that assumption part of input validation or the public contract.

## Resource Accounting

Record more than asymptotic time:

- maximum number of elements stored at once
- maximum recursion or parser-state depth
- maximum queue, heap, or work-list occupancy
- maximum integer value and overflow behavior
- number of passes over external or cache-cold data
- allocation and deallocation points
- blocking, retries, and cancellation checks

An O(n) algorithm that allocates on every item may be unsuitable for a real-time path. An O(n log n) bounded sort may be preferable to an unbounded heuristic if it gives a clear completion bound.

## Pages In This Section

- [Greedy Algorithms](greedy-algorithms.md) — local choices, exchange arguments, and counterexamples.
- [Dynamic Programming](dynamic-programming.md) — state, transitions, memoization, tabulation, and reconstruction.
- [Amortized Analysis And Recurrence Relations](amortized-analysis-and-recurrence-relations.md) — sequence-wide cost and recursive growth.
- [Algorithm Testing Fuzzing And Reference Models](algorithm-testing-fuzzing-and-reference-models.md) — invariants, differential checks, and reproducible failures.
- [String And Protocol Parsing Algorithms](string-and-protocol-parsing-algorithms.md) — bounded incremental parsing and wire-format validation.

## Embedded And Systems Angle

- prefer a technique whose maximum state can be calculated before deployment
- keep algorithmic failure distinct from resource exhaustion and malformed input
- use fixed-capacity tables, queues, and stacks when limits are known
- make degradation, retry, timeout, and cancellation policy part of the algorithm
- use a simple reference model to test optimized C, SIMD, or concurrent code
- treat parsers as security boundaries: every length and index needs a checked path

## Review Checklist

- Is the objective stated precisely?
- Is each important precondition checked or documented?
- Is there an invariant, recurrence, or proof sketch?
- Does the implementation represent “not found,” “infinite,” and “partial” explicitly?
- Are worst-case storage, work, and intermediate values bounded?
- Can a small independent implementation serve as a test oracle?
- Are ties, duplicate inputs, empty inputs, and malformed inputs covered?

## Related Topics

- [Algorithmic Foundations](../algorithmic-foundations/index.md)
- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
- [Control Flow And Recursion](../control-flow-and-recursion/index.md)
- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Data Structures For Algorithms](../data-structures-for-algorithms/index.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)
