---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Data Modeling And Abstract Data Types

Data modeling is the choice of how a problem is represented. An abstract data type is the behavior you promise to callers independent of the representation used internally.

This distinction matters because many algorithmic problems become simple or difficult depending on representation. A poor data model can force complicated code. A good data model can make the algorithm almost obvious.

## Data Model Vs Data Structure

A data model describes the meaning and rules of the data:

- readings have a valid range
- a history has a maximum capacity
- the oldest reading can be overwritten
- callers may query the number of stored readings
- invalid states are not observable

A data structure describes the concrete representation:

- fixed array
- linked list
- ring buffer
- heap
- hash table
- bitmap

The data model should usually come first. The structure is chosen to support the model's operations and constraints.

## Abstract Data Types

An abstract data type defines behavior through operations.

For a reading history, possible operations are:

- initialize
- clear
- push a new reading
- get the number of stored readings
- read the most recent value
- compute the minimum, maximum, or average

The caller does not need to know whether the history is implemented as an array, ring buffer, linked list, or file-backed store. The caller needs a stable contract.

## Choosing A Representation

Choose a representation by asking what the algorithm must do most often.

Arrays are strong when:

- maximum size is known
- indexed access is useful
- traversal dominates
- cache locality matters
- allocation should be avoided

Linked structures are useful when:

- insertion and removal in the middle dominate
- object ownership is already explicit
- elements must live in more than one container policy

Bitmaps are useful when:

- identifiers are dense and bounded
- the state is boolean or set membership
- compact representation matters

Hash tables are useful when:

- key lookup dominates
- ordering is not required
- memory growth and collision policy are acceptable

For embedded and systems work, a simple fixed array is often the correct first representation when the maximum size is small and known.

## Representation Independence

Representation independence means callers depend on operations, not fields.

Avoid exposing this as a public contract:

```c
history.values[history.next] = value;
history.next++;
```

Prefer exposing this:

```c
reading_history_push(&history, value);
```

The operation can preserve invariants even if the internal representation changes later.

## Programming Examples

### C: Fixed-Size Reading History ADT

This example uses a fixed-size array as the representation, but callers interact through operations. The invariant is kept inside the implementation boundary.

```c
#include <stddef.h>

enum {
    READING_HISTORY_CAPACITY = 16
};

enum reading_history_status {
    READING_HISTORY_OK = 0,
    READING_HISTORY_ERR_NULL,
    READING_HISTORY_ERR_EMPTY
};

struct reading_history {
    int values[READING_HISTORY_CAPACITY];
    size_t count;
    size_t next;
};

static int reading_history_valid(const struct reading_history *history)
{
    return history != NULL
        && history->count <= READING_HISTORY_CAPACITY
        && history->next < READING_HISTORY_CAPACITY;
}

void reading_history_init(struct reading_history *history)
{
    if (history == NULL)
        return;

    history->count = 0;
    history->next = 0;
}

enum reading_history_status reading_history_push(struct reading_history *history,
                                                 int value)
{
    if (!reading_history_valid(history))
        return READING_HISTORY_ERR_NULL;

    history->values[history->next] = value;
    history->next = (history->next + 1) % READING_HISTORY_CAPACITY;

    if (history->count < READING_HISTORY_CAPACITY)
        history->count++;

    return READING_HISTORY_OK;
}

enum reading_history_status reading_history_latest(
    const struct reading_history *history,
    int *out_value)
{
    size_t index;

    if (!reading_history_valid(history) || out_value == NULL)
        return READING_HISTORY_ERR_NULL;
    if (history->count == 0)
        return READING_HISTORY_ERR_EMPTY;

    index = (history->next + READING_HISTORY_CAPACITY - 1)
          % READING_HISTORY_CAPACITY;
    *out_value = history->values[index];

    return READING_HISTORY_OK;
}
```

The caller can push and query readings without knowing the wraparound arithmetic. That is the value of the ADT boundary.

### C: Swappable Representation Boundary

If the public API is stable, the internals can change later. For example, a future implementation could store `oldest` instead of `next`, or track a running sum for faster averages. The caller-facing operations should not change unless the abstract behavior changes.

```c
/* Public behavior:
 * - push stores the newest reading
 * - latest returns the newest reading
 * - empty history returns READING_HISTORY_ERR_EMPTY
 *
 * Private representation:
 * - fixed array
 * - count of initialized slots
 * - next write position
 */
```

This short comment is not a substitute for code, but it documents the boundary between abstract behavior and representation.

### Python: Reference ADT

Python can model the same ADT compactly. This is useful for expected behavior and for tests that compare sequences of operations.

```python
class ReadingHistory:
    def __init__(self, capacity):
        if capacity <= 0:
            raise ValueError("capacity must be positive")
        self._capacity = capacity
        self._values = []

    def push(self, value):
        if len(self._values) == self._capacity:
            self._values.pop(0)
        self._values.append(value)

    def latest(self):
        if not self._values:
            raise ValueError("history is empty")
        return self._values[-1]

    def count(self):
        return len(self._values)
```

The Python representation uses a list differently than the C representation uses an array. The abstract behavior is the same: bounded history, newest reading, empty-state error.

## Representation Tradeoff Example

For a bounded reading history:

| Representation | Strength | Weakness |
| --- | --- | --- |
| Fixed array with count | Simple traversal and no allocation | Removing oldest may require shifting unless ring logic is added |
| Ring buffer | O(1) insertion and bounded memory | Wraparound indexes require careful invariants |
| Linked list | Flexible insertion and removal | Allocation, pointer chasing, and ownership complexity |
| Running summary only | Very small memory | Cannot answer questions about individual readings |

The right model depends on required operations. If the only required output is a running average, storing every reading may be unnecessary. If diagnostics need the last N readings, a bounded history is part of the model.

## Common Mistakes

- Choosing a container before listing operations.
- Exposing representation fields as the public interface.
- Ignoring ownership and lifetime.
- Using dynamic allocation when fixed capacity is known and sufficient.
- Optimizing lookup while the actual workload is mostly traversal.
- Forgetting that a representation has invariants that every operation must preserve.

## Data Modeling Checklist

Before choosing a structure, write:

- required operations
- expected operation frequency
- maximum element count
- ordering requirements
- lookup requirements
- mutation requirements
- ownership and lifetime rules
- memory allocation policy
- concurrency or interrupt-context constraints

## Embedded And Systems Angle

- choose fixed bounds and ownership rules early
- hide representation details when future layout changes are likely
- keep allocation, lifetime, and concurrency constraints visible
- prefer simple representations until workload or constraints justify a more complex one

## Related Topics

- [Problem Modeling](problem-modeling.md)
- [Data Structures For Algorithms](../data-structures-for-algorithms/index.md)
- [Intrusive Data Structures](../data-structures-for-algorithms/intrusive-data-structures.md)
