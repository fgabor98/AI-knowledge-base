---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Modular Design And APIs

A C module is a boundary around data, behavior, dependencies, and ownership. A good module can be compiled, tested, replaced, and reviewed without requiring every caller to understand its representation.

This chapter moves from translation units and preprocessing to public APIs, opaque types, resource lifetimes, errors, callbacks, and architecture patterns. The embedded emphasis is deliberate: a driver interface is also a timing contract, an ownership protocol, and a boundary between portable logic and hardware policy.

## What This Chapter Teaches

A stable C interface answers more than “what function do I call?” It answers:

- Which headers and declarations are required?
- Which objects are owned, borrowed, transferred, or shared?
- Which inputs and outputs are valid, and what are their bounds?
- What happens on partial failure?
- Can the call block, allocate, sleep, or access hardware?
- Which execution contexts may call it?
- What state survives reset or shutdown?
- Which representation and ABI assumptions cross the boundary?
- How can the dependency be replaced by a test double?

## Recommended Progression

1. [Translation Units And Headers](./translation-units-and-headers.md) — package declarations and definitions without symbol or dependency surprises.
2. [Preprocessor And Macros](./preprocessor-and-macros.md) — use textual configuration without making code untyped and opaque.
3. [APIs And Opaque Types](./api-and-opaque-types.md) — define stable contracts and hide representations.
4. [Ownership And Resource Lifetimes](./ownership-and-resource-lifetimes.md) — make cleanup and asynchronous lifetimes explicit.
5. [Error Handling](./error-handling.md) — propagate failure without losing context or entering unsafe states.
6. [Callbacks And Function Tables](./callbacks-and-function-tables.md) — inject behavior and model driver/event interfaces.
7. [Architecture Patterns](./architecture-patterns.md) — compose modules into testable embedded systems.

## Module Anatomy

A typical module has:

| Part | Responsibility | Review question |
| --- | --- | --- |
| Public header | Stable types, constants, and declarations | Can a caller use this without private knowledge? |
| Private header | Shared implementation details within a subsystem | Is its visibility narrower than the public API? |
| Source file | Representation and behavior | Does it own the state it claims to own? |
| Configuration | Build- or target-specific choices | Are unsupported combinations rejected? |
| Tests | Contract and failure evidence | Can behavior be tested without real hardware? |
| Documentation | Lifetime, timing, errors, and context | Could another engineer call it safely? |

A module should have one reason for change where practical. A driver that also parses a protocol, owns application policy, and prints diagnostics is difficult to test and difficult to reuse.

## Dependency Direction

Prefer a dependency graph that points from policy toward mechanisms:

- application policy depends on a service interface;
- the service interface depends on domain types, not a concrete board;
- a platform adapter implements the interface;
- tests provide a fake adapter.

Avoid cycles between headers and modules. A cycle usually indicates that a type, callback, or service boundary has been placed at the wrong layer.

Use forward declarations and opaque types to break representation dependencies. Use explicit dependency injection when a module needs a clock, allocator, logger, or hardware operation.

## Interface Quality Checklist

Before publishing a function or type, check:

- names express units and ownership;
- input pointers have null and lifetime rules;
- lengths and capacities are explicit;
- outputs are defined on failure;
- status values are stable and documented;
- side effects and blocking behavior are stated;
- reentrancy and execution-context rules are stated;
- resource release and shutdown order are stated;
- representation and ABI requirements are stated;
- the API can be tested with a fake or host implementation;
- unsupported configurations fail at compile time or initialization.

## Embedded Context Matrix

The same API may be legal in one context and forbidden in another:

| Context | Typical restrictions |
| --- | --- |
| Initialization | Ordering, reset state, clocks, and failure rollback |
| Thread/task | Blocking and synchronization may be allowed |
| Interrupt | Usually no blocking or general allocation; bounded execution |
| DMA callback | Ownership and cache state must be transferred correctly |
| Bootloader | Small runtime, limited storage, recovery priority |
| Fault handler | Reentrancy, stack, and hardware availability are constrained |
| Host test | Fakes, sanitizers, and deterministic fault injection are available |

Document the allowed context next to the interface. A function name alone cannot communicate these constraints.

## A Small Dependency Boundary

A portable service can depend on a narrow port:

~~~c
#ifndef CLOCK_PORT_H
#define CLOCK_PORT_H

#include <stdint.h>

typedef uint32_t (*clock_now_fn)(void *context);

struct clock_port {
    clock_now_fn now;
    void *context;
};

#endif
~~~

The application does not need to know whether the clock comes from a timer register, RTOS tick, Linux clock, or a test counter. The adapter owns that decision.

## Design Review Questions

Ask these questions during review:

1. What is the smallest public surface?
2. Can a caller create an invalid object state?
3. Who owns every pointer after each call?
4. What does failure leave behind?
5. What happens if initialization runs twice?
6. What happens if shutdown races with a callback or DMA completion?
7. Which dependencies are concrete when they could be injected?
8. Does the abstraction preserve the hardware’s important semantics?
9. Is the interface stable across compiler, ABI, and firmware versions?
10. Can the tests exercise timeouts, allocation failure, and unexpected hardware events?

## Chapter Outcomes

After completing this chapter, you should be able to:

- organize a multi-file C subsystem with clear public and private boundaries;
- use headers without duplicate definitions or accidental dependency cycles;
- decide when a macro should become an inline function or a typed API;
- design opaque handles and versioned configuration interfaces;
- draw ownership and cleanup graphs for resources;
- choose status, error, retry, and recovery policies;
- build callback and function-table interfaces with safe context lifetimes;
- separate policy, ports, adapters, and hardware mechanisms;
- replace concrete dependencies with fakes for host tests;
- evaluate an API for ABI, timing, context, and long-term maintenance risk.

## Exercises Across The Chapter

Build a small sensor service in two implementations:

1. Define a public sensor API with an opaque handle.
2. Implement a host fake and a target adapter behind the same port.
3. Add explicit initialization, shutdown, ownership, and error contracts.
4. Add a callback for sample-ready events with context.
5. Inject timeouts, short reads, allocation failures, and stale callbacks.
6. Compile with two board configurations and reject unsupported combinations.
7. Review the resulting headers as if they were a long-lived product ABI.

## Related Topics

- [Language Fundamentals](../language-fundamentals/index.md)
- [Semantics And Memory](../semantics-and-memory/index.md)
- [Standard Library And Ecosystem](../standard-library-and-ecosystem/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- [ISO/IEC 9899 standards and drafts — WG14](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC preprocessor options](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html)
- [Clang attribute reference](https://clang.llvm.org/docs/AttributeReference.html)
- [GCC visibility and linkage options](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html)
