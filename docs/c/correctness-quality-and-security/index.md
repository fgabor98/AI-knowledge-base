---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Correctness, Quality, And Security

Reliable C is produced by a system of practices rather than a single compiler switch or analysis tool. The system starts with explicit requirements and contracts, continues through readable implementation and review, and gathers evidence from tests, dynamic instrumentation, static analysis, debugging, security review, safety processes, and—when appropriate—formal verification.

This chapter is deliberately embedded-oriented. It covers host tests and sanitizers, but also explains what changes when code runs without an operating system, inside an interrupt, under an RTOS, with memory-mapped I/O, or on a device whose failures must be diagnosed after deployment.

## The Evidence Ladder

Use several complementary layers:

```text
requirements and hazards
          |
          v
contracts, invariants, architecture, coding rules
          |
          v
review + compiler diagnostics + static analysis
          |
          v
unit, integration, property, fault, timing, and HIL tests
          |
          v
sanitizers, tracing, GDB, crash evidence, field telemetry
          |
          v
formal proof for selected high-value properties
```

No layer dominates all others:

- a unit test can prove one example but miss another path;
- a sanitizer can expose a runtime memory error but only on executed paths;
- static analysis can explore paths but may be imprecise or configuration-sensitive;
- a debugger explains one execution and can perturb timing;
- a coding standard constrains risky constructs but cannot prove the requirements;
- a formal proof can establish a property under a model that may not include the hardware or integration environment.

Senior practice means knowing what each result does and does not establish.

## Recommended Progression

1. [Coding Practices](./coding-practices.md) — make intent, ownership, invariants, and changes reviewable.
2. [Portability](./portability.md) — separate ISO C guarantees from target contracts.
3. [Testing Strategy](./testing-strategy.md) — build layered evidence from host to hardware.
4. [Static Analysis](./static-analysis.md) — find defects across paths and enforce project rules.
5. [Sanitizers And Dynamic Analysis](./sanitizers-and-dynamic-analysis.md) — instrument executable behavior.
6. [Debugging With GDB](./debugging-with-gdb.md) — investigate failures at source, assembly, and hardware levels.
7. [Security](./security.md) — treat inputs, privileges, secrets, updates, and dependencies as attack surfaces.
8. [Safety Standards And MISRA](./safety-standards-and-misra.md) — build auditable safety evidence and justified deviations.
9. [Formal Methods](./formal-methods.md) — state and prove selected properties with explicit assumptions.

## Correctness Vocabulary

Keep these concepts distinct:

| Concept | Question |
| --- | --- |
| Requirement | What must the system do or prevent? |
| Assumption | What must be true about callers, hardware, timing, or environment? |
| Precondition | What must hold before an operation starts? |
| Postcondition | What does a successful operation guarantee? |
| Invariant | What remains true across states or iterations? |
| Safety property | Can a bad event never occur under stated assumptions? |
| Liveness property | Does a required event eventually occur? |
| Diagnostic | What evidence explains a failure? |
| Verification | Does the implementation satisfy a stated property? |
| Validation | Does the system solve the real user and product problem? |

For example, “the UART driver works” is not an adequate requirement. A reviewable contract might say that `uart_write` accepts a non-null buffer, never reads beyond `length`, returns the number of bytes committed, may block only from task context, is not callable from an ISR, and preserves record ordering among producers.

## Host, Target, And Context Matrices

Document where code may execute:

| Context | May block? | May allocate? | Available diagnostics | Main hazards |
| --- | --- | --- | --- | --- |
| host unit test | usually yes | usually yes | sanitizers, debugger, coverage | test doubles hiding target behavior |
| RTOS task | policy-dependent | policy-dependent | trace, task-aware debugger | priority inversion, deadlock, stack limits |
| ISR | no, normally | no | counters, retention record | latency, reentrancy, nesting, MMIO ordering |
| boot/reset | only if initialized | rarely | fault record, early UART | uninitialized RAM/libc/peripherals |
| embedded Linux process | OS-dependent | yes, with limits | GDB, core, logs, tracing | privilege, IPC, resource exhaustion |
| safety mechanism | bounded | restricted | independent monitor | common-cause and diagnostic coverage |

A test that passes on the host may still be invalid for an ISR or before the C runtime is initialized. Put context restrictions in APIs and review checklists, not only in tribal knowledge.

## Defect-Prevention Workflow

For each feature:

1. Record requirements, assumptions, threats, and hazards.
2. Define state, ownership, bounds, failure behavior, timing, and concurrency contracts.
3. Select a representation that makes invalid states difficult to express.
4. Implement the smallest cohesive change with diagnostics enabled.
5. Review the diff for behavior, not only style.
6. Run fast unit and static checks before integration tests.
7. Run sanitizers and fault injection on host-compatible code.
8. Exercise target timing, power, reset, interrupt, DMA, and hardware boundaries.
9. Store evidence with the exact source, toolchain, configuration, and image.
10. Feed escaped defects back into requirements, tests, analysis rules, or architecture.

## Quality Gates

A production C project should define explicit gates for:

- compiler warnings and approved deviations;
- formatting and generated-code policy;
- unit, integration, target, and hardware-in-loop coverage;
- static-analysis findings and baseline changes;
- sanitizer runs and suppressions;
- security review and dependency provenance;
- coding-standard compliance and deviations;
- timing, stack, heap, flash, power, and fault-injection budgets;
- reproducible artifacts and symbol/debug retention;
- release, rollback, incident-response, and vulnerability-handling processes.

Do not turn every measurement into a single percentage. A high line coverage number can coexist with no meaningful boundary or fault coverage; a clean analyzer run can reflect an incomplete model; and a low warning count can result from disabled diagnostics.

## Running Exercise

Build a bounded packet decoder and carry it through the full evidence ladder:

1. Specify maximum frame size, accepted versions, checksum rules, timeout, and error responses.
2. Implement the decoder with explicit integer-width and buffer contracts.
3. Write normal, boundary, malformed, truncated, repeated, and resource-exhaustion tests.
4. Run compiler warnings, static analysis, AddressSanitizer, UndefinedBehaviorSanitizer, and fuzz/property tests on the host.
5. Integrate it with a target transport using a bounded buffer and watchdog-aware timeout.
6. Exercise reset, power loss, malformed input, and timing overload on hardware.
7. Review against CERT/MISRA rules selected for the project.
8. Add an ACSL contract or model for one critical parser property.
9. Preserve reports, firmware, map, symbols, and configuration as release evidence.

## Chapter Outcomes

After completing this chapter, you should be able to:

- turn vague quality goals into contracts and measurable evidence;
- write C that exposes ownership, bounds, state transitions, and context restrictions;
- design tests that cover behavior, faults, timing, resource limits, and hardware boundaries;
- combine compiler diagnostics, static analysis, sanitizers, GDB, and formal methods appropriately;
- distinguish portability defects from target-specific defects and tool limitations;
- perform threat modeling and secure input, memory, privilege, secret, and update handling;
- apply MISRA/CERT and safety-process guidance without confusing compliance with correctness;
- preserve enough provenance to reproduce, debug, and audit a released firmware image.

## Related Topics

- [Semantics And Memory](../semantics-and-memory/index.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Professional Practice And Capstones](../professional-and-capstone/index.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- [GCC static analyzer options](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html)
- [Clang Static Analyzer](https://clang.llvm.org/docs/ClangStaticAnalyzer.html)
- [Clang sanitizer documentation](https://clang.llvm.org/docs/)
- [GDB documentation](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
- [MISRA C resources](https://misra.org.uk/)
- [NIST Secure Software Development Framework SP 800-218](https://csrc.nist.gov/pubs/sp/800/218/final)
- [MITRE CWE](https://cwe.mitre.org/)
- [Frama-C and ACSL](https://frama-c.com/acsl.html)
