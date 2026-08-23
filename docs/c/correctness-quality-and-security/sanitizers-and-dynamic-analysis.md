---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Sanitizers And Dynamic Analysis

Dynamic analysis instruments a program or observes it while it runs. Sanitizers are especially effective for host-executable C because they turn many silent memory, integer, and concurrency failures into immediate diagnostics with stack traces. They are testing tools, not a replacement for target testing: instrumentation changes memory layout, timing, resource use, and sometimes scheduling.

## Learning Objectives

- select AddressSanitizer, UndefinedBehaviorSanitizer, LeakSanitizer, ThreadSanitizer, MemorySanitizer, or Valgrind for a defect class;
- build and run sanitized C tests with useful symbols and reproducible options;
- interpret reports and reduce them to a minimal failing case;
- handle suppressions and low-level boundaries without masking defects;
- understand why sanitizers usually run on hosts rather than small MCUs;
- complement sanitizers with target checks, fault injection, and hardware diagnostics.

## Sanitizer Families

| Tool | Finds well | Important limits |
| --- | --- | --- |
| ASan | heap/stack/global out-of-bounds, use-after-free, some use-after-scope | memory and runtime overhead; not every target |
| UBSan | selected undefined behavior such as signed overflow, bad shifts, misalignment | depends on enabled checks and runtime policy |
| LSan | leaks at process end | weak for intentional pools and long-lived firmware |
| TSan | data races and synchronization mistakes | high overhead; target/library support constraints |
| MSan | uses of uninitialized data | requires instrumented dependencies and careful setup |
| Valgrind Memcheck | memory errors and leaks through dynamic instrumentation | much slower; different behavior from compiler instrumentation |
| hardware watchpoints/MPU | selected runtime corruption | limited coverage and target resources |

Choose a combination based on the defect and build environment. ASan and TSan are generally not combined in one normal build; use separate jobs where supported.

## AddressSanitizer

Build a host test with debug information:

~~~sh
clang -std=c17 -g -O1 -fno-omit-frame-pointer \
      -fsanitize=address,undefined \
      -Wall -Wextra test_packet.c packet.c -o test_packet
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./test_packet
~~~

ASan surrounds allocations and selected objects with poisoned regions and reports accesses to invalid memory. The report usually includes the operation, address, object allocation/free stack, and source locations. Fix the first invalid access; later failures may be consequences.

Run with realistic input and failure tests. A passing sanitized unit suite means only that the executed paths were clean under the instrumentation and runtime model.

## UndefinedBehaviorSanitizer

UBSan instruments selected operations that can violate the language or implementation contract:

~~~sh
clang -std=c17 -g -O1 -fsanitize=undefined \
      -fno-sanitize-recover=undefined \
      arithmetic_tests.c -o arithmetic_tests
~~~

Relevant checks can include signed overflow, invalid shifts, out-of-bounds array operations, null or misaligned dereferences, invalid enum values, unreachable paths, and problematic conversions. Select checks explicitly when the project needs a different policy. Some embedded targets support parts of UBSan with a custom runtime; many do not support the full host experience.

Do not “fix” a UBSan report by relying on unsigned wraparound if the algorithm requires a bounded mathematical result. State and test the intended arithmetic domain.

## ThreadSanitizer

TSan instruments memory accesses and synchronization to detect data races in supported threaded programs:

~~~sh
clang -std=c17 -g -O1 -fsanitize=thread \
      -pthread race_test.c -o race_test
./race_test
~~~

A race is a correctness defect even when the observed values “look right.” TSan may not model custom atomics, inline assembly, MMIO, RTOS primitives, or target synchronization correctly. Add annotations or interceptors only after verifying the primitive’s semantics, and run a target-specific concurrency review as well.

## Leak And Uninitialized-Data Analysis

LSan can find allocations that remain unreachable at process exit. Embedded firmware often intentionally keeps pools or singleton allocations alive, so define the ownership policy before suppressing a report. Prefer explicit cleanup in tests so leaks become observable before process termination.

MSan detects data used before initialization, but all relevant code—including libraries—must be instrumented or modeled. It can be expensive to adopt in a large system. Valgrind can be useful when compiler instrumentation is unavailable, but its timing and threading behavior differ from native execution.

## Low-Level And Embedded Boundaries

Sanitizers do not automatically understand:

- memory-mapped registers and volatile device windows;
- DMA writes performed outside the CPU’s instrumented address space;
- interrupt stack frames and exception entry;
- linker-defined memory regions;
- boot ROM, secure monitor, or another privilege domain;
- custom allocators, pools, and RTOS heaps;
- inline assembly and compiler barriers;
- cache maintenance and memory-protection hardware.

Create host models for pure protocol and state logic, but verify the boundary on target with MPU guards, canaries, guard regions, DMA ownership checks, stack watermarking, and fault handlers.

## Suppressions And Special Cases

Suppressions should be narrow and justified. A low-level function may require an exclusion because it intentionally inspects a stack or implements an allocator, but the exclusion should identify:

- exact function or source pattern;
- sanitizer and version;
- invariant that makes the operation valid;
- review owner and expiry;
- test that covers the boundary.

Do not suppress an entire file because it contains one assembly wrapper. Keep sanitized and unsanitized builds separate where necessary and test the wrapper’s contract independently.

## Reading A Report

For every report:

1. preserve the complete log and exact binary;
2. identify the first invalid operation;
3. inspect allocation/free or initialization history;
4. check whether a macro, optimizer, or test double changed the path;
5. reduce the input and state to a minimal reproducer;
6. fix the ownership, bounds, synchronization, or arithmetic contract;
7. add a regression test;
8. rerun the relevant sanitizer and neighboring jobs.

Do not rely on a line number alone. Inlined functions, generated code, stale binaries, and symbol mismatches can point to the wrong source context.

## Exercises

1. Create heap use-after-free, stack out-of-bounds, and signed-overflow tests and inspect their reports.
2. Add a leak to a host test and make ownership explicit rather than suppressing it.
3. Introduce a race protected by an RTOS-like fake and model the synchronization for TSan.
4. Run Valgrind and ASan on the same memory defect and compare evidence.
5. Build a custom pool and decide which checks belong in the pool, sanitizer, and tests.
6. Add a DMA/ISR boundary model and document which host checks cannot see it.
7. Turn a sanitizer regression into a minimized corpus input and permanent test.

## Common Mistakes

- treating a sanitized host run as proof of firmware safety;
- mixing sanitizer and production builds without recording the difference;
- ignoring instrumentation-induced timing and memory changes;
- suppressing reports before understanding the first invalid operation;
- assuming TSan recognizes custom RTOS or assembly synchronization;
- forgetting to instrument or model dependent libraries;
- using LSan assumptions for intentionally persistent embedded pools;
- leaving sanitizer runtimes or unsafe diagnostics in a production image;
- resolving a report against stale symbols or a different binary.

## Related Topics

- [Testing Strategy](./testing-strategy.md)
- [Static Analysis](./static-analysis.md)
- [Debugging With GDB](./debugging-with-gdb.md)
- [Memory Safety And Lifetime](../semantics-and-memory/memory-safety-and-lifetime.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)

## References

- [Clang AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [Clang ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html)
- [Clang MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html)
- [Clang LeakSanitizer](https://clang.llvm.org/docs/LeakSanitizer.html)
- [Valgrind documentation](https://valgrind.org/docs/manual/manual.html)
