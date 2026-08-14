---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Data Structures For Algorithms

Overview of data structures as algorithm enablers rather than standalone names to memorize.

## Coverage

- arrays
- strings and byte buffers
- structs and records
- linked lists
- stacks
- queues
- ring buffers
- hash tables
- heaps and priority queues
- trees
- graphs
- bitsets and bitmaps
- intrusive data structures
- memory pools and fixed-size allocators

## Pages In This Section

- [Arrays Buffers And Records](arrays-buffers-and-records.md)
- [Linked Lists Stacks And Queues](linked-lists-stacks-and-queues.md)
- [Ring Buffers](ring-buffers.md)
- [Hash Tables](hash-tables.md)
- [Heaps And Priority Queues](heaps-and-priority-queues.md)
- [Bitsets And Bitmaps](bitsets-and-bitmaps.md)
- [Intrusive Data Structures](intrusive-data-structures.md)
- [Memory Pools And Fixed-Size Allocators](memory-pools-and-fixed-size-allocators.md)
- [Deques](deques.md)
- [Disjoint-Set Union](disjoint-set-union.md)

## Programming Examples

- C: use bounded implementations with explicit invariants, ownership, and failure behavior.
- Python: use semantic reference models and test generators where helpful.

## Embedded And Systems Angle

- choose structures by operations, invariants, ownership, and bounds
- prefer arrays and fixed-size buffers when maximum size is known
- use ring buffers for producer-consumer and interrupt-to-thread handoff
- use bitmaps for compact resource tracking
- use intrusive structures when allocation and ownership need to be explicit

## Related Topics

- [Algorithmic Foundations](../algorithmic-foundations/index.md)
- [Graph Algorithms](../graph-algorithms/index.md)
- [Tree Algorithms](../tree-algorithms/index.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)
