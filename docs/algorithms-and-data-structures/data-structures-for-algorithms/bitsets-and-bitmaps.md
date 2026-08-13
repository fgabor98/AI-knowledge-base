---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Bitsets And Bitmaps

A bitset stores one boolean state per bit. A bitmap is the same idea commonly applied to dense identifiers, resources, pages, vertices, or flags. Bitsets reduce storage and make set operations available as word-level operations.

Use them when the universe of identifiers is dense and bounded. A bitmap is a poor representation for a sparse set with a very large maximum identifier.

## Bit Numbering

Choose and document whether bit zero is the least significant bit of word zero, how words are serialized, and how unused high bits are treated. In-memory bit numbering and wire-format byte order are separate concerns.

For a bitmap with `bit_count` bits and `WORD_BITS` bits per word:

```text
word = index / WORD_BITS
mask = 1 << (index % WORD_BITS)
word_count = ceil(bit_count / WORD_BITS)
```

Every operation must reject `index >= bit_count`, even when the backing word array has padding bits.

## Set Invariants

A valid bitmap maintains:

- its word count matches its allocated storage
- every public index is within the modeled universe
- unused padding bits are either zero or explicitly ignored
- a count field, if present, matches the set bits after every mutation

If the bitmap is serialized, clear padding bits before emitting it unless the format defines them as meaningful.

## Programming Examples

### C: Bounded Bitmap Operations

```c
#include <stddef.h>
#include <stdint.h>

enum {
    BITMAP_BITS = 64,
    BITMAP_WORD_BITS = 32,
    BITMAP_WORDS = (BITMAP_BITS + BITMAP_WORD_BITS - 1) / BITMAP_WORD_BITS
};

enum bitmap_status {
    BITMAP_OK = 0,
    BITMAP_ERR_NULL,
    BITMAP_ERR_INDEX
};

struct bitmap {
    uint32_t words[BITMAP_WORDS];
};

static int bitmap_valid_index(size_t index)
{
    return index < BITMAP_BITS;
}

enum bitmap_status bitmap_set(struct bitmap *bitmap, size_t index)
{
    if (bitmap == NULL)
        return BITMAP_ERR_NULL;
    if (!bitmap_valid_index(index))
        return BITMAP_ERR_INDEX;
    bitmap->words[index / BITMAP_WORD_BITS] |=
        UINT32_C(1) << (index % BITMAP_WORD_BITS);
    return BITMAP_OK;
}

enum bitmap_status bitmap_clear(struct bitmap *bitmap, size_t index)
{
    if (bitmap == NULL)
        return BITMAP_ERR_NULL;
    if (!bitmap_valid_index(index))
        return BITMAP_ERR_INDEX;
    bitmap->words[index / BITMAP_WORD_BITS] &=
        ~(UINT32_C(1) << (index % BITMAP_WORD_BITS));
    return BITMAP_OK;
}

enum bitmap_status bitmap_test(const struct bitmap *bitmap,
                               size_t index,
                               int *out_set)
{
    if (bitmap == NULL || out_set == NULL)
        return BITMAP_ERR_NULL;
    if (!bitmap_valid_index(index))
        return BITMAP_ERR_INDEX;
    *out_set = (bitmap->words[index / BITMAP_WORD_BITS] &
                (UINT32_C(1) << (index % BITMAP_WORD_BITS))) != 0;
    return BITMAP_OK;
}

size_t bitmap_find_first_zero(const struct bitmap *bitmap)
{
    if (bitmap == NULL)
        return BITMAP_BITS;
    for (size_t index = 0; index < BITMAP_BITS; index++) {
        int is_set;

        if (bitmap_test(bitmap, index, &is_set) == BITMAP_OK && !is_set)
            return index;
    }
    return BITMAP_BITS;
}
```

The sentinel `BITMAP_BITS` means no zero bit was found. This is safe because valid indexes are strictly smaller than `BITMAP_BITS`.

### C: Word-Level Set Operations

```c
void bitmap_or(struct bitmap *destination,
               const struct bitmap *left,
               const struct bitmap *right)
{
    if (destination == NULL || left == NULL || right == NULL)
        return;
    for (size_t i = 0; i < BITMAP_WORDS; i++)
        destination->words[i] = left->words[i] | right->words[i];
}

size_t bitmap_count(const struct bitmap *bitmap)
{
    size_t count = 0;

    if (bitmap == NULL)
        return 0;
    for (size_t i = 0; i < BITMAP_WORDS; i++) {
        uint32_t word = bitmap->words[i];
        while (word != 0) {
            word &= word - 1;
            count++;
        }
    }
    return count;
}
```

The `word &= word - 1` operation clears one set bit per iteration, so counting is proportional to the number of set bits rather than always to the word width. A platform builtin may be faster, but the invariant and width assumptions remain the same.

### Python: Set Reference

```python
class BitSet:
    def __init__(self, bit_count):
        if bit_count < 0:
            raise ValueError("bit_count must not be negative")
        self.bit_count = bit_count
        self.bits = 0

    def _check(self, index):
        if not 0 <= index < self.bit_count:
            raise IndexError(index)

    def set(self, index):
        self._check(index)
        self.bits |= 1 << index

    def clear(self, index):
        self._check(index)
        self.bits &= ~(1 << index)

    def test(self, index):
        self._check(index)
        return bool(self.bits & (1 << index))
```

Python integers grow as needed and therefore do not model fixed-word storage or padding bits. They are useful for checking set semantics.

## Resource Allocation

A bitmap can represent free resources:

1. find a zero bit
2. set it as part of allocation
3. return the index to the caller
4. clear it on release

Allocation needs an ownership rule. A double release can make one resource appear free while still in use; a missing release leaks capacity. In concurrent code, find-and-set must be atomic or protected by a lock.

## Visited-State Tracking

Graph traversals often use one bit per vertex. This reduces O(V) byte flags to O(V / word size) storage and supports fast clearing or combining when the vertex bound is known. Generation counters can avoid clearing a large bitmap between runs, but they add width and wraparound policy.

## Common Mistakes

- Shifting a signed value or by a width equal to the word size.
- Accepting padding-bit indexes beyond the modeled universe.
- Treating serialized bit order as identical to in-memory byte order.
- Forgetting to clear a bit on resource release.
- Returning the bitmap sentinel as if it were a valid resource index.
- Performing non-atomic allocation in a concurrent bitmap.

## Embedded And Systems Angle

- use bitmaps when identifiers are dense and bounded
- document bit numbering and endianness assumptions where serialized
- make concurrent bit updates atomic when required
- use them for compact visited state, resource allocation, and flags
- clear or validate unused bits at ABI and wire-format boundaries

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Depth-First Search](../graph-algorithms/depth-first-search.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)
- [Graph Representations](../graph-algorithms/graph-representations.md)
