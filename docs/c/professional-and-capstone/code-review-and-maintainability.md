---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Code Review And Maintainability

Code review is a technical control, not an approval ritual. In C, a review must uncover
contracts the compiler cannot enforce: lifetime, bounds, aliasing, interrupt context,
ownership, reset behavior, timing, ABI, hardware side effects, and recovery. The best
review is risk-based, specific, and supported by tests or tools.

## Learning Objectives

- Review a C change in a repeatable order without reducing review to formatting.
- Prioritize findings by impact, likelihood, detectability, and reversibility.
- Evaluate APIs, invariants, ownership, concurrency, failure paths, and platform edges.
- Refactor legacy code in behavior-preserving increments.
- Distinguish a defect, a maintainability concern, a design question, and a preference.
- Leave review evidence that helps future maintainers understand the decision.

## Prepare Before Reading The Diff

A reviewer should know:

- the requirement, non-goals, and acceptance criteria;
- target, compiler, C dialect, ABI, RTOS/OS, and hardware context;
- caller/callee ownership and lifecycle;
- timing, memory, power, safety, and security budgets;
- changed interfaces, data formats, linker/startup effects, and migration plan;
- available tests, logs, traces, and known field failures.

If the change is too large to understand, request a decomposition or first review the
design record. A huge mixed formatting/refactoring/feature diff hides defects and makes
behavioral comparison difficult.

## Review In Risk Order

Use this order so a style discussion does not consume the time needed for a safety issue:

1. **System behavior:** Does the change satisfy the requirement and preserve existing
   behavior outside its scope?
2. **Failure and security:** What happens on invalid input, allocation failure, timeout,
   reset, privilege boundary, malformed data, and partial hardware operation?
3. **Memory and lifetime:** Are bounds, alignment, aliasing, initialization, ownership,
   and reclamation correct?
4. **Concurrency and context:** Are atomicity, ordering, lock rules, ISR/task rules,
   cancellation, and reentrancy correct?
5. **Platform/ABI:** Are registers, linker sections, startup, calling convention,
   cache/DMA, endianness, and compiler extensions handled explicitly?
6. **Resource budgets:** Are CPU, stack, heap, flash, latency, power, and retry bounds
   preserved?
7. **Testability and diagnostics:** Can the behavior and failures be observed and
   reproduced?
8. **Clarity and style:** Is the code consistent, unsurprising, and maintainable?

## The Review Checklist

### Interface and contract

- Are names precise about units, ownership, blocking, and failure?
- Are input/output lengths explicit and validated?
- Are nullable pointers, zero lengths, and empty states specified?
- Is the lifetime of returned data clear?
- Is the API usable from every intended context?
- Are ABI-visible types fixed-width, aligned, versioned, and padding-safe?
- Are callbacks, cancellation, and destruction races documented?

### Correctness and undefined behavior

- Are all reads initialized and all writes within object lifetime and bounds?
- Are arithmetic operations safe for the full input range, including size calculations?
- Are shifts, conversions, pointer arithmetic, and aliasing legal?
- Are `memcpy`/`memmove` overlap and alignment contracts satisfied?
- Are `restrict`, packed fields, atomics, and compiler extensions justified?
- Does error handling avoid using invalid or partially initialized state?

### Concurrency and execution context

- Which agents access each object: tasks, ISRs, DMA, cores, processes, callbacks?
- Is every shared access atomic or ordered by a lock/protocol?
- Is memory reclamation safe after cancellation and callback completion?
- Can a lock be taken in this context, and is lock order documented?
- Are priority inversion, interrupt latency, and starvation addressed?
- What happens if the peer resets or disappears between two operations?

### Hardware and platform

- Are MMIO access widths, side effects, barriers, and reset values correct?
- Are cache, DMA, MPU/MMU, endianness, and address-space assumptions explicit?
- Does the linker place code/data where the algorithm expects?
- Are vector/handler names and ABI attributes correct?
- Are clock, power, watchdog, and fault paths included?
- Does generated code match the target ISA and deployment baseline?

### Verification and operations

- Does a test fail before the fix and pass after it?
- Are boundary, negative, timeout, reset, and allocation-failure cases covered?
- Are static-analysis, sanitizer, fuzz, coverage, and hardware results relevant to the
  change?
- Can a field crash be diagnosed from logs, fault records, counters, and build IDs?
- Are configuration and release artifacts traceable to the source revision?

## Classifying Review Findings

Use precise categories:

- **Blocking defect:** correctness, safety, security, data loss, build, or requirement
  failure; the change should not merge.
- **Required change:** a contract or test is missing and risk cannot be accepted without
  addressing it.
- **Question:** intent or assumption is unclear; the author should explain or document.
- **Suggestion:** a maintainability/performance improvement that is not required for
  correctness.
- **Nit:** style or wording with no material behavior impact.

A good finding contains the location, observed risk, reasoning, and suggested evidence
or fix. “This is unsafe” is less useful than “`length + header_size` can wrap before
the bounds check; check `length > SIZE_MAX - header_size` and add a maximum-length test.”

## Review Evidence, Not Trust

For a nontrivial change, ask for the smallest evidence that resolves uncertainty:

- a unit test for an invariant;
- a static assertion for layout;
- a sanitizer/fuzz result for memory/input behavior;
- a map/disassembly diff for ABI or placement;
- a timing trace for latency;
- a fault-injection result for recovery;
- a benchmark with workload and environment recorded.

Evidence should be reproducible by another engineer. A screenshot or “tested on my
board” is useful context but not a durable verification record.

## API Clarity And Invariants

Prefer APIs that make invalid states harder to express:

- pair pointers with lengths;
- use opaque handles for owned resources;
- return status separately from output values;
- make units and ownership visible in names/types;
- centralize state transitions instead of exposing writable fields;
- provide one destruction function for each allocation path;
- use explicit constructors for representations requiring validation.

Document invariants near the code that enforces them. If a queue requires one producer
and one consumer, state it in the type/API and assert it in design/tests. If a function
requires a non-overlapping buffer, state the precondition and use `restrict` only when
the callers can guarantee it.

## Reviewing Legacy Code

Legacy code often works because an undocumented assumption happens to hold. Before
changing it:

1. Identify externally observable behavior and safety boundaries.
2. Add characterization tests, traces, or assertions around the current behavior.
3. Record known defects and assumptions separately from the requested change.
4. Make one behavior-preserving structural change at a time.
5. Re-run tests and compare generated image/timing/output.
6. Change semantics only in a separate, reviewed step with migration evidence.

Do not begin by rewriting a driver, allocator, or state machine wholesale unless the
team can preserve and compare behavior. Small seams—an adapter, fake interface, explicit
context, or parser reader—make legacy code testable without requiring a full rewrite.

## Refactoring Tactics

- **Extract a pure function:** isolate arithmetic/protocol policy from I/O and state.
- **Introduce a seam:** wrap hardware, time, allocation, or filesystem operations.
- **Name an invariant:** replace a magic condition with a checked helper or state type.
- **Replace implicit ownership:** add create/destroy or a buffer-view structure.
- **Split initialization from operation:** make startup prerequisites and failure clear.
- **Reduce global state:** pass context explicitly and make dependencies visible.
- **Stabilize a boundary:** add an opaque handle or versioned function table.
- **Delete dead code with evidence:** use coverage, link reports, and call-site review.

Avoid “cleanup” that changes integer widths, packing, evaluation order, timeout units,
interrupt priority, or initialization order without a dedicated design review.

## Technical Debt

Debt is not simply old code. It is a known future cost or risk created by a shortcut.
Record its mechanism and consequence:

```text
Debt: driver polls status with no deadline.
Trigger: device can remain in reset after brownout.
Impact: task can starve and watchdog recovery is delayed.
Evidence: trace shows unbounded loop in fault injection.
Plan: add deadline, reset transition, and hardware test.
Owner/review date: ...
```

Prioritize debt that blocks diagnosis, increases failure severity, multiplies change
cost, or violates a product/safety/security requirement. A backlog item with no trigger,
impact, owner, or review point is easy to forget.

## Long-Term Ownership

Maintainability includes the ability to operate and modify the component years later:

- one clear owner and backup owner;
- documented target/configuration matrix;
- tests that run without scarce hardware where possible;
- deterministic reproduction commands;
- diagnostics and crash decoding tools;
- migration and deprecation policy;
- known limitations and field history;
- review of toolchain/library/platform upgrades.

Do not optimize for the first author. Optimize for the engineer who receives a fault
record at 03:00 with a new compiler, a different board revision, and no local debugger.

## Exercises And Diagnostics

1. Review a deliberately flawed driver using the risk order; classify each finding and
   request one piece of evidence for every blocking issue.
2. Split a large mixed diff into behavior, refactoring, and test changes; explain why
   the smaller review units reduce risk.
3. Add characterization tests and a fake hardware seam to a legacy module without
   changing its observable behavior.
4. Write a technical-debt record for an unbounded retry, global allocator, or hidden
   reset assumption and define its retirement test.
5. Conduct a handoff review: ask a second engineer to diagnose a synthetic field fault
   using only the repository's documentation and artifacts.

## Common Mistakes

- Treating review as formatting or personal style enforcement.
- Reviewing a huge mixed diff without first understanding scope and requirements.
- Accepting “tested” without a command, input, environment, or artifact.
- Focusing on happy paths while ignoring bounds, reset, cancellation, and error paths.
- Refactoring legacy code and changing behavior accidentally.
- Hiding ownership, units, blocking, or context assumptions in comments only.
- Recording technical debt without impact, owner, trigger, or review date.
- Optimizing for the original author's knowledge instead of future diagnosability.

## Related Topics

- [Professional Practice And Capstones overview](./index.md)
- [Product-Quality C Workflow](./product-quality-c-workflow.md)
- [Coding Practices](../correctness-quality-and-security/coding-practices.md)
- [Testing Strategy](../correctness-quality-and-security/testing-strategy.md)
- [Static Analysis](../correctness-quality-and-security/static-analysis.md)
- [Debugging With GDB](../correctness-quality-and-security/debugging-with-gdb.md)

## References

- [MISRA C](https://misra.org.uk/misra-c/)
- [CERT C Secure Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
- [Clang-Tidy documentation](https://clang.llvm.org/extra/clang-tidy/)
- [Clang Static Analyzer](https://clang.llvm.org/docs/ClangStaticAnalyzer.html)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- The project's review checklist, coding standard, issue tracker, incident reports, and
  architecture/design-record conventions
