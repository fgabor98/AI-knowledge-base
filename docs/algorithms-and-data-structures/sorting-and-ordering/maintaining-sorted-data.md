---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Maintaining Sorted Data

There are two ways to obtain ordered data: collect unsorted values and sort them when needed, or preserve sorted order as values arrive. Maintaining order makes lookup and ordered iteration simple, but every insertion or deletion may move many elements.

The right choice depends on the workload, not on the name of the data structure.

## Sort Once Or Maintain Order

Let:

- `n` be the number of values
- `q` be the number of lookups or ordered traversals
- `u` be the number of updates

Sorting on demand has a cost such as O(n log n) per sort. Maintaining a sorted array has O(n) insertion and deletion but O(log n) lookup. A useful first comparison is:

```text
sort-on-read cost ~= number_of_sorts * sort_cost
maintained-order cost ~= updates * movement_cost + lookups * lookup_cost
```

This is only a model. Cache locality, record size, duplicate policy, and maximum bounds can change the practical decision.

## Sorted Array Invariant

For an array containing `count` entries:

- `0 <= count <= capacity`
- entries in `[0, count)` are initialized
- keys are nondecreasing according to the comparator
- entries in `[count, capacity)` are outside the logical collection

If duplicates are allowed, decide whether insertion appends after existing equal keys, before them, or replaces an existing record. That policy affects stability and lookup behavior.

## Operations And Costs

| Operation | Sorted fixed array | Hash table | Heap |
| --- | --- | --- | --- |
| find exact key | O(log n) | average O(1) | not the purpose |
| find range | simple | usually awkward | not the purpose |
| insert | O(n) movement | average O(1) | O(log n) |
| delete | O(n) movement | average O(1) | O(log n) for root |
| ordered iteration | O(n) | requires extra work | priority order only |
| storage overhead | low | buckets/metadata | low for array heap |

For small bounded collections, an array can outperform pointer-rich structures because binary search and linear movement operate on contiguous memory.

## Programming Examples

### C: A Fixed-Capacity Sorted Table

This table uses replacement semantics for duplicate keys: inserting an existing key updates its value without changing the entry count. The lower-bound operation returns the first entry whose key is greater than or equal to the target.

```c
#include <stddef.h>
#include <string.h>

enum {
    SORTED_TABLE_CAPACITY = 16
};

enum sorted_table_status {
    SORTED_TABLE_OK = 0,
    SORTED_TABLE_NOT_FOUND,
    SORTED_TABLE_FULL,
    SORTED_TABLE_ERR_NULL
};

struct sorted_entry {
    int key;
    int value;
};

struct sorted_table {
    struct sorted_entry entries[SORTED_TABLE_CAPACITY];
    size_t count;
};

static size_t sorted_lower_bound(const struct sorted_table *table, int key)
{
    size_t lo = 0;
    size_t hi = table->count;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if (table->entries[mid].key < key)
            lo = mid + 1;
        else
            hi = mid;
    }

    return lo;
}

enum sorted_table_status sorted_table_find(const struct sorted_table *table,
                                           int key,
                                           int *out_value)
{
    size_t index;

    if (table == NULL || out_value == NULL)
        return SORTED_TABLE_ERR_NULL;

    index = sorted_lower_bound(table, key);
    if (index == table->count || table->entries[index].key != key)
        return SORTED_TABLE_NOT_FOUND;

    *out_value = table->entries[index].value;
    return SORTED_TABLE_OK;
}

enum sorted_table_status sorted_table_put(struct sorted_table *table,
                                          int key,
                                          int value)
{
    size_t index;

    if (table == NULL)
        return SORTED_TABLE_ERR_NULL;
    if (table->count > SORTED_TABLE_CAPACITY)
        return SORTED_TABLE_ERR_NULL;

    index = sorted_lower_bound(table, key);
    if (index < table->count && table->entries[index].key == key) {
        table->entries[index].value = value;
        return SORTED_TABLE_OK;
    }
    if (table->count == SORTED_TABLE_CAPACITY)
        return SORTED_TABLE_FULL;

    memmove(&table->entries[index + 1],
            &table->entries[index],
            (table->count - index) * sizeof(table->entries[0]));
    table->entries[index] = (struct sorted_entry){ .key = key,
                                                    .value = value };
    table->count++;
    return SORTED_TABLE_OK;
}

enum sorted_table_status sorted_table_remove(struct sorted_table *table,
                                             int key,
                                             int *out_value)
{
    size_t index;

    if (table == NULL || out_value == NULL)
        return SORTED_TABLE_ERR_NULL;

    index = sorted_lower_bound(table, key);
    if (index == table->count || table->entries[index].key != key)
        return SORTED_TABLE_NOT_FOUND;

    *out_value = table->entries[index].value;
    memmove(&table->entries[index],
            &table->entries[index + 1],
            (table->count - index - 1) * sizeof(table->entries[0]));
    table->count--;
    return SORTED_TABLE_OK;
}
```

The `memmove` calls are safe for zero bytes and correctly handle overlapping ranges. In production code, validate the structure invariant at public boundaries or in debug builds.

Lookup takes O(log n). Insertion and removal take O(n) in the worst case because entries after the insertion point move. The storage is fixed and the table performs no allocation.

### Python: Bisect-Based Reference

```python
from bisect import bisect_left


class SortedTable:
    def __init__(self, capacity):
        if capacity < 0:
            raise ValueError("capacity must not be negative")
        self.capacity = capacity
        self.entries = []

    def put(self, key, value):
        index = bisect_left(self.entries, (key,))
        if index < len(self.entries) and self.entries[index][0] == key:
            self.entries[index] = (key, value)
            return
        if len(self.entries) == self.capacity:
            raise OverflowError("sorted table is full")
        self.entries.insert(index, (key, value))

    def find(self, key):
        index = bisect_left(self.entries, (key,))
        if index == len(self.entries) or self.entries[index][0] != key:
            return None
        return self.entries[index][1]
```

The Python model is useful for checking duplicate replacement, insertion points, and full-capacity behavior against the C implementation.

## Duplicate And Update Policy

Common policies include:

- reject duplicate keys
- replace the existing value
- keep all values and insert before or after equal keys
- aggregate values under the same key

The policy must be consistent across insertion, lookup, deletion, and iteration. A caller that needs all duplicate records should not use a replacement table accidentally.

## When Not To Use A Sorted Array

Consider another representation when:

- updates dominate and movement is expensive
- the collection grows beyond a practical fixed bound
- lookup is exact-key only and hashing is suitable
- only the smallest or largest item matters, suggesting a heap
- records are large enough that shifting them is costly
- concurrent readers and writers require a different synchronization design

Do not assume a tree is automatically better. Pointer chasing, allocation, balancing, and fragmentation are costs too.

## Common Mistakes

- Inserting at the wrong lower-bound position and breaking sortedness.
- Forgetting to distinguish logical count from physical capacity.
- Moving records but not their associated payload.
- Leaving duplicate-key behavior implicit.
- Returning a pointer into an array that a later insertion can move.
- Computing midpoint as `lo + hi` and risking overflow.
- Calling O(n) insertion acceptable without stating the maximum `n` and update rate.

## Embedded And Systems Angle

- sorted arrays can beat trees for small bounded collections because of locality and low overhead
- choose insertion movement when reads dominate and the maximum count is known
- define duplicate, replacement, and full-capacity policy in the API
- use caller-owned storage when allocation failure is not locally recoverable
- keep array mutation and lookup synchronized according to the reader/writer model

## Related Topics

- [Binary Search](../basic-algorithm-schemes/binary-search.md)
- [Sorting Fundamentals](sorting-fundamentals.md)
- [Priority And Partial Ordering](priority-and-partial-ordering.md)
- [Hash Tables](../data-structures-for-algorithms/hash-tables.md)
- [Arrays Buffers And Records](../data-structures-for-algorithms/arrays-buffers-and-records.md)
