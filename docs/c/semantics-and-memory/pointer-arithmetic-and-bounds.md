---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Pointer Arithmetic And Bounds

Pointer arithmetic is defined in terms of array objects, not raw numeric addresses. A pointer to an element may move within the same array and one position past its end, but it may not be used to access storage outside that object.

This rule is easy to violate in embedded code that walks packet buffers, ring buffers, memory windows, or register arrays.

## Learning Objectives

- Perform legal pointer addition, subtraction, and indexing.
- Explain the one-past-the-end rule.
- Use size_t and ptrdiff_t for bounds and differences.
- Keep pointer and length views synchronized.
- Distinguish an invalid pointer value from a valid one-past pointer.
- Identify why raw address arithmetic is not portable C pointer arithmetic.

## Array Relationship

For an array object:

~~~c
#include <stddef.h>
#include <stdint.h>

void clear(uint8_t data[4])
{
    uint8_t *first = data;
    uint8_t *last = data + 4u;

    for (uint8_t *cursor = first; cursor != last; ++cursor) {
        *cursor = 0u;
    }
}
~~~

The pointer one past the last element is valid to form and compare, but it must not be dereferenced. The expression data[i] is defined as *(data + i), so the same bounds rule applies to indexing.

The array relationship exists only when the pointers refer into the same array object or one past it. A pointer to one object cannot be advanced into an unrelated neighboring object merely because the numeric addresses appear adjacent.

## Legal Pointer Operations

If p points to an element of an array, these operations are meaningful within the array:

- p + n and p - n when the result is within the array or one past it;
- increment and decrement when the resulting position is valid;
- p1 - p2 when both point into the same array, producing ptrdiff_t;
- comparisons for positions within the same array;
- equality comparison of compatible pointer values.

~~~c
#include <stddef.h>
#include <stdint.h>

ptrdiff_t distance_between(const uint16_t *begin,
                           const uint16_t *end)
{
    return end - begin;
}
~~~

The caller must guarantee that begin and end designate positions in one array. Pointer subtraction can overflow ptrdiff_t if the array is too large for the result type.

Do not subtract arbitrary pointers, compare unrelated pointers with ordering operators, or form a pointer far outside an object and hope not to dereference it.

## One-Past Pointers

One-past pointers make half-open ranges possible:

~~~c
#include <stddef.h>

int sum(const int *begin, const int *end)
{
    int result = 0;
    for (const int *cursor = begin; cursor != end; ++cursor) {
        result += *cursor;
    }
    return result;
}
~~~

The range is [begin, end). Empty ranges are represented by begin equal to end. A generic function should not increment or subtract a null pointer.

A one-past pointer cannot be dereferenced. It can be converted back by subtracting one only when the range is non-empty.

## Indexing And Overflow

An index is often easier to review than pointer movement:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool read_at(const uint8_t *data, size_t length,
             size_t index, uint8_t *value)
{
    if (data == NULL || value == NULL || index >= length) {
        return false;
    }

    *value = data[index];
    return true;
}
~~~

Check the index before the access. For a two-dimensional or byte-offset calculation, check multiplication and addition before forming the pointer:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool element_address(uint16_t *base, size_t count,
                     size_t index, uint16_t **result)
{
    if (base == NULL || result == NULL || index >= count) {
        return false;
    }

    *result = &base[index];
    return true;
}
~~~

The bound check must use the same array view and must not itself overflow. For a byte offset, validate the offset against byte capacity before converting it to a typed pointer.

## Pointer Differences And Types

Use size_t for a number of elements or bytes, ptrdiff_t for a signed difference between pointers into one array, and an explicit integer type only for a protocol or hardware-defined numeric address.

Do not store a pointer difference in int merely because the current target uses 32-bit pointers. The valid range of an array difference and the type’s range are separate questions.

## Pointer Comparisons

Equality comparisons answer whether pointer values designate the same location or compare equal under the language rules. Relational comparisons such as less-than are meaningful for positions in one array and are not a portable ordering for unrelated objects.

If an API needs deterministic ordering of arbitrary objects, compare explicit indexes, allocation sequence numbers, or integer tokens defined by that API. Do not sort raw pointers as portable addresses.

## Pointer Invalidation

Pointer arithmetic does not extend lifetime. These operations can invalidate pointers:

- leaving the block containing an automatic object;
- releasing allocated storage;
- reallocating a block, even when the new block appears at the same address;
- changing an owning container that moves its elements;
- resetting an arena or pool;
- unmapping or disabling a device region;
- handing a DMA buffer to hardware under an ownership protocol.

After invalidation, set local owning pointers to NULL where that improves diagnostics, but do not assume clearing one copy fixes aliases elsewhere.

## Ring Buffers And Wrapped Storage

A ring buffer is logically contiguous but physically split at the wrap point. A pointer should not be advanced across the end of the physical array as though the object continued:

~~~c
#include <stddef.h>
#include <stdint.h>

struct ring {
    uint8_t *storage;
    size_t capacity;
    size_t head;
    size_t tail;
};

static size_t ring_advance(size_t index, size_t capacity)
{
    ++index;
    return index == capacity ? 0u : index;
}
~~~

Use indexes modulo the capacity or expose two separate linear spans. Check that capacity is nonzero before wrap arithmetic.

## Memory-Mapped Regions

A hardware address range may be contiguous numerically without being an ordinary C array. Hardware can insert holes, side effects, protection boundaries, or access-width rules. Do not apply pointer arithmetic to a register base unless the vendor interface explicitly defines the register block as an array-like object and access pattern.

## Exercises

1. Implement a half-open range API and test empty, one-element, and full-array ranges.
2. Write a bounds-checked byte-slice function that returns a pointer and remaining length.
3. Demonstrate pointer subtraction inside one array and explain why subtracting pointers to separate arrays is invalid.
4. Implement ring-buffer index advancement for capacities one, two, and a power-of-two capacity.
5. Add overflow checks to a two-dimensional byte-offset calculation.
6. Review a driver for pointer arithmetic over MMIO and compare every access with the reference manual.

## Common Mistakes

- Dereferencing the one-past pointer.
- Advancing a pointer beyond one-past even if no dereference occurs.
- Subtracting pointers from different arrays.
- Comparing unrelated pointers with ordering operators.
- Checking a bound after indexing.
- Letting pointer and length views describe different storage.
- Assuming realloc preserves an old pointer.
- Treating wrapped ring-buffer storage as one C array.
- Performing raw address arithmetic for hardware registers.
- Using int for arbitrary pointer differences.

## Debugging Checklist

1. Name the array object that owns each pointer.
2. Record the valid range and one-past position.
3. Check every index before dereference and every multiplication before offset formation.
4. Use sanitizers and compiler bounds diagnostics on host tests.
5. Log ring-buffer indexes and capacity at wrap boundaries.
6. Check whether allocation, reset, or DMA ownership invalidated the pointer.
7. Inspect pointer subtraction types and compiler warnings.
8. For MMIO, replace generic pointer walks with documented register accessors.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Pointer Fundamentals](./pointer-fundamentals.md)
- [Memory Safety And Lifetime](./memory-safety-and-lifetime.md)
- [Object Representation, Alignment, And Padding](./object-representation-alignment-and-padding.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [CERT C array and pointer rules](https://wiki.sei.cmu.edu/confluence/display/c)
