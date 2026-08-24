---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Coding Practices

Good C style is not decoration. It reduces the number of states a reviewer must hold in mind, exposes the ownership and lifetime model, makes compiler diagnostics useful, and gives tests and analyzers a stable structure to reason about. The best practice is the one that makes a failure or violated assumption visible.

## Learning Objectives

- structure C modules around clear responsibilities and narrow interfaces;
- make ownership, bounds, state, error, and context assumptions visible;
- choose names and comments that communicate decisions rather than syntax;
- review code for correctness, security, portability, and maintainability;
- avoid hidden global state, ambiguous cleanup, and misleading abstractions;
- make embedded code testable without pretending hardware is ordinary memory.

## Start With A Contract

Before implementing a function, write down:

- valid input ranges and nullability;
- output validity on success and failure;
- ownership transfer and release operation;
- storage duration and aliasing assumptions;
- blocking, allocation, interrupt, and thread restrictions;
- timing, stack, heap, and power expectations;
- concurrency and reentrancy rules;
- hardware preconditions and reset behavior;
- error classification and recovery policy.

Example contract:

~~~c
#include <stddef.h>

/*
 * Decode one complete frame.
 *
 * Preconditions: frame != NULL, output != NULL, frame_length <= 256.
 * Context: task or host test; not ISR; does not block or allocate.
 * Success: output is initialized and returns 0.
 * Failure: output is unchanged and returns a negative status.
 */
int frame_decode(const unsigned char *frame,
                 size_t frame_length,
                 struct frame *output);
~~~

The implementation should make the contract mechanically visible through checks, types, assertions, tests, and analysis annotations. A comment that is not enforced or tested is a useful hypothesis, not evidence.

## Naming And Representation

Names should expose the unit and ownership where ambiguity is costly:

- `timeout_ms`, `sample_count`, and `capacity_bytes` carry units;
- `owned_buffer`, `borrowed_view`, or `out_handle` communicate lifetime;
- `is_ready` and `has_crc` communicate Boolean intent;
- `irq_state` and `mmio_status` communicate hardware context;
- `*_init`, `*_deinit`, `*_acquire`, and `*_release` communicate lifecycle.

Prefer types that make invalid combinations harder:

~~~c
#include <stddef.h>

struct byte_span {
    const unsigned char *data;
    size_t length;
};

struct byte_buffer {
    unsigned char *data;
    size_t length;
    size_t capacity;
};
~~~

Do not call both a borrowed view and an owning buffer `buffer`. Do not encode units in comments while passing raw integers through unrelated APIs.

## Module Cohesion

A C module should own a coherent policy and hide implementation details behind a header. Keep these separate when their change reasons differ:

- protocol parsing versus transport I/O;
- device register access versus application policy;
- allocation strategy versus data structure operations;
- persistent format versus in-memory representation;
- interrupt capture versus task-level processing.

Use file-scope `static` for private data and functions. Keep headers small, self-contained, and free of accidental transitive dependencies. A module with a narrow interface is easier to test with a fake dependency and harder to misuse from an ISR or unrelated subsystem.

## Control Flow And Error Paths

Make the success path and cleanup path obvious. For multi-resource functions, one cleanup point can be clearer than repeated partial cleanup:

~~~c
#include <stddef.h>

struct device;
struct device *device_alloc(void);
int device_hw_init(struct device *device);
void device_free(struct device *device);

int device_open(struct device **out)
{
    struct device *device = NULL;
    int status = 0;

    if (out == NULL) {
        return -1;
    }
    *out = NULL;

    device = device_alloc();
    if (device == NULL) {
        status = -2;
        goto cleanup;
    }
    status = device_hw_init(device);
    if (status != 0) {
        goto cleanup;
    }

    *out = device;
    device = NULL;

cleanup:
    device_free(device);
    return status;
}
~~~

`goto` is not inherently poor C. A forward-only cleanup path can make ownership transfer and failure handling more auditable. Avoid jumping into scopes, creating hidden control flow, or using one status value for unrelated error classes.

## Assertions And Runtime Checks

Use assertions for programmer or integration assumptions that should never be false in a valid build; use ordinary error handling for expected environmental failures:

~~~c
#include <assert.h>
#include <stddef.h>

struct ring {
    unsigned char *data;
    size_t head;
    size_t count;
    size_t capacity;
};

int ring_push(struct ring *ring, unsigned char value)
{
    assert(ring != NULL);
    if (ring->count == ring->capacity) {
        return 0; /* Normal full condition. */
    }
    ring->data[ring->head] = value;
    ring->head = (ring->head + 1u) % ring->capacity;
    ++ring->count;
    return 1;
}
~~~

Do not put required safety behavior only in `assert`; release builds may disable it. In safety or embedded products, define whether assertions halt, reset, enter a safe state, record evidence, or notify a supervisor.

## Comments That Preserve Decisions

Useful comments explain:

- why an apparently unusual ordering is required by hardware;
- which ABI, erratum, timing, or protocol rule constrains the code;
- why a bound is conservative;
- what a lock protects and which contexts may call the function;
- why a sanitizer or analyzer suppression is justified;
- what compatibility behavior must remain stable.

Avoid comments that restate `i++`, predict what the next line obviously does, or describe behavior that the code no longer implements. Link to a requirement, erratum, test, or issue when the reason is expected to outlive the author.

## Reviewable Changes

Keep a change easy to inspect:

1. separate mechanical formatting from behavior changes;
2. update the contract and tests with the implementation;
3. keep unrelated refactors out of a defect fix;
4. show failure behavior, not only the happy path;
5. include target, timing, memory, and concurrency impact;
6. state which tools ran and which could not run;
7. preserve generated-code and configuration changes with their source;
8. make deviations and suppressions local and justified.

Reviewers should ask “what happens if this fails halfway?” and “what context can call this?” as routinely as they ask whether the algorithm is correct.

## Embedded-Specific Practices

- Separate register definitions, access wrappers, and device policy.
- Keep ISR work bounded: capture minimal state, clear the source, notify a task.
- Make MMIO access width and ordering explicit.
- Mark externally changing registers `volatile` only where required; use barriers for ordering.
- Put DMA buffers in a named, aligned section with cache ownership rules.
- Track stack and heap budgets per task and interrupt nesting level.
- Avoid hidden allocation, logging, locks, and locale-dependent behavior in critical paths.
- Make reset and brownout behavior part of the module contract.
- Provide a host seam for pure logic and a target test for the hardware boundary.

## Exercises

1. Rewrite an ambiguous buffer API using span, capacity, ownership, and status conventions.
2. Split a mixed UART/parser module into cohesive interfaces and test doubles.
3. Implement a multi-resource initialization function with an auditable cleanup path.
4. Review a driver for ISR safety, MMIO width, cache, and error-path issues.
5. Add contracts and tests for every public function in a small module.
6. Turn one implicit assumption into a type, assertion, linker check, or test.
7. Review a real diff and classify every comment as requirement, invariant, rationale, or noise.

## Common Mistakes

- using style rules without ownership, bounds, and context rules;
- hiding allocation, locks, or blocking behind innocent-looking APIs;
- returning partially initialized output on failure;
- using assertions for recoverable hardware or input failures;
- duplicating cleanup until one path inevitably diverges;
- exposing private structs and globals through headers;
- commenting what code does while omitting why it must do it;
- mixing formatting, refactoring, and behavior changes in one review;
- assuming host-friendly abstractions have embedded-friendly timing or memory costs.

## Related Topics

- [API And Opaque Types](../modular-design-and-apis/api-and-opaque-types.md)
- [Ownership And Resource Lifetimes](../modular-design-and-apis/ownership-and-resource-lifetimes.md)
- [Testing Strategy](./testing-strategy.md)
- [Safety Standards And MISRA](./safety-standards-and-misra.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)

## References

- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [MISRA C resources](https://misra.org.uk/)
