---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 11. Spin-Wait Data Race

One thread waits in a loop for a plain `bool` that another thread sets. At `-O2` the
waiting thread reads the flag once and then loops forever. Making the flag `volatile`
makes the loop end, but the program still has a data race.

## Symptom

The release build hangs at startup or at a hand-over point, with one core at 100%. The
debug build works. A colleague suggests `volatile`, the hang disappears, and the remaining
race on the payload ships, to show up later as a stale or torn value on a weakly
ordered multicore target.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/11-spin-wait-data-race/bug.c"
```

```text
$ gcc -O0 -pthread bug.c -o bug-O0 && ./bug-O0
payload = 42
$ gcc -O2 -Wall -Wextra -pthread bug.c -o bug-O2 && ./bug-O2
[killed after 5 s: program did not terminate]
```

## What The Compiler Did

The relevant part of `main` at `-O2`:

```text
	call	pthread_create@PLT
	test	eax, eax
	jne	.L9
	cmp	BYTE PTR ready[rip], 0      # read the flag once
	jne	.L6
.L7:
	jmp	.L7                         # flag was false: loop forever
.L6:
	mov	edx, DWORD PTR payload[rip]
```

`ready` is a non-atomic object, and nothing in the loop body writes it or
synchronizes with another thread. The compiler may therefore assume its value cannot
change during the loop, so it reads the flag once before the loop instead of on every
iteration. If the flag was false, the loop never reads it again.

## The Rule

Two accesses to the same memory location conflict if at least one is a write. A
program with conflicting accesses in different threads, at least one non-atomic and
neither happening before the other, contains a data race, and its behavior is
undefined (C11 5.1.2.4p4, p25). This is what lets the compiler treat ordinary
variables as if no other thread modifies them, which is essential for keeping values
in registers.

`volatile` tells the compiler that each access is observable and must be performed. It
does not make the access atomic, and it does not order other memory operations. In C,
a `volatile` data race is still a data race.

## Why Review And Testing Missed It

- The loop looks correct to anyone thinking of "memory" rather than of the C
  abstract machine.
- Debug builds re-read the variable on every iteration.
- After `volatile` is added, x86's strong memory ordering usually makes the
  payload visible in time, so tests pass on the development host.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` | silent |
| `-fanalyzer` | silent |
| ThreadSanitizer, `-O0` build | two reports: the write/read of `ready`, and the write/read of `payload` |
| ThreadSanitizer, `-O1` build | **hung without printing a report** in our run: the optimized loop no longer re-reads the flag |
| ThreadSanitizer, `volatile` variant | the same two races: `volatile` changed nothing for TSan |

```text
WARNING: ThreadSanitizer: data race
  Write of size 1 at 0x555555558014 by thread T1:
  Previous read of size 1 at 0x555555558014 by main thread:
SUMMARY: ThreadSanitizer: data race .../bug.c:19 in producer
WARNING: ThreadSanitizer: data race
  Read of size 4 at 0x555555558018 by main thread:
  Previous write of size 4 at 0x555555558018 by thread T1:
SUMMARY: ThreadSanitizer: data race .../bug.c:33 in main
```

On kernels with high address-space randomization, TSan in GCC 13 aborts with
`unexpected memory mapping`. The run script disables ASLR for the process with
`setarch -R`.

## Fix

Make the flag atomic, and use it to *publish* the payload with release/acquire:

```c
--8<-- "examples/c/failure-case-studies/11-spin-wait-data-race/fixed.c:11:34"
```

The release store guarantees that `payload = 42` becomes visible before
`ready == true` does. The acquire load guarantees that the read of `payload` happens
after the flag was observed. `memory_order_relaxed` on the flag would end the hang, but
the payload would still not be published. TSan reports no race for the fixed program.

A real program should not spin at all: use a condition variable, semaphore, futex, or
RTOS event. The atomic flag is the minimal fix that shows the memory-model
requirement.

## Keep It Fixed

- Code review rule: any variable shared between threads, or between a thread and an
  ISR, is `_Atomic`, protected by a lock, or documented as owned by one context at a
  time.
- Run TSan on host builds at `-O0`/`-O1` as a regular CI job.
- On microcontrollers, an ISR-to-main flag is a different contract (single core, no
  threads). There, `volatile sig_atomic_t` or a C11 atomic plus an interrupt-safe
  ownership rule is appropriate. Write down which model a variable follows.

## Same Root Cause Elsewhere

- "Stop" flags in worker threads that never stop at `-O2`.
- Double-checked initialization with a plain pointer.
- Statistics counters incremented from several threads without atomics, which lose
  updates even when nothing hangs.
- The Linux kernel uses `READ_ONCE()`/`WRITE_ONCE()` to force each access to happen
  (preventing this hoisting) and separate barriers or acquire/release helpers for
  ordering. The two concerns are separate there as well.

## Review Question

> Which thread or interrupt writes this variable, which reads it, and what operation
> creates the happens-before edge between them?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [Atomics, Threads, And Signals](../standard-library-and-ecosystem/atomics-threads-and-signals.md)
- [Const, Volatile, And Restrict](../semantics-and-memory/qualifiers-const-volatile-restrict.md)

## References

- C11 (N1570) 5.1.2.4, multi-threaded executions and data races; 7.17, atomics
- [ThreadSanitizer documentation](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
- [Linux kernel memory barriers](https://docs.kernel.org/core-api/wrappers/memory-barriers.html)
