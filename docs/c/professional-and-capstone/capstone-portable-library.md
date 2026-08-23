---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Capstone: Portable C Library

Build a small C library that can be compiled as a freestanding-friendly core, a hosted
library, and a testable component on at least two compilers. The project is deliberately
about boundaries: public/private headers, opaque state, explicit ownership, error
contracts, configuration, portability, and evidence. A polished API with weak tests is
not a successful capstone.

## Project Brief

Implement a bounded binary buffer/ring library suitable for telemetry, packet staging,
or a device-independent transport layer. The core shall operate on caller-provided
storage and shall not perform I/O. A separate adapter may connect it to a file, socket,
UART, or RTOS queue.

The core should support:

- initialization over caller-provided aligned storage;
- append/write and read/consume operations;
- peek without consumption;
- reset and explicit ownership transfer;
- capacity/full/empty queries;
- optional fixed-size records or byte-stream mode;
- error/status reporting without global mutable state;
- compile-time configuration for C dialect and assertions;
- host tests and a target build with no hidden heap dependency.

You may choose another library domain, but it must have a real resource/lifetime
contract and at least one platform-independent core plus one platform adapter.

## Requirements And Non-Goals

Write a one-page specification before coding. Include:

- maximum capacity and alignment;
- whether writes are atomic or require external synchronization;
- full/empty policy and partial operation behavior;
- whether input/output buffers are borrowed or copied;
- error/status values and retry policy;
- integer overflow and size limits;
- reset and reuse semantics;
- supported C standards, compilers, targets, and endianness;
- timing and memory budgets.

Non-goals should be explicit. For example, the core may not provide thread safety,
dynamic growth, persistence, encryption, or transport retries. An explicit non-goal is
safer than an accidental behavior that callers begin to depend on.

## Public API Design

Prefer a small C ABI with opaque state and caller-owned buffers:

```c
#ifndef PORTABLE_BUFFER_H
#define PORTABLE_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct portable_buffer portable_buffer_t;

enum portable_buffer_status {
    PORTABLE_BUFFER_OK = 0,
    PORTABLE_BUFFER_INVALID = 1,
    PORTABLE_BUFFER_EMPTY = 2,
    PORTABLE_BUFFER_FULL = 3,
    PORTABLE_BUFFER_TOO_LARGE = 4,
    PORTABLE_BUFFER_BUSY = 5
};

size_t portable_buffer_state_size(void);
enum portable_buffer_status portable_buffer_init(
    portable_buffer_t *buffer, void *storage, size_t storage_size);
enum portable_buffer_status portable_buffer_write(
    portable_buffer_t *buffer, const void *data, size_t length,
    size_t *written);
enum portable_buffer_status portable_buffer_read(
    portable_buffer_t *buffer, void *data, size_t capacity, size_t *read);
enum portable_buffer_status portable_buffer_peek(
    const portable_buffer_t *buffer, void *data, size_t capacity, size_t *read);
size_t portable_buffer_size(const portable_buffer_t *buffer);
size_t portable_buffer_capacity(const portable_buffer_t *buffer);
void portable_buffer_reset(portable_buffer_t *buffer);

#endif
```

If the type must remain completely opaque, expose a state-size/alignment query and
require the caller to supply storage, or provide create/destroy functions with an
allocator contract. Document whether a successful partial write is possible and
whether output arguments are initialized on every error path. Use a status enum rather
than making callers infer “full” from a zero count.

## Ownership And Lifetime Contract

Document each pointer:

| Parameter/result | Contract |
| --- | --- |
| `storage` | Caller owns, keeps aligned and alive until reset/destroy |
| `data` on write | Borrowed for the duration of the call; bytes are copied or explicitly viewed |
| `data` on read/peek | Caller-owned writable storage for the call |
| `buffer` | Initialized object, not concurrently modified unless an adapter synchronizes it |
| returned state size | Configuration/ABI query, not an allocation request by itself |

Never store a caller buffer pointer unless the API says so. If a zero-copy extension is
added, define view lifetime and a release/consume transition separately from the copy-
based API. Make reset invalidate all outstanding views and state that in the contract.

## Internal Representation Choices

Compare at least two implementations:

- a linear buffer with compaction;
- a ring with head/tail indexes;
- a fixed-record pool if records are the true unit.

Record invariants such as `0 <= used <= capacity`, index range, and ownership of each
slot. Analyze unsigned counter wraparound and whether capacity must be a power of two.
Use `size_t` for object sizes, check every addition/multiplication, and keep index
arithmetic independent from untrusted lengths.

If the core is single-threaded, do not silently add atomics that imply thread safety.
If an RTOS adapter needs concurrent producers, put the lock/queue policy in the adapter
or provide an explicitly named synchronized wrapper.

## Portability Matrix

Build the library with a matrix such as:

| Dimension | Minimum coverage |
| --- | --- |
| C dialect | C11 and C17; optionally a C99 compatibility build |
| Compiler | GCC and Clang; one vendor compiler if available |
| Target | hosted x86-64 plus one embedded target |
| Optimization | `-O0`, `-O2`, size-oriented, and LTO where supported |
| Diagnostics | warnings, sanitizers, static analyzer, coverage |
| Configuration | assertions on/off, debug/release, different capacity/alignment |
| ABI | 32-bit/64-bit or two data models where relevant |

Do not call a host build a portability test if it uses host-only assumptions. Keep
platform headers and adapters outside the core, and make feature detection visible in
the build report.

## Test Plan

### Unit and boundary tests

- null/invalid state, zero capacity, zero-length operations;
- one-byte and maximum-size buffers;
- empty read/peek, full write, partial read/write;
- wrap at every possible head/tail position;
- reset after partial use;
- overlapping source/destination where the contract forbids or permits it;
- `SIZE_MAX`-adjacent requested sizes and arithmetic overflow;
- repeated fill/drain cycles.

### Model and property tests

Maintain a simple reference queue using a host container or test array. Feed the same
operation sequence to the library and model, then compare bytes, counts, and status.
Generate random operation sequences with constrained capacity and include reset,
failed operations, and partial I/O.

### Analysis and target tests

- ASan/UBSan/MSan where supported;
- static analysis and warnings at production flags;
- fuzzed operation sequences and malformed configuration;
- target stack/heap/flash map and alignment assertions;
- cycle and worst-case operation measurements;
- integration with a fake transport and one real adapter;
- fault injection for storage corruption, timeout, reset, and allocation policy.

## Documentation Deliverables

Ship:

- README with supported platforms, build, test, and integration commands;
- public API reference with ownership, thread, blocking, and error contracts;
- design note with representation and invariant choices;
- portability matrix and known deviations;
- examples for normal, full, empty, and failure paths;
- changelog and semantic-versioning policy;
- test report and coverage interpretation;
- performance/size report and target map evidence.

## ABI And Packaging

If the library is distributed as a binary, define symbol visibility, calling convention,
compiler runtime, allocator ownership, structure layout, and versioning. Prefer opaque
handles and functions over exposing private structs. If it is distributed as source,
still define a stable public header and prevent internal headers from becoming accidental
API.

Package headers, static/shared libraries, debug symbols, license metadata, generated
configuration, and a machine-readable build manifest. Test installation in a clean
consumer project rather than only inside the source tree.

## Milestones

1. Specification, API sketch, invariants, and portability matrix.
2. Minimal core with deterministic unit tests.
3. Model/property tests and malformed/error coverage.
4. Sanitizer/static-analysis/coverage integration.
5. Second compiler/target and adapter integration.
6. Performance, size, and failure-injection report.
7. API/release review, packaging, documentation, and reproducible build.

At each milestone, keep the artifact usable. Avoid building a large implementation
before the ownership and error contracts are reviewed.

## Assessment Rubric

- **Correctness:** invariants hold under normal and boundary sequences.
- **Safety:** no UB, bounds errors, stale views, unchecked arithmetic, or hidden races.
- **API quality:** ownership, status, lifetime, thread/context, and version rules are
  obvious from the interface.
- **Portability:** supported configurations build and tests detect target assumptions.
- **Evidence:** failures are tested, reports are reproducible, and artifacts are saved.
- **Maintainability:** modules are cohesive, changes are reviewable, and documentation
  explains decisions.
- **Engineering judgment:** trade-offs are measured and non-goals are respected.

## Common Mistakes

- Designing the implementation before defining full/empty, ownership, and reset rules.
- Calling an opaque API portable while exposing allocator or ABI assumptions.
- Testing only normal sequential use and not wraparound, failure, or invalid inputs.
- Adding thread safety implicitly instead of specifying the concurrency model.
- Using host containers as the only reference without matching target integer/size rules.
- Treating coverage percentage as proof of invariant or error-path correctness.
- Shipping a library without a consumer build, version policy, or diagnostic story.

## Related Topics

- [Professional Practice And Capstones overview](./index.md)
- [C Interoperability](../advanced-c/c-interoperability.md)
- [Advanced Data Structures](../advanced-c/advanced-data-structures.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Testing Strategy](../correctness-quality-and-security/testing-strategy.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)

## References

- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [CMake build system documentation](https://cmake.org/cmake/help/latest/)
- [Clang sanitizer documentation](https://clang.llvm.org/docs/index.html#sanitizers)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [Semantic Versioning](https://semver.org/)
- The project's compiler/ABI support matrix, packaging policy, API review checklist,
  and release artifact requirements
