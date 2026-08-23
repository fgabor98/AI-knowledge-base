---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Formal Methods

Formal methods use mathematical models, specifications, and algorithms to establish properties of software. They range from executable assertions and model checking to abstract interpretation and deductive proof. Their value is highest when the property is important, the state space is difficult to cover by tests, and the assumptions and specification can be made precise.

Formal verification does not prove that a system meets an unspecified requirement, that the hardware behaves as modeled, or that the compiler emitted the intended image. It proves a stated property of a stated model under stated assumptions.

## Learning Objectives

- express preconditions, postconditions, invariants, and temporal properties;
- distinguish runtime assertions, static analysis, model checking, and deductive proof;
- use contracts to guide implementation, tests, and analysis;
- understand abstraction, proof obligations, assumptions, and vacuous proofs;
- apply ACSL/Frama-C-style reasoning to bounded C modules;
- choose formal work that provides useful evidence for an embedded product.

## The Formalization Ladder

| Technique | Typical question | Result |
| --- | --- | --- |
| assertion | does this state hold at runtime? | failure or observed pass |
| contract | what must callers provide and what does a function guarantee? | explicit interface property |
| abstract interpretation | can values, ranges, or alarms violate a property? | alarms or computed invariants |
| bounded model checking | is there a counterexample within a bound? | proof within bound or trace |
| symbolic execution | which input conditions reach this path? | path constraints/counterexamples |
| deductive verification | can a function satisfy its contract for all modeled inputs? | proof obligations |
| temporal model checking | can a state machine always avoid/ultimately reach states? | property proof/counterexample |

Use the lightest technique that gives credible evidence, then escalate for high-risk properties.

## Contracts

A contract separates caller obligations from implementation guarantees:

```text
requires: buffer is valid for [0, length), capacity is sufficient
ensures:  output represents exactly the accepted input
assigns:  only output fields and permitted state change
exits:    returns success or a classified failure
```

An embedded contract also includes context and hardware assumptions: task versus ISR, clock state, DMA ownership, register accessibility, interrupt masking, and maximum execution time.

For an API, write the contract before choosing the implementation. A postcondition that cannot be tested, checked, or reviewed is often too vague.

## Invariants

An invariant is true before and after every relevant operation or loop iteration. For a ring buffer:

- `0 <= count <= capacity`;
- `head` and `tail` are within the storage range;
- an empty buffer has no readable elements;
- a full buffer has exactly `capacity` stored elements;
- producer and consumer ownership rules are respected.

State invariants should be centralized in checks or helper functions where practical:

~~~c
#include <stdbool.h>
#include <stddef.h>

struct ring_view {
    size_t head;
    size_t tail;
    size_t count;
    size_t capacity;
};

static bool ring_is_valid(const struct ring_view *ring)
{
    return ring != NULL
        && ring->capacity > 0u
        && ring->head < ring->capacity
        && ring->tail < ring->capacity
        && ring->count <= ring->capacity;
}
~~~

An invariant checker is useful in tests and debug builds, but a proof or runtime assertion is meaningful only if the representation and concurrency model are correct.

## ACSL-Style Contracts

ACSL is a specification language used by Frama-C for C properties. A bounded copy might be documented as:

~~~c
#include <stddef.h>

/*@
  requires n == 0 || (\valid_read(source + (0 .. n - 1)) &&
                      \valid(destination + (0 .. n - 1)));
  assigns destination[0 .. n - 1];
  ensures \forall integer i; 0 <= i < n ==>
            destination[i] == source[i];
*/
void copy_bytes(unsigned char *destination,
                const unsigned char *source,
                size_t n);
~~~

The notation is not C syntax; it is consumed by a compatible specification tool. The contract must match the actual function, including overlap, volatile memory, aliasing, and failure behavior. A false or incomplete contract can make a proof meaningless.

## Proof Obligations And Assumptions

Tools may generate obligations such as:

- an index stays within an array;
- a pointer is valid and initialized;
- an integer operation does not overflow;
- a loop terminates or preserves an invariant;
- a lock is held before accessing shared state;
- a return value satisfies the postcondition.

For each discharged obligation, record whether it was proved automatically, by a lemma, by a trusted assumption, or by an external fact. Review assumptions such as “the hardware never returns more than 1024 bytes” and connect them to a driver contract or test.

Beware vacuous proofs. If a precondition is impossible because of a bad model, every postcondition can appear true. Test that the entry assumptions are reachable and that the model includes the real caller and configuration.

## Abstract Interpretation

Abstract interpretation computes an over-approximation of possible states. If it proves that all possible values satisfy a bound, that can be strong evidence. If it emits an alarm, improve precision or inspect the defect. If the model excludes a path through an undocumented assumption, the result is only conditional.

Frama-C Eva, for example, can analyze value ranges and emit alarms for potential runtime errors. A typical workflow is:

~~~sh
frama-c -eva -main packet_decode src/packet.c src/checksum.c
~~~

The exact options and results depend on the Frama-C release, compiler preprocessing, entry-point assumptions, and annotations.

## Model Checking And State Machines

Model checking is well suited to finite protocol, scheduler, and safety state machines. Model:

- states and transitions;
- inputs and nondeterminism;
- timing/ordering abstraction;
- safety properties (“never”);
- liveness properties (“eventually”);
- fairness and environment assumptions.

Use a small model to explore deadlock, illegal command ordering, reset recovery, queue bounds, and watchdog behavior. Then connect the model states to C implementation states and add conformance tests. A model that omits DMA, interrupts, or reset may still be useful, but its scope must be explicit.

## Bounded Model Checking And Symbolic Execution

Bounded methods search executions up to a selected depth or resource bound. They are excellent for finding short counterexamples in parsers, arithmetic, state transitions, and error paths. A result “no counterexample up to bound 50” is not the same as an unbounded proof unless the bound is justified by an invariant or finite-state argument.

Symbolic execution represents inputs as symbolic values and explores path conditions. It can find a value that reaches a dangerous operation, but path explosion, external calls, concurrency, and hardware can limit coverage. Use counterexamples as regression tests and simplify the path before changing code.

## Concurrency And Hardware

Formal reasoning about embedded concurrency must include:

- interrupt and task interleavings;
- atomicity and memory ordering;
- DMA writes and cache ownership;
- volatile/MMIO semantics;
- disabling/enabling interrupts;
- priority inversion and bounded blocking;
- reset and power transitions;
- hardware faults and watchdog reaction.

Do not model an MMIO register as an ordinary variable unless the model states which reads have side effects and which writes are externally observed. A proof over sequential C can miss a real race or hardware ordering defect.

## Requirements And Proof Traceability

For a critical property, maintain:

1. requirement and hazard identifier;
2. formal property and assumptions;
3. model boundary and abstraction;
4. implementation mapping;
5. tool/version/configuration;
6. proof result and unresolved obligations;
7. tests and counterexample regression cases;
8. reviewer and change history.

Formal work becomes part of the product evidence when another engineer can reproduce the result and understand what was trusted.

## Choosing What To Prove

Good candidates include:

- packet length and index safety;
- absence of arithmetic overflow in safety-critical calculations;
- queue/ring-buffer invariants;
- state-machine illegal transition exclusion;
- fail-safe reaction to invalid commands;
- memory ownership and separation at an API;
- watchdog and deadline properties within an abstract model;
- cryptographic wrapper preconditions and output handling.

Poor first candidates include a huge unconstrained application, an unstable hardware abstraction, or a vague property such as “the driver is correct.” Start with a small, stable, high-consequence boundary.

## Exercises

1. Write preconditions, postconditions, and assigns behavior for a bounded buffer function.
2. Add a ring-buffer invariant checker and test every transition.
3. Run a value analysis on a parser and classify each alarm.
4. Model a three-state bootloader update protocol and find illegal reset transitions.
5. Create a bounded search for an integer-overflow counterexample.
6. Add a formal assumption for a hardware register and document how it is validated on target.
7. Build a traceability record from one safety requirement through proof, tests, and release artifacts.

## Common Mistakes

- proving an incomplete or false contract;
- confusing “no counterexample within a bound” with an unbounded proof;
- treating analyzer assumptions as established hardware facts;
- ignoring vacuous proofs and unreachable preconditions;
- modeling concurrency as sequential execution;
- treating MMIO and DMA as ordinary memory;
- omitting tool configuration and versions from evidence;
- choosing a large unstable system before proving a small critical boundary;
- failing to turn formal counterexamples into regression tests.

## Related Topics

- [Testing Strategy](./testing-strategy.md)
- [Static Analysis](./static-analysis.md)
- [Safety Standards And MISRA](./safety-standards-and-misra.md)
- [Memory Safety And Lifetime](../semantics-and-memory/memory-safety-and-lifetime.md)
- [Atomics, Threads, And Signals](../standard-library-and-ecosystem/atomics-threads-and-signals.md)

## References

- [Frama-C ACSL](https://frama-c.com/acsl.html)
- [Frama-C Eva value analysis](https://frama-c.com/value.html)
- [Frama-C documentation](https://www.frama-c.com/html/documentation.html)
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
- [NIST Secure Software Development Framework](https://csrc.nist.gov/pubs/sp/800/218/final)
