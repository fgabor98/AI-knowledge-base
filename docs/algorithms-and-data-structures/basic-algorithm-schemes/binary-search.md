---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Binary Search

Binary search finds a target in sorted data by repeatedly discarding half of the remaining search range. It is a compact algorithm, but it is also a common source of off-by-one bugs, overflow bugs, and unclear duplicate-key behavior.

Use binary search when ordered storage is part of the data model or when sorting/building ordered data is justified by repeated lookup.

## Preconditions

Binary search requires:

- data is sorted
- the search comparison uses the same ordering as the sort
- the range bounds are valid
- duplicate-key behavior is documented
- the data is not mutated during the search

If the input is not sorted, binary search can return incorrect not-found results.

## Half-Open Range Model

Use the range:

```text
[lo, hi)
```

This means:

- `lo` is included
- `hi` is excluded
- the number of candidates is `hi - lo`
- the range is empty when `lo == hi`

The loop invariant is:

> If the target exists, it is somewhere in `values[lo]` through `values[hi - 1]`.

Each iteration shrinks that range.

## Midpoint Calculation

Avoid this:

```c
mid = (lo + hi) / 2;
```

If `lo + hi` overflows, the midpoint is wrong.

Prefer:

```c
mid = lo + (hi - lo) / 2;
```

This avoids overflow when `lo <= hi`.

## Exact Match Search

Exact match search returns any matching index. When duplicates exist, this variant does not promise first or last match unless the implementation explicitly does so.

Use it when:

- duplicates are impossible
- any duplicate is acceptable
- the caller only needs existence

## Lower Bound

Lower bound returns the first index where:

```text
values[index] >= target
```

It also returns the insertion point that keeps the array sorted.

Lower bound is often more useful than exact-match search because it gives a deterministic answer for duplicates and absent values.

## Upper Bound

Upper bound returns the first index where:

```text
values[index] > target
```

The range of duplicate values equal to `target` is:

```text
[lower_bound(target), upper_bound(target))
```

## Recursive Binary Search

Recursive binary search is useful for teaching divide-and-conquer. Iterative binary search is usually preferred in C systems code because it uses O(1) stack space and is easy to bound.

## Programming Examples

### C: Exact-Match Binary Search

```c
#include <stddef.h>

enum binary_find_status {
    BINARY_FIND_OK = 0,
    BINARY_FIND_NOT_FOUND,
    BINARY_FIND_ERR_NULL
};

enum binary_find_status binary_find_int(const int *values,
                                        size_t count,
                                        int target,
                                        size_t *out_index)
{
    size_t lo = 0;
    size_t hi = count;

    if (out_index == NULL)
        return BINARY_FIND_ERR_NULL;
    if (values == NULL && count > 0)
        return BINARY_FIND_ERR_NULL;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if (values[mid] == target) {
            *out_index = mid;
            return BINARY_FIND_OK;
        }
        if (values[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }

    return BINARY_FIND_NOT_FOUND;
}
```

Cost: O(log n) comparisons, O(1) extra memory.

Duplicate policy: returns an arbitrary matching index determined by the search path.

### C: Lower Bound

```c
#include <stddef.h>

int lower_bound_int(const int *values,
                    size_t count,
                    int target,
                    size_t *out_index)
{
    size_t lo = 0;
    size_t hi = count;

    if (out_index == NULL)
        return -1;
    if (values == NULL && count > 0)
        return -1;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if (values[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }

    *out_index = lo;
    return 0;
}
```

Postcondition on success:

- `0 <= *out_index <= count`
- every element before `*out_index` is less than `target`
- if `*out_index < count`, then `values[*out_index] >= target`

### C: Upper Bound

```c
#include <stddef.h>

int upper_bound_int(const int *values,
                    size_t count,
                    int target,
                    size_t *out_index)
{
    size_t lo = 0;
    size_t hi = count;

    if (out_index == NULL)
        return -1;
    if (values == NULL && count > 0)
        return -1;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if (values[mid] <= target)
            lo = mid + 1;
        else
            hi = mid;
    }

    *out_index = lo;
    return 0;
}
```

Postcondition on success:

- every element before `*out_index` is less than or equal to `target`
- if `*out_index < count`, then `values[*out_index] > target`

### C: Duplicate Range

```c
#include <stddef.h>

int equal_range_int(const int *values,
                    size_t count,
                    int target,
                    size_t *out_first,
                    size_t *out_end)
{
    size_t first;
    size_t end;

    if (out_first == NULL || out_end == NULL)
        return -1;
    if (lower_bound_int(values, count, target, &first) != 0)
        return -1;
    if (upper_bound_int(values, count, target, &end) != 0)
        return -1;

    *out_first = first;
    *out_end = end;
    return 0;
}
```

If `out_first == out_end`, the target does not appear. Otherwise, all matching elements are in `[out_first, out_end)`.

### C: Recursive Binary Search

```c
#include <stddef.h>

static int binary_find_recursive_impl(const int *values,
                                      size_t lo,
                                      size_t hi,
                                      int target,
                                      size_t *out_index)
{
    size_t mid;

    if (lo == hi)
        return 1;

    mid = lo + (hi - lo) / 2;

    if (values[mid] == target) {
        *out_index = mid;
        return 0;
    }
    if (values[mid] < target)
        return binary_find_recursive_impl(values, mid + 1, hi, target, out_index);

    return binary_find_recursive_impl(values, lo, mid, target, out_index);
}

int binary_find_recursive(const int *values,
                          size_t count,
                          int target,
                          size_t *out_index)
{
    if (out_index == NULL)
        return -1;
    if (values == NULL && count > 0)
        return -1;

    return binary_find_recursive_impl(values, 0, count, target, out_index);
}
```

This has O(log n) stack depth. The iterative version is usually preferable in C production code.

### Python: Reference Search Variants

```python
def lower_bound(values, target):
    lo = 0
    hi = len(values)
    while lo < hi:
        mid = lo + (hi - lo) // 2
        if values[mid] < target:
            lo = mid + 1
        else:
            hi = mid
    return lo


def upper_bound(values, target):
    lo = 0
    hi = len(values)
    while lo < hi:
        mid = lo + (hi - lo) // 2
        if values[mid] <= target:
            lo = mid + 1
        else:
            hi = mid
    return lo


def equal_range(values, target):
    first = lower_bound(values, target)
    end = upper_bound(values, target)
    return first, end
```

These functions are useful for generating expected indexes in tests.

## Correctness Argument

For lower bound:

Initialization:
: `[lo, hi)` starts as the entire array, so the insertion point is inside that range.

Preservation:
: If `values[mid] < target`, the lower bound must be after `mid`, so `lo = mid + 1`. Otherwise, the lower bound is at `mid` or before it, so `hi = mid`.

Termination:
: The range shrinks every iteration. When `lo == hi`, that index is the first position where `target` can be inserted while preserving order.

## Binary Search Checklist

Before using binary search, check:

- Is the input sorted?
- Is the sort order the same as the comparison?
- What happens with duplicates?
- Is the output any match, first match, last match, or insertion point?
- Is the midpoint overflow-safe?
- Does the loop shrink the range every iteration?
- Is empty input handled?
- Is the index type wide enough for `count`?

## Common Mistakes

- Searching unsorted data.
- Using `(lo + hi) / 2`.
- Updating `lo = mid` instead of `lo = mid + 1` when `values[mid] < target`.
- Returning arbitrary duplicate matches when the caller expects the first match.
- Forgetting that insertion point may be equal to `count`.
- Mixing signed and unsigned indexes carelessly.
- Using recursive binary search where stack policy forbids recursion.

## Embedded And Systems Angle

- avoid overflow in midpoint calculations
- document ordering and duplicate-key policy
- use binary search when sorted storage is cheaper than a heavier index
- prefer iterative binary search in C when stack depth must be tightly controlled

## Related Topics

- [Basic Algorithm Schemes](index.md)
- [Loop Invariants And Termination](../control-flow-and-recursion/loop-invariants-and-termination.md)
- [Maintaining Sorted Data](../sorting-and-ordering/maintaining-sorted-data.md)
