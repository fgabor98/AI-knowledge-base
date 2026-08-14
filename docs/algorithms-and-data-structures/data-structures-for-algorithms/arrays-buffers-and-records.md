---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Arrays Buffers And Records

Contiguous arrays are the default storage model for many algorithms. They provide constant-time indexing, predictable layout, good locality, and simple ownership when the maximum size is known. Buffers and records extend the same idea to bytes and structured fields.

The important distinction is between storage capacity and logical length. Capacity describes how much memory exists; length describes how much of it currently contains valid data.

## Array Model

For an array-backed collection, define:

- element type and size
- logical length
- storage capacity
- valid index range `[0, length)`
- whether the collection owns the storage
- whether operations may move or mutate elements
- behavior when capacity is reached

The basic invariant is:

```text
0 <= length <= capacity
```

Only elements below `length` may be read as initialized collection data. Bytes or slots between `length` and `capacity` are reserved storage, not implicit zero values.

## Buffers And Strings

A byte buffer needs more than a pointer and a capacity. Document:

- whether the data is binary or text
- whether a terminator is required
- whether length includes the terminator
- encoding assumptions
- ownership and mutability
- whether partial writes are allowed

For a C string, the allocated capacity must include the terminating byte. For a binary protocol, a zero byte is ordinary data and cannot be used as an end marker.

## Records And Layout

A record groups related fields with one ownership and lifetime model. Arrays of records are convenient when algorithms process one complete item at a time. Structure-of-arrays layouts can be better when an algorithm scans one field across many records.

| Layout | Strength | Tradeoff |
| --- | --- | --- |
| array of structures | natural item ownership | loads unused fields during field-specific scans |
| structure of arrays | efficient column scans and SIMD | joining fields by index must remain consistent |
| packed bytes | compact wire or DMA format | alignment and decoding costs |
| padded native record | simple CPU access | ABI, padding, and serialization concerns |

Do not serialize a native C struct by copying its bytes unless padding, alignment, endianness, and field widths are part of the format contract.

## Programming Examples

### C: Bounded Byte Buffer

This buffer keeps length separate from capacity and reserves one byte for a C-string terminator only when the caller chooses the text operation.

```c
#include <stddef.h>
#include <string.h>

enum buffer_status {
    BUFFER_OK = 0,
    BUFFER_FULL,
    BUFFER_ERR_NULL,
    BUFFER_ERR_LENGTH
};

struct byte_buffer {
    unsigned char *data;
    size_t length;
    size_t capacity;
};

enum buffer_status buffer_append(struct byte_buffer *buffer,
                                 const void *source,
                                 size_t source_length)
{
    if (buffer == NULL ||
        (buffer->data == NULL && buffer->capacity > 0) ||
        (source == NULL && source_length > 0))
        return BUFFER_ERR_NULL;
    if (buffer->length > buffer->capacity)
        return BUFFER_ERR_LENGTH;
    if (source_length > buffer->capacity - buffer->length)
        return BUFFER_FULL;

    memcpy(buffer->data + buffer->length, source, source_length);
    buffer->length += source_length;
    return BUFFER_OK;
}

enum buffer_status buffer_append_text(struct byte_buffer *buffer,
                                      const char *text)
{
    size_t text_length;

    if (buffer == NULL || text == NULL ||
        (buffer->data == NULL && buffer->capacity > 0))
        return BUFFER_ERR_NULL;
    text_length = strlen(text);
    if (text_length == buffer->capacity - buffer->length)
        return BUFFER_FULL;
    if (text_length > buffer->capacity - buffer->length - 1)
        return BUFFER_FULL;

    memcpy(buffer->data + buffer->length, text, text_length);
    buffer->length += text_length;
    buffer->data[buffer->length] = '\0';
    return BUFFER_OK;
}
```

The text operation requires one free byte for the terminator. The first equality check prevents subtracting one from zero in the following condition.

### C: Length-Bounded Record Scan

```c
struct reading_record {
    uint32_t sequence;
    int value;
    unsigned char valid;
};

size_t count_valid_records(const struct reading_record *records,
                           size_t count)
{
    size_t valid_count = 0;

    if (records == NULL && count > 0)
        return 0;
    for (size_t i = 0; i < count; i++)
        if (records[i].valid != 0)
            valid_count++;
    return valid_count;
}
```

The function uses the logical count, not the allocation size. A caller that needs to distinguish invalid input from zero valid records should return a status instead of overloading zero.

### Python: Capacity-Aware Reference

```python
class ByteBuffer:
    def __init__(self, capacity):
        if capacity < 0:
            raise ValueError("capacity must not be negative")
        self.data = bytearray(capacity)
        self.length = 0

    def append(self, data):
        if len(data) > len(self.data) - self.length:
            raise BufferError("buffer is full")
        end = self.length + len(data)
        self.data[self.length:end] = data
        self.length = end

    def value(self):
        return bytes(self.data[:self.length])
```

The Python model makes logical slicing explicit and is useful for testing boundary cases. It does not model C alignment or ownership rules.

## Algorithmic Costs

Arrays provide:

- indexing: O(1)
- scan: O(n)
- append with spare capacity: O(1)
- insertion in the middle: O(n) movement
- deletion in the middle: O(n) movement
- binary search when sorted: O(log n)

Changing from a linked structure to an array can make an algorithm faster even when both have the same asymptotic scan cost, because contiguous access uses fewer pointers and usually has better cache behavior.

## Common Mistakes

- Confusing capacity with initialized length.
- Checking `length + source_length <= capacity` in a type where addition can overflow.
- Forgetting the text terminator or counting it inconsistently.
- Reading padding bytes as serialized fields.
- Moving a record's key without moving its payload.
- Returning pointers into storage that a later resize or compaction can move.
- Allowing a partial write without returning the number of bytes committed.

## Embedded And Systems Angle

- prefer fixed-size arrays when the maximum size is known
- keep logical length separate from storage capacity
- validate byte-buffer ownership, encoding, and termination rules
- use contiguous layouts for cache, DMA, and simple bounds reasoning
- make partial-write and full-buffer behavior explicit

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Constant Factors And Cache Effects](../complexity-and-efficiency/constant-factors-and-cache-effects.md)
- [Cache-Aware And DMA-Friendly Layouts](../embedded-linux-algorithmic-constraints/cache-aware-and-dma-friendly-layouts.md)
- [Maintaining Sorted Data](../sorting-and-ordering/maintaining-sorted-data.md)
