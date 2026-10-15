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

## Worked Session

Every report on this page comes from `bash examples/c/tool-sessions/sanitizers/run.sh`
(GCC 13.3, x86-64 Ubuntu 24.04). Clang accepts the same `-fsanitize=` flags and produces
reports in the same format; the runtime options are shared. Paths in the reports appear
as `./file.c` because the session runs in a scratch directory.

## AddressSanitizer

The defect: a function allocates room for a payload but not for its terminator.

```c
--8<-- "examples/c/tool-sessions/sanitizers/frame_copy.c:6:16"
```

Without instrumentation the program prints `STATUS=OK`: the stray byte lands in malloc's
rounding slack. With ASan:

```text
$ gcc -O0 -g -fsanitize=address frame_copy.c -o frame_copy_asan
$ ./frame_copy_asan
=================================================================
==20442==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000019 at pc 0x596c0bc502f2 bp 0x7ffdf4a4cae0 sp 0x7ffdf4a4cad0
WRITE of size 1 at 0x502000000019 thread T0
    #0 0x596c0bc502f1 in frame_to_string ./frame_copy.c:14
    #1 0x596c0bc50406 in main ./frame_copy.c:21
    ...

0x502000000019 is located 0 bytes after 9-byte region [0x502000000010,0x502000000019)
allocated by thread T0 here:
    #0 0x736389afd9c7 in malloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0x596c0bc50288 in frame_to_string ./frame_copy.c:8
    #2 0x596c0bc50406 in main ./frame_copy.c:21
    ...

SUMMARY: AddressSanitizer: heap-buffer-overflow ./frame_copy.c:14 in frame_to_string
Shadow bytes around the buggy address:
  ...
=>0x502000000000: fa fa 00[01]fa fa fa fa fa fa fa fa fa fa fa fa
  ...
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07
  Heap left redzone:       fa
  ...
==20442==ABORTING
[exit status 1]
```

Read a report in this order:

1. **The first line** gives the class (`heap-buffer-overflow`) and the bad address.
2. **The access** is a `WRITE of size 1` at `frame_copy.c:14`, which is `text[length] = '\0'`.
3. **The object** is `0 bytes after 9-byte region`: one past the end of a 9-byte
   allocation. "0 bytes after" almost always means an off-by-one.
4. **The history** shows where the object was allocated (`frame_copy.c:8`). For a
   use-after-free, ASan also prints where it was freed.
5. **The shadow map** confirms it. Each shadow byte describes 8 application bytes: `fa` is
   redzone, `00` means all 8 bytes are valid, and `01` means only the first byte is. The
   9-byte allocation is one full granule (`00`) plus one byte (`01`), and the write hit
   the second byte of the bracketed granule.

ASan stops at the first error by default (exit status 1). Fix that error first, because
later reports may be consequences of it. A passing sanitized suite means only that the
executed paths were clean under the instrumentation and runtime model.

## LeakSanitizer And Suppressions

On Linux, LeakSanitizer is part of ASan and runs when the process exits:

```c
--8<-- "examples/c/tool-sessions/sanitizers/leaks.c:5:39"
```

```text
$ ./leaks
=================================================================
==20376==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 64 byte(s) in 1 object(s) allocated from:
    #0 ... in malloc ...
    #1 ... in vendor_sdk_init ./leaks.c:27
    ...
Direct leak of 16 byte(s) in 1 object(s) allocated from:
    #1 ... in session_open ./leaks.c:12
    ...
Indirect leak of 256 byte(s) in 1 object(s) allocated from:
    #1 ... in session_open ./leaks.c:17
    ...
SUMMARY: AddressSanitizer: 336 byte(s) leaked in 3 allocation(s).
[exit status 1]
```

A *direct* leak is a block that nothing points to. An *indirect* leak is reachable only
from leaked blocks: the 256-byte buffer is lost because the 16-byte `struct session`
holding it is lost. Fix the direct leaks, and the indirect ones usually go with them.

The 64-byte leak stands in for third-party code you cannot change. Suppress it by
function name, and make the run print what it suppressed so the suppression stays
visible:

```text
$ cat lsan.supp
# LeakSanitizer suppressions: allocations whose stack contains vendor_sdk_init.
leak:vendor_sdk_init
$ LSAN_OPTIONS=suppressions=lsan.supp:print_suppressions=1 ./leaks
...
-----------------------------------------------------
Suppressions used:
  count      bytes template
      1         64 vendor_sdk_init
-----------------------------------------------------

SUMMARY: AddressSanitizer: 272 byte(s) leaked in 2 allocation(s).
[exit status 1]
```

LSan can find allocations that remain unreachable at process exit. Embedded firmware often intentionally keeps pools or singleton allocations alive, so define the ownership policy before suppressing a report. Prefer explicit cleanup in tests so leaks become observable before process termination.

## UndefinedBehaviorSanitizer

UBSan instruments selected operations that can violate the language or implementation
contract. Its default behavior surprises many teams:

```text
$ gcc -O0 -g -fsanitize=undefined checks.c -o checks
$ ./checks
checks.c:11:9: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
checks.c:12:28: runtime error: shift exponent 32 is too large for 32-bit type 'unsigned int'
sum = -2147483648, mask = 1
```

The program reported two errors, **continued, and exited with status 0**. A CI job that
only checks exit codes passes. Make UB fatal, either at run time or at build time:

```text
$ UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ./checks
checks.c:11:9: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
    #0 0x63c008bc2216 in main ./checks.c:11
    ...
[exit status 1]
$ gcc -O0 -g -fsanitize=undefined -fno-sanitize-recover=undefined checks.c -o checks_strict
$ ./checks_strict
checks.c:11:9: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
[exit status 1]
```

Prefer the build flag in CI, because it cannot be lost when someone runs the test
binary without the environment variable. Relevant checks include signed overflow,
invalid shifts, out-of-bounds array indexing, null or misaligned dereferences, invalid
`bool` and enum values, unreachable paths, and problematic conversions. Select checks
explicitly when the project needs a different policy. Some embedded targets support
parts of UBSan with a minimal runtime that traps; many do not support the full host
experience.

Do not “fix” a UBSan report by relying on unsigned wraparound if the algorithm requires a bounded mathematical result. State and test the intended arithmetic domain.

## ThreadSanitizer

TSan instruments memory accesses and synchronization. Besides data races (see
[case study 11](../failure-case-studies/11-spin-wait-data-race.md)), it builds a graph of
lock acquisitions and reports cycles. Here two threads take the same two mutexes in
opposite orders. They run one after the other, so the program never deadlocks:

```text
$ gcc -O1 -g -fsanitize=thread lock_order.c -o lock_order
$ setarch x86_64 -R ./lock_order
==================
WARNING: ThreadSanitizer: lock-order-inversion (potential deadlock) (pid=20541)
  Cycle in lock order graph: M0 (0x5555555580a0) => M1 (0x555555558060) => M0

  Mutex M1 acquired here while holding mutex M0 in thread T1:
    #0 pthread_mutex_lock ...
    #1 reload_config ./lock_order.c:14 (lock_order+0x135f) ...

  Mutex M0 acquired here while holding mutex M1 in thread T2:
    #0 pthread_mutex_lock ...
    #1 report_stats ./lock_order.c:25 (lock_order+0x12de) ...
  ...
SUMMARY: ThreadSanitizer: lock-order-inversion (potential deadlock) ./lock_order.c:14 in reload_config
==================
ThreadSanitizer: reported 1 warnings
version 1, reads 1
[exit status 66]
```

TSan reported a deadlock that *can* happen under a different schedule, from a run where
it did not happen. This is the main reason to run TSan even on tests that pass. The
exit status is 66, TSan's default `exitcode`.

`setarch -R` disables address-space randomization for the process. On kernels with high
mmap randomization, GCC 13's TSan can otherwise abort with `FATAL: ThreadSanitizer:
unexpected memory mapping`.

A race is a correctness defect even when the observed values “look right.” TSan may not model custom atomics, inline assembly, MMIO, RTOS primitives, or target synchronization correctly. Add annotations or interceptors only after verifying the primitive’s semantics, and run a target-specific concurrency review as well.

## Combining Sanitizers

ASan, UBSan, and LSan combine in one build. TSan needs its own:

```text
$ gcc -fsanitize=address,thread lock_order.c -o lock_order_both
cc1: error: ‘-fsanitize=thread’ is incompatible with ‘-fsanitize=address’
[exit status 1]
```

A practical CI layout is one `-fsanitize=address,undefined -fno-sanitize-recover=all`
job, one `-fsanitize=thread` job for threaded tests, and optimized builds at `-O1` with
`-g`, so reports have usable stacks without hiding optimizer-dependent bugs.

## Uninitialized Data

MSan detects data used before initialization, but all relevant code, including
libraries, must be instrumented or modeled. It is available in Clang, not GCC, and can be
expensive to adopt in a large system. Valgrind's Memcheck finds many of the same defects
without recompiling, at a much larger slowdown, and its timing and threading behavior
differ from native execution. [Case study 10](../failure-case-studies/10-struct-padding-leak.md)
shows a leak of uninitialized padding that ASan cannot see.

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

Suppressions, like the LSan file above, should be narrow and justified. A low-level function may require an exclusion because it intentionally inspects a stack or implements an allocator, but the exclusion should identify:

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

1. Run `examples/c/tool-sessions/sanitizers/run.sh`, then change `frame_copy.c` into a
   heap use-after-free and a stack out-of-bounds read, and compare the three reports.
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
- running UBSan in CI without `-fno-sanitize-recover` or `halt_on_error=1`, so reports
  never fail the job;
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

- [GCC instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
- [Sanitizer runtime flags](https://github.com/google/sanitizers/wiki/SanitizerCommonFlags)
- [Clang AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [Clang ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html)
- [Clang MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html)
- [Clang LeakSanitizer](https://clang.llvm.org/docs/LeakSanitizer.html)
- [Valgrind documentation](https://valgrind.org/docs/manual/manual.html)
