---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# C Interoperability

C is often the boundary language: firmware calls a boot service, a C++ application
uses a C driver, Rust owns a safe wrapper around a C library, Python loads a native
extension, or a plugin is loaded from a shared object built by another team. At these
boundaries, “the function has the right name” is nowhere near enough. The participants
must agree on ABI, data layout, calling convention, ownership, errors, threading,
initialization, versioning, and who may unload or destroy an object.

## Learning Objectives

- Design a C ABI that remains usable across compilers, languages, targets, and versions.
- Use `extern "C"`, opaque handles, fixed-width fields, and function tables correctly.
- Define ownership, allocation, destruction, callbacks, errors, and thread context.
- Interoperate with C++, Rust, Python/CPython, shared libraries, and plugins safely.
- Detect ABI drift with layout assertions, symbol checks, compatibility tests, and
  versioned interfaces.

## The ABI Boundary

An ABI includes more than calling-convention registers. It can include:

- symbol names, visibility, and linkage;
- argument/return register rules and stack alignment;
- scalar widths, endianness, alignment, and aggregate layout;
- enum, bit-field, boolean, `size_t`, and pointer representation;
- structure packing and padding;
- error and exception conventions;
- ownership and allocator compatibility;
- TLS, callbacks, unwind metadata, and thread context;
- object-file format, relocation, loader, and versioning rules.

Keep the boundary deliberately boring. Prefer fixed-width integers, opaque pointers,
length-delimited byte spans, explicit status codes, and caller-provided storage. Avoid
exposing C structs with pointers, bit-fields, `long`, implementation-sized enums, or
library-owned allocation unless the ABI explicitly fixes every property.

## A C-Compatible Header

`extern "C"` is a C++ language feature that requests C linkage for the declarations
inside it. The guard below keeps one public header usable by C and C++ translation
units:

```c
#ifndef SENSOR_API_H
#define SENSOR_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sensor sensor_t;

enum sensor_status {
    SENSOR_OK = 0,
    SENSOR_INVALID_ARGUMENT = 1,
    SENSOR_NOT_READY = 2,
    SENSOR_IO_ERROR = 3
};

struct sensor_sample {
    uint32_t sequence;
    int32_t value_milliunits;
    uint32_t flags;
};

sensor_t *sensor_open(uint32_t instance, enum sensor_status *status);
void sensor_close(sensor_t *sensor);
enum sensor_status sensor_read(sensor_t *sensor,
                               struct sensor_sample *sample);

#ifdef __cplusplus
}
#endif

#endif
```

The header does not promise that an enum has a particular ABI width or that a sample
has no padding. If the structure crosses a binary boundary, add `_Static_assert`
checks for the supported ABI or replace it with explicit serialization. A C++ caller
must not delete a C handle with `delete`; it must call the library's destruction
function.

## Opaque Handles

An opaque handle hides representation and keeps allocation, locking, file descriptors,
and hardware resources on the owning side. The public header declares an incomplete
type; the implementation defines it privately. This permits internal layout changes
without recompiling every caller and prevents callers from copying or fabricating the
object accidentally.

Define handle rules precisely:

- whether null is a valid value and how errors are returned;
- whether handles are thread-safe or thread-affine;
- whether operations may block or allocate;
- whether `close` is idempotent;
- whether outstanding callbacks or views keep the handle alive;
- what happens after device removal, reset, or owner shutdown.

For integer handles, use a generation/index scheme when stale-handle reuse is a risk.
For pointer handles, validate ownership at the API boundary and never expose an
implementation pointer as a stable ABI token unless its lifetime is guaranteed.

## Ownership And Allocation

Every pointer parameter needs an ownership annotation in the API documentation:

- borrowed for the duration of the call;
- borrowed until a callback or asynchronous operation completes;
- transferred to the callee;
- returned with ownership transferred to the caller;
- shared under a reference-count or synchronization contract;
- output storage supplied and owned by the caller.

Never allocate in one component and free in another unless the allocator ABI is
explicitly shared. Different CRTs, heaps, debug modes, or static library instances can
make `malloc`/`free` pairs incompatible. Provide `module_free`, caller-provided buffers,
or a shared allocator interface.

If a callback retains a pointer, define the release operation and cancellation race.
An asynchronous API should expose a completion token or reference rather than relying
on a caller stack object remaining alive by convention.

## Calling Conventions And Function Pointers

A function pointer type includes the parameter and return types but may not express all
ABI attributes. Calling-convention attributes, variadic rules, structure returns,
floating-point ABI, and interrupt/foreign callbacks can differ. Use one canonical
declaration in a shared header and compile both sides against it.

Callbacks need a context pointer so they do not rely on global state:

```c
#include <stddef.h>

typedef void (*event_callback)(void *context,
                               const unsigned char *data,
                               size_t length);

struct event_subscription {
    event_callback callback;
    void *context;
};

static void deliver_event(const struct event_subscription *subscription,
                          const unsigned char *data, size_t length)
{
    if (subscription != NULL && subscription->callback != NULL) {
        subscription->callback(subscription->context, data, length);
    }
}
```

Document the callback thread/ISR, reentrancy, maximum duration, whether it may retain
the data, and how unsubscription waits for an in-flight callback. A function pointer
that is valid in process context may be invalid in a non-secure, interrupt, or plugin
unload context.

## C And C++

C++ can call C functions with C linkage, but it does not make C++ types ABI-compatible.
Do not expose templates, classes, references, exceptions, `std::string`, STL containers,
overloaded functions, or C++ allocation across a C boundary. Catch C++ exceptions before
they cross into C; translate them to status codes or an error object.

For a C++ implementation of a C API:

- keep exported functions `extern "C"`;
- use RAII internally but provide explicit C create/destroy operations;
- translate exceptions and allocation failures at the boundary;
- ensure destruction can run with the required thread/context;
- avoid static initialization dependencies when loaded by a C/embedded runtime;
- compile with matching visibility, ABI, exceptions, RTTI, and runtime library policy.

For a C caller using a C++ library, prefer a vendor-supplied C wrapper rather than
guessing mangled names or object layout.

## C And Rust FFI

Rust's `#[repr(C)]` makes a struct's field layout compatible with the C ABI for the
supported types. Rust FFI functions use an explicit ABI such as `extern "C"`, and raw
pointers are unsafe because the compiler cannot prove validity, alignment, lifetime,
aliasing, or thread safety.

An FFI design should state:

- whether null is accepted;
- whether a pointer is input, output, or in/out;
- length and initialization rules;
- ownership transfer and which side deallocates;
- whether callbacks may run concurrently;
- panic/unwind policy;
- integer overflow and error mapping;
- ABI and target support.

Rust panics must not cross an ordinary C ABI boundary. Catch or prevent unwinding on the
Rust side and return an error. C callbacks into Rust need a lifetime strategy so the
context remains valid and no callback occurs after destruction. Conversely, Rust must
not assume that a C function honors Rust's aliasing or thread-safety expectations unless
the wrapper enforces them.

Use fixed-width fields and explicit byte buffers for FFI structures. Avoid C bit-fields,
flexible layouts with undocumented allocation, and ownership represented only by a raw
pointer. Generate or test bindings against the actual header and target ABI.

## Python And CPython Extensions

A CPython extension is a shared library loaded into the interpreter and must export the
appropriate module initialization function. The CPython C API has reference-counting,
object ownership, exception-state, thread/GIL, and interpreter-lifetime rules. A C
function returning a `PyObject *` must establish the documented new/borrowed reference
ownership and set an exception when returning failure as required by the API.

Keep native work bounded and release the interpreter lock only when the objects and
memory it touches are protected by a separate lifetime protocol. Do not hold a borrowed
reference across a point where the interpreter can run arbitrary code unless the API
contract keeps it alive. Convert C errors into Python exceptions at the outer boundary.

The CPython Stable ABI/Limited API can reduce version coupling, but it restricts the
available API and does not make arbitrary native pointers or third-party dependencies
stable. An extension should declare its supported interpreter, platform, architecture,
threading mode, and binary packaging policy.

If the requirement is only to call a C library, `ctypes` or a dedicated FFI generator
may be more portable than a custom CPython extension. Choose an extension when native
types, performance, callbacks, or interpreter integration justify its maintenance cost.

## Plugin And Shared-Library Interfaces

A stable plugin interface should avoid relying on the host's private structure layout.
Common patterns are:

- exported `plugin_get_api` function returning a versioned function table;
- a table beginning with `size`, `version`, and capability fields;
- opaque plugin context passed to every operation;
- explicit `plugin_shutdown` before unloading;
- host-owned or plugin-owned allocator functions;
- error/status functions with stable representations;
- optional functions enabled by table size/capability checks.

```c
#include <stddef.h>
#include <stdint.h>

struct plugin_api {
    uint32_t size;
    uint32_t version;
    void *context;
    int (*start)(void *context);
    int (*process)(void *context, const void *input, size_t input_length);
    void (*stop)(void *context);
};

typedef int (*plugin_get_api_fn)(uint32_t host_version,
                                 struct plugin_api *api);
```

The host must initialize the table, validate the returned `size` and function
pointers, reject unsupported versions, and call `stop` before unloading. A plugin must
not leave worker threads, callbacks, TLS destructors, function pointers, or allocated
objects running after the shared library is unloaded.

## ABI Versioning

Version the interface when semantics or layout change, not only when a file is released.
Useful techniques include:

- versioned symbol names for incompatible entry points;
- size-prefixed structures with append-only fields;
- capability bits for optional behavior;
- explicit minimum/maximum version negotiation;
- opaque handles so internal layout can change;
- compatibility shims that translate old requests;
- symbol visibility control to keep the exported surface small.

Do not append a field and assume every old caller passes a new-sized object. Check the
declared size before reading later fields and initialize new output fields only when the
caller provided storage. Do not change the meaning of an existing field while keeping
the same version.

## Error And Cancellation Contracts

Errors must cross the boundary in a stable form. Use fixed status enums, documented
numeric ranges, or an error object whose storage/lifetime is clear. Do not pass a pointer
to `errno`, a language-specific exception, or a temporary string without a contract.

Asynchronous operations need cancellation semantics:

- can cancellation race with completion;
- does completion still invoke the callback;
- when are input/output buffers reusable;
- who owns the cancellation token;
- what happens if the worker or plugin is unloaded;
- is cancellation bounded or merely requested?

Write the state machine before implementing the callback API.

## Testing Interoperability

Use more than a happy-path integration test:

- compile the same header with every supported compiler and language mode;
- assert sizes, alignments, offsets, and calling conventions where possible;
- inspect exported symbols and visibility;
- test old/new version negotiation and truncated structures;
- test null, zero-length, maximum-length, and invalid handles;
- run under sanitizers on both sides where supported;
- exercise allocation failure, callback reentrancy, cancellation, and unload;
- use ABI comparison tools or generated bindings in CI;
- test the exact deployment loader, architecture, and runtime libraries.

An interface is not stable because a small sample works. Stability is a release policy,
compatibility test suite, and ownership discipline.

## Exercises And Diagnostics

1. Design a C header for a C++ implementation with opaque handles, status returns,
   caller-owned buffers, callbacks, and explicit destruction.
2. Expose the same library to Rust using `#[repr(C)]`; list every unsafe precondition and
   write a safe wrapper that enforces it.
3. Build a minimal CPython extension or inspect one and document reference ownership,
   exception conversion, interpreter-thread, and native-buffer lifetime rules.
4. Create a versioned plugin table, then test smaller tables, unknown capabilities,
   missing callbacks, unload, and worker-thread shutdown.
5. Compare two compilers' ABI reports and symbol tables for the same public header;
   explain any layout, visibility, or calling-convention difference.

## Common Mistakes

- Assuming C linkage makes C++ object layout, exceptions, or allocation compatible.
- Exposing `long`, bit-fields, native enums, padding, or pointers in an undocumented ABI.
- Allocating on one side of a boundary and freeing with a different runtime.
- Returning borrowed memory without a lifetime or invalidation rule.
- Letting exceptions, Rust panics, or language runtimes unwind across C.
- Calling callbacks after unsubscribe, destruction, or shared-library unload.
- Treating a version number as sufficient without size, capability, and semantic rules.
- Testing only one compiler, architecture, optimization mode, or loader.

## Related Topics

- [Advanced C overview](./index.md)
- [ABI, Calling Conventions, And FFI](../compilation-linking-and-abi/abi-calling-conventions-and-ffi.md)
- [C++](../../cpp/index.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Ownership And Resource Lifetimes](../modular-design-and-apis/ownership-and-resource-lifetimes.md)
- [Compiler And Vendor Extensions](../platform-specific-c/compiler-and-vendor-extensions.md)

## References

- [Arm ABI specifications](https://github.com/ARM-software/abi-aa)
- [System V AMD64 ABI](https://gitlab.com/x86-psABIs/x86-64-ABI)
- [Rustonomicon: FFI](https://doc.rust-lang.org/nomicon/ffi.html)
- [Rust reference: type layout](https://doc.rust-lang.org/reference/type-layout.html)
- [Python extending and embedding documentation](https://docs.python.org/3/extending/index.html)
- [Python C API and ABI stability](https://docs.python.org/3/c-api/stable.html)
- [LLVM IR ABI and calling convention reference](https://llvm.org/docs/LangRef.html#calling-conventions)
- The exact target ABI, loader, compiler runtimes, language versions, packaging system,
  and compatibility policy
