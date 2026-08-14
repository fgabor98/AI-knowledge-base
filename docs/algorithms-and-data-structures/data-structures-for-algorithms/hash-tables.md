---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Hash Tables

A hash table maps a key to a bucket so that exact-key lookup is usually close to O(1). The table does not preserve sorted order, and its performance depends on the hash function, collision policy, load factor, and resizing or capacity policy.

Hashing is a data-model choice for repeated membership and exact-key operations. It is not a general replacement for arrays, sorted tables, or priority queues.

## Hash Table Vocabulary

Hash function:
: Maps a key to a numeric hash value.

Bucket:
: A slot or chain selected from the hash value.

Collision:
: Two different keys map to the same bucket.

Load factor:
: Occupied entries divided by available buckets.

Open addressing:
: Stores entries in the table itself and probes for another slot after a collision.

Separate chaining:
: Stores a collection of entries per bucket.

Tombstone:
: A deletion marker in an open-addressed table that preserves probe chains.

## Collision Strategies

Separate chaining tolerates deletion naturally but needs node storage or per-bucket arrays. Open addressing has compact storage and good locality, but deletion and high load factors require care.

Linear probing checks consecutive slots. It is simple and cache-friendly, but primary clusters can grow. Quadratic probing or double hashing changes the probe sequence but introduces more arithmetic and requires a proof that the sequence covers enough slots.

For fixed-capacity systems, reject insertion when a load or probe limit is reached. Do not let a lookup loop forever when the table is full or corrupted.

## Key And Hash Contracts

Keys must obey:

- equal keys produce equal hash values
- equality is deterministic
- key bytes remain unchanged while stored
- the hash width and serialization are documented

Cryptographic hashing is usually unnecessary for an in-memory table, but an attacker-controlled key set may need a keyed or randomized hash to avoid deliberate collision attacks.

## Programming Examples

### C: Fixed-Capacity Linear-Probing Table

This table stores unsigned integer keys and values. It uses a tombstone state for deletion and rejects insertion when no slot can be found.

```c
#include <stddef.h>
#include <stdint.h>

enum {
    HASH_CAPACITY = 17,
    HASH_EMPTY = 0,
    HASH_USED = 1,
    HASH_TOMBSTONE = 2
};

enum hash_status {
    HASH_OK = 0,
    HASH_NOT_FOUND,
    HASH_FULL,
    HASH_ERR_NULL
};

struct hash_entry {
    uint32_t key;
    int value;
    unsigned char state;
};

struct hash_table {
    struct hash_entry entries[HASH_CAPACITY];
    size_t count;
};

static size_t hash_key(uint32_t key)
{
    key ^= key >> 16;
    key *= UINT32_C(0x7feb352d);
    key ^= key >> 15;
    key *= UINT32_C(0x846ca68b);
    key ^= key >> 16;
    return key % HASH_CAPACITY;
}

void hash_table_init(struct hash_table *table)
{
    if (table == NULL)
        return;
    for (size_t i = 0; i < HASH_CAPACITY; i++)
        table->entries[i].state = HASH_EMPTY;
    table->count = 0;
}

enum hash_status hash_table_find(const struct hash_table *table,
                                 uint32_t key,
                                 int *out_value)
{
    size_t start;

    if (table == NULL || out_value == NULL)
        return HASH_ERR_NULL;
    start = hash_key(key);

    for (size_t step = 0; step < HASH_CAPACITY; step++) {
        size_t index = (start + step) % HASH_CAPACITY;
        const struct hash_entry *entry = &table->entries[index];

        if (entry->state == HASH_EMPTY)
            return HASH_NOT_FOUND;
        if (entry->state == HASH_USED && entry->key == key) {
            *out_value = entry->value;
            return HASH_OK;
        }
    }
    return HASH_NOT_FOUND;
}

enum hash_status hash_table_put(struct hash_table *table,
                                uint32_t key,
                                int value)
{
    size_t start;
    size_t tombstone = HASH_CAPACITY;

    if (table == NULL)
        return HASH_ERR_NULL;
    start = hash_key(key);

    for (size_t step = 0; step < HASH_CAPACITY; step++) {
        size_t index = (start + step) % HASH_CAPACITY;
        struct hash_entry *entry = &table->entries[index];

        if (entry->state == HASH_USED && entry->key == key) {
            entry->value = value;
            return HASH_OK;
        }
        if (entry->state == HASH_TOMBSTONE && tombstone == HASH_CAPACITY)
            tombstone = index;
        if (entry->state == HASH_EMPTY) {
            if (tombstone != HASH_CAPACITY)
                index = tombstone;
            table->entries[index] = (struct hash_entry){
                .key = key,
                .value = value,
                .state = HASH_USED
            };
            table->count++;
            return HASH_OK;
        }
    }

    if (tombstone != HASH_CAPACITY) {
        table->entries[tombstone] = (struct hash_entry){
            .key = key,
            .value = value,
            .state = HASH_USED
        };
        table->count++;
        return HASH_OK;
    }
    return HASH_FULL;
}

enum hash_status hash_table_remove(struct hash_table *table, uint32_t key)
{
    size_t start;

    if (table == NULL)
        return HASH_ERR_NULL;
    start = hash_key(key);
    for (size_t step = 0; step < HASH_CAPACITY; step++) {
        size_t index = (start + step) % HASH_CAPACITY;
        struct hash_entry *entry = &table->entries[index];

        if (entry->state == HASH_EMPTY)
            return HASH_NOT_FOUND;
        if (entry->state == HASH_USED && entry->key == key) {
            entry->state = HASH_TOMBSTONE;
            table->count--;
            return HASH_OK;
        }
    }
    return HASH_NOT_FOUND;
}
```

The bounded probe loop guarantees termination even if all slots are occupied. Tombstones preserve the fact that a later occupied entry may have probed across the deleted slot. Periodic rebuild or rehashing may be needed when tombstones accumulate.

### Python: Reference Dictionary Behavior

```python
class FixedTable:
    def __init__(self, capacity):
        if capacity <= 0:
            raise ValueError("capacity must be positive")
        self.capacity = capacity
        self.values = {}

    def put(self, key, value):
        if key not in self.values and len(self.values) == self.capacity:
            raise OverflowError("table is full")
        self.values[key] = value

    def get(self, key):
        return self.values.get(key)

    def remove(self, key):
        if key not in self.values:
            return False
        del self.values[key]
        return True
```

Python's dictionary hides hashing and collision handling, so it is a semantic reference rather than a model of the C probe sequence.

## Load Factor And Resizing

Open addressing generally needs a lower maximum load factor than chaining because long probe sequences appear as the table fills. A dynamic table can allocate a larger table and reinsert every live entry, but resizing is a burst of O(n) work and can fail due to memory exhaustion.

For a real-time or fixed-memory path, choose capacity up front or move resizing to a controlled preparation phase. A failed resize must not lose the original table.

## Complexity

Expected lookup, insertion, and deletion are O(1) under a good hash distribution and controlled load. Worst-case operations are O(n), including deliberate collisions or a poor hash function. Storage is O(capacity).

## Common Mistakes

- Stopping a probe at a tombstone and missing a later matching key.
- Treating a full table as an infinite probe loop.
- Incrementing count when replacing an existing key.
- Letting tombstones accumulate without a rebuild policy.
- Using mutable key bytes whose hash changes after insertion.
- Assuming average O(1) is a deadline guarantee.
- Resizing inside an interrupt, lock-held, or otherwise bounded path.

## Embedded And Systems Angle

- avoid unbounded resizing where memory must be predictable
- choose hash and bucket strategies for expected key sets
- handle collision-heavy cases deliberately
- cap probe work and report a full or degraded result
- consider sorted arrays when keys are few and deterministic iteration matters

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)
- [Maintaining Sorted Data](../sorting-and-ordering/maintaining-sorted-data.md)
- [Bitsets And Bitmaps](bitsets-and-bitmaps.md)
