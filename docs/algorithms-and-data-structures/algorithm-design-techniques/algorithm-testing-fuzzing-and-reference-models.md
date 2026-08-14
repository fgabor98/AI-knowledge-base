---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Algorithm Testing Fuzzing And Reference Models

Roadmap for validating algorithms beyond a few hand-written examples.

## Coverage

- example and counterexample design
- boundary-value and failure-path testing
- invariant checks
- reference models and differential testing
- property-based test generation
- fuzzing parsers, buffers, and state machines
- reproducible random seeds and minimized failures
- testing resource limits and cancellation

## Test Oracles

A reference model can be slower or simpler than the production algorithm while preserving the same semantics. Compare outputs, statuses, mutations, and invariants—not only the happy-path result.

Useful properties include:

- sorting preserves element multiplicity and ordering policy
- a ring buffer never returns an item twice
- a graph traversal visits only reachable valid vertices
- a bounded operation never writes beyond capacity

## Programming Examples

- C: add deterministic table-driven tests and invariant assertions.
- Python: generate small cases and compare optimized C behavior with a simple reference model.

## Embedded And Systems Angle

- keep failing seeds and input bytes reproducible
- test allocation failure, full queues, depth limits, and overflow
- distinguish a minimized test case from the original production trace
- run expensive fuzzing off-target while retaining target-specific boundary tests

## Future Material

- differential test harness for C and Python implementations
- fuzzing a length-delimited parser
- invariant instrumentation patterns
- failure triage and corpus management

## Related Topics

- [Algorithmic Foundations](../algorithmic-foundations/index.md)
- [Invariants And Correctness](../algorithmic-foundations/invariants-and-correctness.md)
- [Problem Modeling](../algorithmic-foundations/problem-modeling.md)
- [String And Protocol Parsing Algorithms](string-and-protocol-parsing-algorithms.md)
