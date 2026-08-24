---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Portability

Portable C is not code that happens to compile on two machines. It is code whose assumptions are identified, whose required guarantees are available on every target, and whose implementation-specific boundaries are isolated and tested. Embedded portability often means moving policy across microcontrollers, compilers, RTOSes, and libc implementations while retaining target adapters for hardware.

## Learning Objectives

- distinguish ISO C, implementation, ABI, OS, RTOS, vendor, and hardware assumptions;
- identify integer-width, representation, alignment, endian, and pointer-width hazards;
- use feature detection and capability interfaces instead of compiler-name guesses;
- design portable wire formats and persistent data;
- test one codebase across substantially different targets;
- isolate unavoidable extensions behind small, documented boundaries.

## Portability Layers

| Layer | Example assumption | How to control it |
| --- | --- | --- |
| ISO C | integer conversions, object lifetime | write conforming code and test constraints |
| implementation | `char` signedness, `sizeof(int)`, floating model | query with headers/macros and assert policy |
| compiler | attributes, built-ins, warnings | feature macros and wrapper headers |
| ABI | calling convention, alignment, struct layout | ABI documentation and fixture tests |
| OS/POSIX | file descriptors, threads, `mmap` | adapter module and target tests |
| RTOS | task, queue, ISR, tick semantics | port-specific interface |
| vendor/BSP | register layout, startup, cache policy | hardware boundary and manual |
| board/product | clocks, memory map, bootloader | configuration and integration tests |

The presence of a familiar function name does not establish portability. `memcpy` is ISO C; `open` is POSIX; a CMSIS register definition is vendor/architecture-specific; and a FreeRTOS queue is an RTOS contract.

## Integer Width And Conversion

Use `<stdint.h>` types when the width is part of the protocol, register, storage, or ABI contract:

~~~c
#include <stdint.h>
#include <limits.h>

_Static_assert(CHAR_BIT == 8, "protocol requires octets");

uint32_t read_u32_le(const unsigned char bytes[4])
{
    return ((uint32_t)bytes[0])
         | ((uint32_t)bytes[1] << 8)
         | ((uint32_t)bytes[2] << 16)
         | ((uint32_t)bytes[3] << 24);
}
~~~

`uint32_t` exists only when the implementation provides an exact 32-bit unsigned type. If the product requires it, make that requirement explicit and fail configuration when unavailable. Use `size_t` for object sizes, `ptrdiff_t` for pointer differences, and `uintptr_t` only when an integer representation of a pointer is genuinely required.

Treat conversions as design decisions. Signed/unsigned mixing, narrowing, multiplication before allocation, and shifting by a width-dependent amount are common portability and security defects. Enable conversion warnings and test boundary values.

## `char`, Bytes, And Text

Plain `char` may be signed or unsigned. Use `unsigned char` or `uint8_t` for byte-oriented data, and convert character values carefully before passing them to `<ctype.h>` functions, whose argument must be representable as `unsigned char` or equal to `EOF`.

Do not assume:

- ASCII is the only character encoding;
- `sizeof(char) == 1` means an octet—`CHAR_BIT` defines the number of bits;
- a C string is valid binary data;
- `wchar_t` has the same width or encoding across targets;
- locale behavior is available or desirable in firmware.

For an embedded protocol, define octets, text encoding, normalization, termination, and maximum lengths explicitly.

## Endianness And Representation

Never serialize a native integer or struct by copying its object representation unless the format explicitly adopts that implementation representation. Encode fields by shifts or named endian helpers:

~~~c
#include <stdint.h>

void write_u16_be(unsigned char output[2], uint16_t value)
{
    output[0] = (unsigned char)(value >> 8);
    output[1] = (unsigned char)value;
}
~~~

Floating-point representation, NaN behavior, padding bits, bit-field allocation, enum size, and pointer representation are implementation concerns. Use a specified serialization format, not `memcpy` of a native object, when data crosses a device, process, compiler, or product version boundary.

## Alignment And Structure Layout

Alignment requirements differ. A packed structure may avoid padding in a file image but can produce unaligned accesses or traps:

~~~c
#include <stdint.h>
#include <stddef.h>

struct header {
    unsigned char tag;
    unsigned char length;
    uint16_t sequence;
};

_Static_assert(offsetof(struct header, tag) == 0u, "tag offset");
~~~

This example still does not define wire endianness. Use offset assertions for an intentional ABI, but prefer encode/decode functions for external formats. Document alignment required by DMA, cache lines, atomic operations, and ABI calls.

## Pointer Width And Data Models

Do not store pointers in `unsigned long`, protocol fields, or persistent records. Use `uintptr_t` for temporary address arithmetic only when supported and required; it does not make a pointer valid after reset or across processes. `size_t` can be wider or narrower than `unsigned long`, and a 64-bit pointer target may still use a 32-bit `int`.

Use opaque handles when an API should hide representation:

~~~c
struct device;
typedef struct device device_t;

int device_open(device_t **out);
void device_close(device_t *device);
~~~

The caller can use the handle without depending on private layout, so the implementation can change across targets.

## Feature Detection

Prefer capability detection over compiler-brand detection:

~~~c
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define PROJECT_HAS_C11 1
#else
#define PROJECT_HAS_C11 0
#endif

#if defined(__has_include)
#  if __has_include(<threads.h>)
#    define PROJECT_HAS_C_THREADS 1
#  endif
#endif
#ifndef PROJECT_HAS_C_THREADS
#define PROJECT_HAS_C_THREADS 0
#endif
~~~

Configuration should also probe target libraries, linker features, atomic lock-free properties, alignment, and compiler options. A feature existing in a header does not prove that its implementation is usable in a freestanding image or ISR.

## Conformance And Extensions

Extensions are often necessary for embedded work: section placement, interrupt declarations, packed types, barriers, register access, and target intrinsics. Isolate them:

- one portability header per concern;
- a standard fallback or explicit unsupported error;
- compile-time checks for target identity and ABI;
- tests for the extension boundary;
- no extension leaking into portable policy modules.

Do not use `#ifdef __GNUC__` as a complete capability test. Clang may define GNU compatibility macros, vendors may use GCC-derived compilers, and two versions of one compiler may support different attributes.

## Portability Test Matrix

Test dimensions that can expose different assumptions:

- 32-bit and 64-bit host;
- little- and big-endian emulator or target where available;
- signed and unsigned plain `char`;
- strict and GNU language modes;
- different optimization levels;
- hardware and software floating-point ABI;
- hosted and freestanding builds;
- multiple libc/RTOS implementations;
- alignment-sensitive and unaligned memory;
- cold, warm, watchdog, and power-loss reset paths.

The goal is not to support every environment. The goal is to demonstrate the declared support envelope and fail clearly outside it.

## Exercises

1. Build a type and layout report for every supported target.
2. Replace native struct serialization with explicit endian encode/decode functions.
3. Run tests with plain `char` signedness changed and find assumptions.
4. Add compile-time checks for pointer width, `CHAR_BIT`, alignment, and required C edition.
5. Isolate a compiler section attribute behind a portability header.
6. Compare a host and target implementation of one timing or I/O adapter.
7. Create a compatibility matrix and make unsupported combinations fail during configuration.

## Common Mistakes

- calling code portable because it compiles on one host and one board;
- assuming `int`, `long`, pointer, enum, or `wchar_t` widths;
- serializing structs and native floating-point values directly;
- confusing `char` text with byte storage;
- using packed structs without alignment and performance analysis;
- using compiler-brand macros instead of capability checks;
- allowing vendor headers and extensions into portable policy code;
- testing only little-endian, optimized, hosted builds;
- assuming a feature header means the runtime and startup support exist.

## Related Topics

- [Types, Values, And Objects](../language-fundamentals/types-values-and-objects.md)
- [Object Representation, Alignment, And Padding](../semantics-and-memory/object-representation-alignment-and-padding.md)
- [ABI, Calling Conventions, And FFI](../compilation-linking-and-abi/abi-calling-conventions-and-ffi.md)
- [Testing Strategy](./testing-strategy.md)
- [Platform-Specific C](../platform-specific-c/index.md)

## References

- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
- [GCC predefined macros](https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html)
