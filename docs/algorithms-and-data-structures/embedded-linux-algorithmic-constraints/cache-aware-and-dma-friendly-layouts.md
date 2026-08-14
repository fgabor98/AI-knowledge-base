---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Cache-Aware And DMA-Friendly Layouts

Data layout affects algorithm cost beyond Big-O notation. Contiguous arrays can reduce cache misses; alignment can enable efficient device access; pointer-rich structures can be difficult or impossible for hardware to consume directly.

DMA adds another boundary: the CPU and device may have different views of ownership and memory visibility. A buffer that is convenient for an algorithm is not automatically a valid DMA descriptor or device-visible payload.

## Layout Choices

| Layout | CPU strength | Device or cache concern |
| --- | --- | --- |
| contiguous array | simple scans and locality | alignment, large transfers |
| array of records | natural item access | loads unused fields |
| structure of arrays | field scans and vectorization | index consistency across arrays |
| linked nodes | flexible insertion | pointer chasing and DMA incompatibility |
| descriptor plus payload | separates metadata and bytes | ownership and lifetime coordination |

Choose layout from access patterns and boundaries. Flatten a pointer graph before handing data to hardware or a serialization format.

## Cache-Line Reasoning

Cache-aware design considers:

- working-set size
- stride and sequentiality
- reuse distance
- false sharing between writers
- alignment and cache-line boundaries
- clean/invalidate operations for non-coherent devices

Avoid claiming that a field is cache-friendly without identifying which context accesses it and whether other contexts write the same line.

## DMA Ownership

A DMA buffer lifecycle often looks like:

```text
CPU prepares -> synchronize/clean if required -> device owns
device completes -> synchronize/invalidate if required -> CPU owns
CPU consumes or reuses
```

The exact operations depend on architecture, mapping API, cache coherency, and kernel subsystem. The algorithmic invariant is that only the owner mutates device-visible contents at a given time.

## Programming Examples

### C: Flat Descriptor Layout

This representation keeps a fixed-size descriptor array separate from byte storage. Offsets are used instead of pointers in the device-visible records.

```c
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
    DMA_MAX_DESCRIPTORS = 8,
    DMA_BUFFER_BYTES = 2048,
    DMA_NO_BUFFER = UINT32_MAX
};

enum dma_layout_status {
    DMA_LAYOUT_OK = 0,
    DMA_LAYOUT_FULL,
    DMA_LAYOUT_ERR_NULL,
    DMA_LAYOUT_ERR_RANGE
};

struct dma_descriptor {
    uint32_t offset;
    uint16_t length;
    uint16_t flags;
};

struct dma_ring_layout {
    struct dma_descriptor descriptors[DMA_MAX_DESCRIPTORS];
    unsigned char buffer[DMA_BUFFER_BYTES];
    size_t count;
    size_t used_bytes;
};

enum dma_layout_status dma_append(struct dma_ring_layout *layout,
                                  const void *data,
                                  size_t length,
                                  uint16_t flags)
{
    if (layout == NULL || (data == NULL && length > 0))
        return DMA_LAYOUT_ERR_NULL;
    if (length > UINT16_MAX ||
        layout->count > DMA_MAX_DESCRIPTORS ||
        layout->used_bytes > DMA_BUFFER_BYTES)
        return DMA_LAYOUT_ERR_RANGE;
    if (layout->count == DMA_MAX_DESCRIPTORS ||
        length > DMA_BUFFER_BYTES - layout->used_bytes)
        return DMA_LAYOUT_FULL;

    memcpy(layout->buffer + layout->used_bytes, data, length);
    layout->descriptors[layout->count++] = (struct dma_descriptor){
        .offset = (uint32_t)layout->used_bytes,
        .length = (uint16_t)length,
        .flags = flags
    };
    layout->used_bytes += length;
    return DMA_LAYOUT_OK;
}
```

The example needs `<string.h>` for `memcpy`; a device-facing implementation would also need explicit alignment, mapping, cache synchronization, and ownership operations defined by the target API. The descriptor contains no process pointer and can therefore be serialized or interpreted in a defined address space.

### C: Structure-of-Arrays Scan

```c
struct sample_columns {
    int *values;
    unsigned char *valid;
    size_t count;
};

size_t count_valid_values(const struct sample_columns *columns)
{
    size_t result = 0;

    if (columns == NULL ||
        (columns->values == NULL && columns->count > 0) ||
        (columns->valid == NULL && columns->count > 0))
        return 0;
    for (size_t i = 0; i < columns->count; i++)
        if (columns->valid[i] != 0)
            result++;
    return result;
}
```

Scanning only validity avoids loading the value field. The parallel arrays must have identical logical length and lifetime; one array must not be resized or reordered without the others.

### Python: Layout Reference

```python
def valid_values(records):
    values = [record["value"] for record in records]
    valid = [record["valid"] for record in records]
    return [value for value, is_valid in zip(values, valid) if is_valid]
```

Python expresses the transformation but not cache lines or DMA ownership. Use it for semantic tests, not layout measurements.

## Copying Versus Sharing

Copying can simplify ownership and produce a contiguous device or serialization layout. Sharing can avoid copy cost but requires synchronization and lifetime coordination. Compare:

- copy bytes and CPU time
- cache pollution
- buffer lifetime
- device mapping cost
- whether producer and consumer can mutate concurrently

An extra copy is often safer than sharing a mutable pointer graph across an ABI boundary.

## Alignment And Endianness

Alignment requirements belong in the layout contract. Use fixed-width integer types for wire or device fields and convert endianness at the boundary. Do not assume the host's native struct padding or byte order matches hardware documentation.

## Common Mistakes

- Passing process pointers to hardware or serializing them as offsets.
- Forgetting cache clean/invalidate or DMA ownership transitions.
- Mixing metadata and payload lifetimes.
- Assuming contiguous storage is automatically aligned for every device.
- Updating a DMA-visible buffer while the device still owns it.
- Reordering one structure-of-arrays column without reordering the others.
- Omitting `<string.h>` or otherwise presenting non-compilable example code.

## Embedded And Systems Angle

- separate CPU-efficient layout from device-visible layout when needed
- account for alignment, cache maintenance, and ownership transfer
- avoid pointer-rich structures in buffers shared with hardware
- use offsets, fixed-width fields, and explicit lengths at ABI boundaries
- measure copy and cache costs on the target architecture

## Related Topics

- [Embedded Linux Algorithmic Constraints](index.md)
- [Arrays Buffers And Records](../data-structures-for-algorithms/arrays-buffers-and-records.md)
- [Constant Factors And Cache Effects](../complexity-and-efficiency/constant-factors-and-cache-effects.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
