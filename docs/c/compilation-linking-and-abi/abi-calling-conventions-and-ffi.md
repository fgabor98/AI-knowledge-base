---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# ABI, Calling Conventions, And FFI

An application binary interface (ABI) allows separately compiled code to cooperate. It covers more than function names: data sizes and alignment, argument and return placement, register preservation, stack rules, object-file conventions, thread-local storage, exception/unwind metadata, and sometimes C library and operating-system interfaces.

## Learning Objectives

- identify the ABI assumptions behind a C call and data exchange;
- understand registers, stack frames, alignment, return values, and variadic calls;
- define stable C interfaces for bootloaders, plugins, shared libraries, and foreign languages;
- interoperate safely with C++, Rust, assembly, and vendor libraries;
- distinguish source compatibility, link compatibility, and behavioral compatibility;
- test ABI boundaries across compilers, architectures, and release versions.

## What The ABI Must Answer

For a public binary boundary, record:

- pointer size and data model (`int`, `long`, pointer, and `size_t` widths);
- byte order and representation of integers and floating-point values;
- alignment and padding of structures and unions;
- enum representation and bit-field allocation rules;
- argument and return registers or stack slots;
- caller-saved and callee-saved registers;
- stack growth, alignment, and frame rules;
- variadic argument representation;
- symbol naming, visibility, relocation, and object format;
- TLS, unwind, exception, and sanitizer expectations where relevant;
- ownership, allocation domain, error, and version policy.

An ABI is a platform contract. ISO C deliberately leaves many representation details to the implementation, so portable source code is not automatically binary-compatible across targets or compilers.

## Calls And Register Preservation

A procedure call standard usually defines a division such as:

1. the caller evaluates arguments and places some in registers and the rest in stack slots;
2. the caller preserves registers designated caller-saved if it needs them after the call;
3. the callee preserves callee-saved registers and establishes a valid stack frame;
4. the callee returns a scalar, aggregate address, or status according to the ABI;
5. both sides maintain stack alignment at call boundaries.

The names and register sets vary by architecture. On one target, a small integer may be returned in a general register; a large struct may be returned through hidden storage supplied by the caller; a floating-point argument may use a different register bank; and a variadic call may require a register-save area.

Inspect a simple call rather than guessing:

~~~sh
cc -std=c17 -O1 -S call.c -o call.s
objdump -dr call.o
readelf -hW call.o
~~~

Never write assembly or FFI declarations based only on one disassembly. Verify the target ABI document and test multiple argument shapes, optimization levels, and return paths.

## Data Layout And Padding

This public structure is fragile as a cross-language or persistent format:

~~~c
struct device_status {
    unsigned char ready;
    unsigned long sequence;
    void *context;
};
~~~

`unsigned long`, pointer size, alignment, and padding can differ across data models. A safer wire or FFI record uses fixed-width fields and explicit reserved bytes:

~~~c
#include <stdint.h>

struct device_status_wire {
    uint8_t ready;
    uint8_t reserved[3];
    uint32_t sequence;
};

_Static_assert(sizeof(struct device_status_wire) == 8u,
               "wire layout changed");
~~~

The assertion checks one build, not all targets. Add offset assertions, endian conversion, serialization tests, and a version field. Do not transmit a native C struct merely because it has no pointers in the current build.

Packed structures remove padding but can create unaligned accesses and inefficient or faulting instructions. Use explicit encode/decode functions for external formats unless a target-specific packed representation is measured and supported.

## Calling C From C++

C++ changes linkage and type systems. A C header intended for both languages typically wraps declarations:

~~~c
#ifndef SENSOR_API_H
#define SENSOR_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sensor_handle sensor_handle_t;
int sensor_open(sensor_handle_t **out);
void sensor_close(sensor_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif
~~~

`extern "C"` suppresses C++ name mangling for those declarations, but it does not make C++ object layout, exceptions, allocation, or ownership automatically compatible. Keep the boundary in C types and define who allocates, frees, blocks, and reports errors. Do not allow C++ exceptions to cross a C boundary unless the platform ABI and policy explicitly support it.

## Calling C From Rust And Other Languages

Foreign-function interfaces should use a deliberately small C ABI surface:

- `extern` functions with C-compatible linkage;
- fixed-width integers and explicit size parameters;
- opaque pointers or handles rather than language-specific objects;
- caller-provided buffers for bounded output;
- explicit nullability and ownership rules;
- status codes or tagged result conventions;
- no C variadic functions unless the foreign language supports the exact ABI;
- no compiler-specific bit-fields or native layout across the boundary;
- explicit thread and callback lifetime rules.

Rust commonly uses `#[repr(C)]` for compatible record layout and `extern "C"` for calls. Other languages have equivalent controls. The foreign side must still match the target’s C ABI and library allocation domain. A layout attribute does not define string encoding, ownership, or synchronization.

## Callbacks And Function Pointers

Callback ABI includes the function signature, calling convention, context lifetime, and execution context:

~~~c
#include <stdint.h>

typedef void (*event_callback_t)(void *context, uint32_t event);

int event_subscribe(event_callback_t callback, void *context);
~~~

Document:

- whether `callback` may be null;
- which thread or interrupt invokes it;
- whether it may block, allocate, or call back into the library;
- whether registration copies or borrows `context`;
- how unsubscription synchronizes with an in-flight callback;
- whether the function pointer must use a special interrupt or secure-call convention.

An ABI-compatible callback with an unsafe lifetime is still a broken interface.

## Variadic Functions

Variadic calls depend heavily on ABI rules and default argument promotions. The callee knows only the fixed parameters and must use a correct format contract or equivalent type information. Cross-language variadic calls are usually a poor choice; use a fixed-argument function or a serialized record.

For C APIs, annotate format functions when the compiler supports it:

~~~c
#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_LIKE(a, b) __attribute__((format(printf, a, b)))
#else
#define PRINTF_LIKE(a, b)
#endif

int diag_printf(const char *format, ...) PRINTF_LIKE(1, 2);
~~~

The attribute improves diagnostics but does not make a variadic call safe in an ISR, across a foreign language, or with an incompatible format implementation.

## Allocator And Error Boundaries

Never assume memory allocated by one binary component can be released by another. Different CRTs, heaps, DLLs, RTOS heaps, or security domains may be distinct. Prefer paired operations:

~~~c
#include <stddef.h>

typedef struct packet packet_t;

int packet_create(packet_t **out);
void packet_destroy(packet_t *packet);
const unsigned char *packet_data(const packet_t *packet, size_t *length);
~~~

Similarly, define whether an error is returned directly, stored in `errno`, delivered through an out parameter, or reported asynchronously. An ABI document should state whether output values are valid on failure and who owns them after a partial operation.

## Versioning A Binary Interface

Use explicit compatibility mechanisms:

- opaque handles so private layout can change;
- `size` fields on extensible structures;
- version or feature masks;
- reserved fields initialized to zero;
- symbol versioning or distinct entry-point names where supported;
- capability negotiation;
- ABI fixtures compiled by multiple toolchains;
- a compatibility policy for old callers and new libraries.

Example size-tagged configuration:

~~~c
#include <stdint.h>

struct driver_config {
    uint32_t size;
    uint32_t version;
    uint32_t flags;
    uint32_t reserved;
};

int driver_configure(const struct driver_config *config);
~~~

The callee must validate `size`, `version`, flags, reserved fields, alignment, and pointer lifetime before reading optional fields.

## ABI Checks And Tools

Useful checks include:

~~~sh
readelf -hW libdevice.so
readelf -Ws libdevice.so
nm -D --defined-only libdevice.so
readelf -A firmware.elf          # target attributes where supported
abi-dumper old.so -o old.abi     # external ABI tooling, if adopted
~~~

Also compile a small ABI fixture that prints `sizeof`, `_Alignof`, and `offsetof` for public records; inspect assembly for representative calls; and run cross-language tests that allocate, call, callback, fail, and destroy objects.

## Exercises

1. Disassemble calls with scalar, floating-point, aggregate, and variadic arguments.
2. Build a public structure on 32-bit and 64-bit targets and document every offset change.
3. Create a C/C++ boundary with an opaque handle and test allocation and error paths.
4. Expose a fixed C API to Rust or another FFI-capable language using explicit ownership.
5. Add a callback and test unsubscription while a callback is in flight.
6. Break a library’s public layout intentionally and detect it with ABI fixtures.
7. Design a versioned configuration structure that can be extended without changing old callers.

## Common Mistakes

- treating matching prototypes as proof of ABI compatibility;
- exposing `long`, `wchar_t`, pointers, bit-fields, or native structs in a wire format;
- crossing C/C++ without `extern "C"`;
- allowing exceptions, panics, or language-specific objects across a C boundary;
- passing variadic calls through an FFI;
- freeing memory in a different allocator domain;
- ignoring callback context and unsubscription lifetime;
- using packed structures to solve every layout problem;
- changing alignment or compiler flags for one module only;
- publishing private symbols or compiler-generated types as stable ABI.

## Related Topics

- [Object Files, Symbols, And Relocations](./object-files-symbols-and-relocations.md)
- [Static And Dynamic Linking](./static-and-dynamic-linking.md)
- [Cross-Compilation And Sysroots](./cross-compilation-and-sysroots.md)
- [API And Opaque Types](../modular-design-and-apis/api-and-opaque-types.md)
- [C Interoperability](../advanced-c/c-interoperability.md)
- [Platform-Specific C](../platform-specific-c/index.md)

## References

- [Arm Application Binary Interface repository](https://github.com/ARM-software/abi-aa)
- [AAPCS64 Procedure Call Standard](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst)
- [System V ABI and ELF specification](https://refspecs.linuxfoundation.org/elf/)
- [GCC type attributes](https://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html)
- [GCC function attributes](https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html)
