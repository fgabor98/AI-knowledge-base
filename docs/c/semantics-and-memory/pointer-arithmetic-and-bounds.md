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

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: a half-open range API"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    /* [begin, end): begin points at the first element, end one past the last. */
    struct u16_range {
        const uint16_t *begin;
        const uint16_t *end;
    };

    static size_t range_length(struct u16_range range)
    {
        return (size_t)(range.end - range.begin);
    }

    static uint32_t range_sum(struct u16_range range)
    {
        uint32_t sum = 0;

        for (const uint16_t *p = range.begin; p != range.end; p++) {
            sum += *p;
        }
        return sum;
    }

    static bool range_contains(struct u16_range range, const uint16_t *p)
    {
        for (const uint16_t *q = range.begin; q != range.end; q++) {
            if (q == p) {
                return true;
            }
        }
        return false;
    }

    int main(void)
    {
        const uint16_t data[4] = { 1, 2, 3, 4 };
        struct u16_range empty = { data, data };
        struct u16_range one = { data + 3, data + 4 };
        struct u16_range full = { data, data + 4 };     /* end is one past: valid, never read */

        assert(range_length(empty) == 0 && range_sum(empty) == 0);
        assert(range_length(one) == 1 && range_sum(one) == 4);
        assert(range_length(full) == 4 && range_sum(full) == 10);
        assert(!range_contains(full, data + 4));        /* end is not inside the range */
        assert(range_contains(one, data + 3));
        return 0;
    }
    ```

    The half-open form needs no special case: empty is `begin == end`, and lengths add up
    when ranges are split. `range_contains` compares with `==` only. Comparing with `<` would
    be undefined for a pointer into a different array, while `==` is always defined.

??? note "Exercise 2: a bounds-checked byte slice"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    /* Gives the bytes from offset to the end. offset == length gives an empty slice. */
    bool slice_from(const uint8_t *data, size_t length, size_t offset,
                    const uint8_t **slice, size_t *remaining)
    {
        if (offset > length) {
            return false;                    /* check before forming data + offset */
        }
        *slice = data + offset;
        *remaining = length - offset;
        return true;
    }

    int main(void)
    {
        const uint8_t frame[5] = { 1, 2, 3, 4, 5 };
        const uint8_t *rest;
        size_t remaining;

        assert(slice_from(frame, 5, 2, &rest, &remaining) && rest[0] == 3 && remaining == 3);
        assert(slice_from(frame, 5, 5, &rest, &remaining) && remaining == 0);   /* one past */
        assert(!slice_from(frame, 5, 6, &rest, &remaining));
        assert(!slice_from(frame, 5, SIZE_MAX, &rest, &remaining));
        return 0;
    }
    ```

    The check runs on the integer before the pointer is formed, because `data + offset`
    beyond one past the end is already undefined, even if it is never dereferenced. The
    pointer and the remaining length always come back together, so the caller cannot
    pair a new pointer with an old length.

??? note "Exercise 3: subtracting pointers"
    Pointer subtraction is defined only when both pointers point into, or one past the end
    of, the same array object (C11 6.5.6p9). The result is the difference in *elements*,
    with type `ptrdiff_t`:

    ```c
    #include <stddef.h>

    ptrdiff_t elements_between(void)
    {
        int samples[16];
        int *first = &samples[2];
        int *last = &samples[10];

        return last - first;        /* 8, not 32: counted in elements */
    }
    ```

    Subtracting pointers to two separate arrays is undefined, even though both are
    addresses and the machine instruction would produce a number. The standard does not
    define a relationship between separate objects: they may be in different segments,
    memory banks, or address spaces, and the optimizer may assume the pointers belong to
    the same object. To measure the distance between unrelated objects, for example in a
    memory map, convert to `uintptr_t` first. That is implementation-defined, but it states
    clearly that you are doing address arithmetic rather than array arithmetic.

??? note "Exercise 4: ring-buffer index advancement"
    ```c
    #include <assert.h>
    #include <stddef.h>

    static size_t ring_advance(size_t index, size_t capacity)
    {
        ++index;
        return index == capacity ? 0u : index;
    }

    /* Faster variant, only for power-of-two capacities. */
    static size_t ring_advance_pow2(size_t index, size_t capacity)
    {
        assert(capacity != 0 && (capacity & (capacity - 1)) == 0);
        return (index + 1u) & (capacity - 1u);
    }

    int main(void)
    {
        assert(ring_advance(0, 1) == 0);                  /* capacity 1: always slot 0 */
        assert(ring_advance(0, 2) == 1 && ring_advance(1, 2) == 0);
        for (size_t i = 0; i < 8; i++) {
            assert(ring_advance(i, 8) == ring_advance_pow2(i, 8));
        }
        assert(ring_advance(7, 8) == 0);
        assert(ring_advance_pow2(0, 1) == 0);             /* mask 0 still works */
        return 0;
    }
    ```

    The comparison version works for every capacity, including 1, and never divides.
    `index % capacity` would also work, but it costs a division on cores without a fast
    divider. The mask version needs a power of two: with capacity 6 the mask is 5, and
    index 5 + 1 = 6 masks to 4, not 0. The assertion catches that misuse. Capacity 0 has no
    valid index, so it must be rejected when the ring is created.

??? note "Exercise 5: an overflow-checked 2-D byte offset"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    /* Byte offset of element [row][column] in a rows x columns array of element_size. */
    bool byte_offset_2d(size_t rows, size_t columns, size_t element_size,
                        size_t row, size_t column, size_t *offset)
    {
        if (row >= rows || column >= columns || element_size == 0) {
            return false;
        }
        if (columns > SIZE_MAX / element_size) {
            return false;
        }
        size_t row_bytes = columns * element_size;
        if (rows > SIZE_MAX / row_bytes) {
            return false;                          /* the whole array is not addressable */
        }
        *offset = row * row_bytes + column * element_size;
        return true;
    }

    int main(void)
    {
        size_t offset;

        assert(byte_offset_2d(3, 4, 2, 2, 3, &offset) && offset == 22);   /* last element */
        assert(!byte_offset_2d(3, 4, 2, 3, 0, &offset));                  /* row out of range */
        assert(!byte_offset_2d(2, SIZE_MAX / 2, 4, 0, 0, &offset));       /* row size overflows */
        assert(!byte_offset_2d(SIZE_MAX, 2, 1, 0, 0, &offset));           /* total overflows */
        return 0;
    }
    ```

    Checking that the *whole array* fits is enough. Once `rows * row_bytes` is known not to
    overflow, every `row * row_bytes + column * element_size` with valid indexes is smaller
    than the total, so it cannot overflow either. Checking the index bounds first also means
    an out-of-range index is reported as such, not as an overflow.

??? note "Exercise 6: reviewing pointer arithmetic over MMIO"
    The most common MMIO arithmetic bug is mixing byte offsets and element counts:

    ```c
    #include <stdint.h>

    #define TIMER_BASE 0x40000400u

    static volatile uint32_t *const timer = (volatile uint32_t *)TIMER_BASE;

    void timer_review_example(void)
    {
        timer[0x10] = 1u;       /* reaches TIMER_BASE + 0x40, not + 0x10 */
        timer[0x10 / 4] = 1u;   /* reaches TIMER_BASE + 0x10 */
    }
    ```

    For each access, record and compare with the reference manual:

    | Access in code | Address it reaches | Manual says at that address | Width in code / required | Verdict |
    | --- | --- | --- | --- | --- |
    | `timer[0x10]` | base + 0x40 | `CCR1` (capture/compare 1) | 32 / 32 | bug: meant `CR2` at + 0x10 |
    | `timer[0x10 / 4]` | base + 0x10 | `CR2` | 32 / 32 | correct, but use a named offset |
    | `((volatile uint16_t *)timer)[1]` | base + 0x02 | reserved half of `CR1` | 16 / 32 only | bug: width not allowed |

    The fix is to make arithmetic disappear from driver code: a vendor register structure
    whose offsets are checked with `_Static_assert(offsetof(...))`, or named byte offsets
    used through one accessor such as `reg_write32(TIMER_BASE + TIMER_CR2, value)`. The
    addresses and register names here are illustrative; the method is the point.

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
