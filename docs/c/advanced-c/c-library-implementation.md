---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# C Library Implementation

The C library is an implementation of contracts, not a magical collection of
functions. On a hosted system it sits above an operating system, loader, threads, and
locale/runtime services. On a freestanding embedded system it may be a small set of
headers and compiler support routines, with the application supplying startup, I/O,
heap, locks, and system-call retargeting.

Understanding the library lets you answer practical questions: Why did `printf` pull
30 KiB into flash? Why did `malloc` fail after a long run? Why is `errno` shared between
tasks? Why did `memcpy` fault on a peripheral region? Which functions are safe in an
ISR? Which runtime objects must be initialized before `main`?

## Learning Objectives

- Explain the startup/runtime responsibilities around `crt0`, libc initialization, and
  `main`.
- Understand allocator metadata, alignment, fragmentation, failure, and concurrency.
- Choose string/memory and stdio APIs from their contracts, not their names.
- Retarget low-level I/O without creating recursion, data races, or unbounded latency.
- Distinguish reentrancy, thread safety, async-signal safety, and ISR safety.
- Compare embedded libc implementations and configure only the features the product
  can support.

## Hosted And Freestanding Library Contracts

The C standard distinguishes hosted and freestanding implementations. A hosted
implementation provides the complete required environment, including the standard
startup contract and library facilities. A freestanding implementation needs only a
smaller required set and may omit or replace much of the hosted environment.

The compiler and library are a pair. Builtins such as `memcpy`, integer division
helpers, floating-point routines, atomics, stack protection, and `__aeabi_*`-style
runtime functions may be emitted even when source code does not name a library
function. Linker options, compiler target, ABI, and library variant must agree.

## Startup And Runtime Support

A typical C image has a path like:

```text
reset/loader -> stack and CPU state -> data copy/zero -> runtime/libc init
             -> constructors/hooks if configured -> main(argc, argv)
```

The exact sequence differs between bare metal, RTOS, and hosted systems. Responsibilities
can include:

- establishing stack and TLS pointers;
- copying initialized data and clearing zero-initialized data;
- initializing heap boundaries and allocator locks;
- setting up `stdin`, `stdout`, `stderr`, `environ`, and `errno` state;
- registering `atexit` handlers and static initialization hooks;
- initializing locale or floating-point environment;
- installing syscall vectors or semihosting stubs;
- calling `main` and deciding what happens when it returns.

Do not call libc before its required initialization. A reset handler that uses `memcpy`
may be safe only if the implementation supplies it as a compiler builtin or its
underlying runtime is already usable. A bootloader image may intentionally avoid all
libc startup and use a small freestanding subset.

## `malloc` And Allocator Design

An allocator manages a region of storage and metadata. Common strategies include:

- first-fit or best-fit free lists;
- segregated size classes;
- boundary tags and coalescing;
- buddy allocators;
- per-thread/per-core caches and arenas;
- fixed-size pools and slabs;
- monotonic regions/arenas;
- wrappers over an RTOS heap or kernel allocator.

The public contract includes returned alignment, minimum allocation size, zero-size
behavior, preservation rules for `realloc`, failure result, thread safety, and whether
memory is zeroed. The allocator also needs a policy for metadata corruption, double
free, invalid pointer, and out-of-memory recovery.

### Fragmentation And Bounds

External fragmentation prevents a large allocation even when total free memory is
large; internal fragmentation wastes space inside blocks or size classes. Measure peak
live bytes, free-block distribution, largest free block, allocation latency, and
failure rate under realistic allocation/free patterns.

For real-time work, general-purpose `malloc` may have unbounded search, coalescing, or
locking latency. Prefer fixed pools, bounded block classes, or a preallocation phase.
If dynamic allocation is required, define when it is allowed and record allocation
failures as a product event rather than silently continuing with a null pointer.

### Alignment And Overflows

An allocator must align every returned pointer for the supported types and correctly
account for header, padding, and requested size. Additions such as `header + padding +
requested` need overflow checks before they are used to select a block. A custom
allocator must not return a pointer to metadata or to a region outside its declared
storage.

### Reentrancy And Locks

An allocator called from two tasks needs synchronization. An allocator called from an
ISR may be forbidden, may deadlock, or may require a separate interrupt-safe pool.
Do not call a logging function from inside the allocator if the logger can allocate or
take the same lock. Keep diagnostic hooks allocation-free or defer them.

## `stdio` Internals And Costs

`FILE` represents a stream with buffering, orientation, error/EOF state, a position, and
possibly a lock. `printf` adds format parsing, integer conversion, floating conversion,
locale behavior, buffering, and output transport. On a small MCU this can be a major
flash and stack dependency, especially `%f`, wide-character support, and dynamic
buffering.

Understand buffering modes:

- unbuffered output can create one device operation per character;
- line-buffered output depends on newline and stream/terminal behavior;
- fully buffered output reduces calls but delays visibility and consumes memory.

The C standard does not make every `stdio` operation suitable for an ISR or signal
handler. Use a fixed-size, non-allocating event logger in interrupt context and format
messages in a task. Always check formatting return values and distinguish truncation
from transport failure in bounded logging APIs.

For embedded diagnostics, alternatives include:

- integer-only formatting with a fixed buffer;
- compile-time-disabled log levels;
- binary event records decoded on the host;
- a lock-free or interrupt-safe byte ring feeding a worker;
- semihosting only in controlled debug builds;
- a minimal `write`-like sink with explicit backpressure.

## String And Memory Primitives

The standard memory functions have precise contracts:

- `memcpy` requires non-overlapping source and destination;
- `memmove` supports overlap;
- `memcmp` compares bytes, not numerical objects or strings;
- `memset` writes repeated bytes, not repeated arbitrary integer values;
- `strlen` requires a terminating null byte within a valid object;
- `strcmp` expects valid null-terminated strings;
- `strncpy` has padding/truncation behavior that often surprises callers.

The compiler may inline or replace these calls with target instructions. That is safe
only when the source-level preconditions hold. Do not use them blindly on MMIO, volatile
objects, overlapping device windows, or uninitialized object representations. Use
device-specific accessors for registers and `memcpy`/explicit serialization for wire
bytes.

For bounded text, carry a length. If a function promises a C string, state who provides
the terminator and what happens when the destination is too small. APIs such as
`snprintf` report the size that would have been needed, but the caller still needs to
interpret negative/error and truncation results correctly.

## Syscall Retargeting

Embedded libraries often route operations such as `read`, `write`, `close`, `sbrk`,
`isatty`, or time access through weak hooks or a board-supplied system-call layer. The
names and exact contracts vary by library. A retargeting layer should define:

- descriptor namespace and ownership;
- blocking/non-blocking and timeout behavior;
- short read/write behavior;
- interrupt or task context restrictions;
- error mapping and `errno` policy;
- reentrancy and locking;
- behavior before scheduler/clock/heap initialization;
- what happens after a device reset or disconnect.

Avoid accidental recursion. For example, a `_write` hook that logs errors with
`fprintf`, which calls `_write` again, can recurse until stack exhaustion. Keep the
lowest sink primitive independent of higher-level formatting and make failure paths
allocation-free.

## `errno`, Reentrancy, And Thread Safety

These terms are different:

- **Reentrant:** a function can be safely entered again before an earlier invocation
  finishes, often because it uses no shared mutable state or protects it appropriately.
- **Thread-safe:** concurrent calls meet the implementation's contract, possibly by
  locking shared state.
- **Async-signal-safe:** a hosted/POSIX function is safe in a signal handler under that
  environment's restricted rules.
- **ISR-safe:** a target/RTOS-specific property involving interrupt context, execution
  time, and allowed operations.

`errno` is commonly per-thread in hosted systems, but an embedded libc may require an
application-provided reentrancy structure or may implement a simpler global state.
Check the library configuration. Save an error value before calling another function
that may change it, and do not expose a pointer to shared error state without a lifetime
contract.

Thread-safe `stdio` or allocator locks may block. A function can be thread-safe and still
be unsuitable for a real-time path. A reentrant function can still be too slow or
allocate. Review all four properties separately.

## Locale And Character Conversion

Locale affects classification, case conversion, collation, formatted I/O, and numeric
conversion in hosted implementations. Embedded products often select the C locale,
omit locale data, or provide only ASCII/UTF-8 policy. Make the policy explicit rather
than relying on a developer machine's environment.

Multibyte and wide-character support brings state, table storage, encoding errors, and
possibly large code/data dependencies. For protocols, use a specified encoding and
length rather than locale-sensitive functions. For user-facing text, decide whether
invalid sequences are rejected, replaced, or preserved.

## Embedded libc Trade-offs

Common choices include a full hosted libc, a reduced newlib configuration, newlib-nano,
picolibc, musl in a Linux image, vendor libraries, or a project-specific freestanding
layer. Compare:

- C standard and POSIX coverage;
- code and data footprint;
- allocator choices and thread/reentrancy support;
- `stdio` and floating-format features;
- locale and wide-character support;
- startup and syscall integration;
- licensing and maintenance;
- sanitizer, debugger, and toolchain compatibility;
- deterministic behavior and target errata.

Do not select a “nano” or reduced library solely from its name. Measure the final image,
stack, heap, timing, and error behavior under the exact configuration. A smaller libc
can move responsibilities into application code and make failures less visible.

## Auditing A Library Call

For every library call on a critical path, answer:

1. What are its preconditions and failure values?
2. Can it allocate, lock, block, access locale/TLS, or call a syscall?
3. Is it safe in the current task, ISR, signal, boot, or fault context?
4. What code and data does it pull into the image?
5. Does it touch cache/MMIO/DMA memory correctly?
6. Is its timing bounded enough for the requirement?
7. Which implementation version and configuration define the behavior?

Use map files, symbol reports, disassembly, tracing, and the library source/configuration
when the answer matters. “It is in the standard library” is not an answer to a target
timing or context question.

## Exercises And Diagnostics

1. Trace a bare-metal image from reset to `main` and list every runtime/libc function
   called before the scheduler and heap are ready.
2. Compare `printf`, integer-only formatting, and binary event logging for flash, stack,
   latency, and output bandwidth.
3. Stress a heap with a recorded allocation trace; report fragmentation, largest free
   block, peak live memory, and allocation latency.
4. Implement a retargeted output sink with short writes, timeout, reset, and recursion
   tests; verify it is not called from forbidden contexts.
5. Build with two libc configurations and explain every image, startup, TLS, allocator,
   locale, and syscall difference.

## Common Mistakes

- Assuming `main` starts before runtime, TLS, heap, or stream initialization.
- Calling `malloc`, `printf`, or a locale/string routine from an ISR without checking
  the library and RTOS contract.
- Treating `memcpy` as safe for overlap, MMIO, volatile objects, or invalid lengths.
- Ignoring allocator fragmentation, metadata corruption, alignment, and failure policy.
- Retargeting low-level I/O through a path that allocates, locks recursively, or blocks
  forever.
- Confusing thread safety with reentrancy, signal safety, or ISR safety.
- Relying on host locale or libc behavior in a protocol or freestanding image.
- Measuring only the source call and ignoring compiler builtins, runtime helpers, and
  linked library dependencies.

## Related Topics

- [Advanced C overview](./index.md)
- [Standard Library And Ecosystem](../standard-library-and-ecosystem/index.md)
- [Embedded libc Implementations](../standard-library-and-ecosystem/embedded-libc.md)
- [Startup, Runtime, And main](../compilation-linking-and-abi/startup-runtime-and-main.md)
- [Memory Layout And Allocation](../semantics-and-memory/memory-layout-and-allocation.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)

## References

- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [newlib documentation](https://sourceware.org/newlib/)
- [picolibc documentation](https://github.com/picolibc/picolibc)
- [musl libc documentation](https://musl.libc.org/)
- [GCC C library builtins](https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html)
- The exact libc source/configuration, startup objects, syscall ABI, allocator, linker
  script, RTOS port, and board I/O contract
