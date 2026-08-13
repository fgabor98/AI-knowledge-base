---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Sorting Fundamentals

Sorting rearranges values according to an ordering relation. The immediate result is an ordered sequence, but the more important algorithmic effect is that ordering enables other operations: binary search, merging, duplicate grouping, range queries, and deterministic output.

Sorting is therefore both an algorithm and a data-model decision. A program that sorts once and performs many lookups has a different cost profile from a program that keeps data ordered after every insertion.

## The Comparator Contract

A comparator defines the ordering relation. For two values `a` and `b`, it should report whether `a` is less than, equal to, or greater than `b`.

A reliable comparator is:

- deterministic: the same pair produces the same result
- total for the values being sorted: every pair is ordered or declared equivalent
- antisymmetric: if `a < b`, then `b` is not less than `a`
- transitive: if `a < b` and `b < c`, then `a < c`
- side-effect free: comparing values does not mutate the collection

Do not implement an integer comparator as `return a - b`. That expression can overflow and reverse the result. Compare with relational operators instead.

## Sorting Properties

Stable:
: Equal-key records retain their original relative order.

Unstable:
: Equal-key records may be reordered.

In-place:
: The algorithm uses only a small amount of storage beyond the input array.

Out-of-place:
: The algorithm uses a separate array or other proportional storage.

Adaptive:
: The algorithm can take less work when the input is already partly ordered.

Comparison sort:
: Ordering is learned through comparisons between elements.

The required property belongs in the problem contract. A stable sort may be necessary when a secondary ordering was established earlier. An in-place sort may be necessary when temporary memory is unavailable.

## Basic Algorithms

| Algorithm | Typical time | Extra space | Stable | Useful for |
| --- | --- | --- | --- | --- |
| insertion sort | O(n²), O(n) nearly sorted | O(1) | yes | small arrays and incremental order |
| selection sort | O(n²) | O(1) | usually no | minimizing writes in tiny arrays |
| bubble sort | O(n²) | O(1) | yes with adjacent swaps | teaching exchange and early-exit behavior |
| merge sort | O(n log n) | O(n) | yes | predictable comparison count and stable records |
| quicksort variants | average O(n log n) | implementation-dependent | usually no | general-purpose in-memory sorting with careful bounds |

The table gives typical behavior, not a license to ignore worst cases. For embedded work, a simple O(n²) algorithm can be the right choice when `n` is a small documented bound and it avoids recursion or temporary allocation.

## Insertion Sort Invariant

Insertion sort maintains this invariant:

> Before iteration `i`, the prefix `[0, i)` is sorted and contains exactly the original values from that prefix.

The next value is saved, larger values are shifted right, and the saved value is inserted into its ordered position. The shift preserves stability because equal values are not moved past one another.

## Programming Examples

### C: Stable Insertion Sort

This implementation sorts signed integers in ascending order. It uses a checked comparator and no allocation.

```c
#include <stddef.h>

enum sort_status {
    SORT_OK = 0,
    SORT_ERR_NULL
};

static int compare_ints(int left, int right)
{
    if (left < right)
        return -1;
    if (left > right)
        return 1;
    return 0;
}

enum sort_status insertion_sort_ints(int *values, size_t count)
{
    if (values == NULL && count > 0)
        return SORT_ERR_NULL;

    for (size_t i = 1; i < count; i++) {
        int value = values[i];
        size_t position = i;

        while (position > 0 &&
               compare_ints(value, values[position - 1]) < 0) {
            values[position] = values[position - 1];
            position--;
        }

        values[position] = value;
    }

    return SORT_OK;
}
```

The algorithm is stable because the loop shifts only values strictly greater than `value`. Its worst-case time is O(n²), best-case time is O(n) for already sorted data, and extra space is O(1).

### C: Sorting Records By Key

Sorting records makes stability observable. The record identifier is not part of the primary comparison, so equal keys retain their input order.

```c
#include <stddef.h>

struct record {
    int key;
    unsigned int id;
};

static int record_key_less(const struct record *left,
                           const struct record *right)
{
    return left->key < right->key;
}

void insertion_sort_records(struct record *records, size_t count)
{
    if (records == NULL)
        return;

    for (size_t i = 1; i < count; i++) {
        struct record value = records[i];
        size_t position = i;

        while (position > 0 &&
               record_key_less(&value, &records[position - 1])) {
            records[position] = records[position - 1];
            position--;
        }
        records[position] = value;
    }
}
```

For input `(key=2,id=10), (key=1,id=20), (key=2,id=30)`, the result is `(1,20), (2,10), (2,30)`. An unstable algorithm may legally return the two key-2 records in the other order.

### Python: Reference Sorting Properties

```python
def insertion_sort(values):
    result = list(values)
    for index in range(1, len(result)):
        value = result[index]
        position = index
        while position > 0 and value < result[position - 1]:
            result[position] = result[position - 1]
            position -= 1
        result[position] = value
    return result


records = [(2, "first"), (1, "middle"), (2, "last")]
ordered = sorted(records, key=lambda record: record[0])
assert ordered == [(1, "middle"), (2, "first"), (2, "last")]
```

Python's built-in sort is stable, making it useful for checking the expected order of records with duplicate keys.

## Merge-Style Thinking

Merge sort divides the input into smaller ranges, sorts each range, and merges two sorted ranges. The merge operation is linear in the combined range and is often more important than the recursive division.

The merge invariant is:

> At each output position, the output prefix contains the smallest remaining values from the two input ranges and is sorted.

Merge sort provides O(n log n) time and stable ordering, but the usual implementation needs O(n) temporary storage. A systems implementation must decide whether that storage is caller-owned, pooled, reused across calls, or unavailable.

## Correctness Questions

When reviewing a sort, ask:

- Is the comparator a consistent ordering relation?
- Does the result contain exactly the original elements?
- Is the requested stable/unstable behavior documented?
- Are empty and one-element inputs handled?
- Can indexes or temporary-size calculations overflow?
- Does the algorithm mutate the caller's array or return a copy?
- Is worst-case time acceptable for the maximum input size?

## Common Mistakes

- Returning `left - right` from a comparator for values that may span the integer range.
- Calling a sort stable without testing equal-key records.
- Forgetting that an in-place sort changes the caller's input.
- Using recursion or a temporary buffer without including it in the resource budget.
- Choosing an algorithm by average time while ignoring a deadline-sensitive worst case.
- Sorting values but losing the relationship between a key and its record payload.

## Embedded And Systems Angle

- choose simple in-place algorithms for small fixed arrays when clarity and bounds matter
- use stable sorting when earlier ordering carries meaning
- keep comparators total, deterministic, and side-effect free
- account for temporary buffers, stack depth, movement cost, and write wear
- make maximum element count part of the API or data model

## Related Topics

- [Sorting And Ordering](index.md)
- [Maintaining Sorted Data](maintaining-sorted-data.md)
- [Priority And Partial Ordering](priority-and-partial-ordering.md)
- [Binary Search](../basic-algorithm-schemes/binary-search.md)
- [Divide And Conquer](../control-flow-and-recursion/divide-and-conquer.md)
