---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Divide And Conquer

Divide and conquer solves a problem by splitting it into smaller subproblems, solving those subproblems, and combining the results.

This pattern is behind binary search, merge sort, quicksort-style partitioning, tree algorithms, and many parallel algorithms. The key design questions are where to split, when to stop, how to combine, and what extra memory is required.

## The Basic Shape

A divide-and-conquer algorithm has:

- a base case
- a divide step
- one or more subproblem solves
- a combine step

Generic shape:

```text
solve(problem):
    if problem is small enough:
        solve directly
    split problem into subproblems
    solve each subproblem
    combine subproblem results
```

## Subproblem Boundaries

Most array-based divide-and-conquer algorithms work best with half-open ranges:

```text
[lo, hi)
```

This means:

- `lo` is included
- `hi` is excluded
- the number of elements is `hi - lo`
- the empty range is `lo == hi`
- the one-element range is `hi - lo == 1`

Half-open ranges make splitting and termination easier to reason about.

## Base Cases

Base cases prevent infinite recursion and define what "small enough" means.

Common base cases:

- empty range
- one-element range
- range below a threshold
- maximum recursion depth reached
- subproblem no longer contains the target

In systems code, "small enough" may also mean "small enough to use a simple loop."

## Combining Results

The combine step depends on the problem:

- sum: add sub-sums
- minimum: take the smaller minimum
- search: choose the found result, or report not found
- sort: merge two sorted ranges
- validation: combine status and diagnostics

The combine step is where temporary memory and ordering constraints often appear.

## Cost Intuition

Divide-and-conquer cost depends on:

- how many subproblems are created
- how much smaller each subproblem is
- how expensive the combine step is
- how much temporary memory is needed
- how deep the recursion goes

For example:

- binary search solves one half each step, giving O(log n) comparisons
- merge sort solves both halves and merges them, giving O(n log n) runtime and O(n) temporary storage
- recursive summation solves both halves and combines with addition, giving O(n) runtime but O(log n) call depth if split evenly

## Programming Examples

### C: Divide-And-Conquer Sum With Bounded Depth

This example sums a range by splitting it in half. It is more stack-efficient than the naive recursive sum because depth grows with `log2(n)`, not `n`, but it is still recursive and still needs a depth policy.

```c
#include <stddef.h>
#include <stdint.h>

enum dc_sum_status {
    DC_SUM_OK = 0,
    DC_SUM_ERR_NULL,
    DC_SUM_ERR_DEPTH,
    DC_SUM_ERR_OVERFLOW
};

static int add_overflows_i64(int64_t a, int64_t b)
{
    if (b > 0 && a > INT64_MAX - b)
        return 1;
    if (b < 0 && a < INT64_MIN - b)
        return 1;
    return 0;
}

static enum dc_sum_status sum_range_dc(const int *values,
                                       size_t lo,
                                       size_t hi,
                                       size_t depth_left,
                                       int64_t *out_sum)
{
    size_t mid;
    int64_t left;
    int64_t right;

    if (lo == hi) {
        *out_sum = 0;
        return DC_SUM_OK;
    }

    if (hi - lo == 1) {
        *out_sum = values[lo];
        return DC_SUM_OK;
    }

    if (depth_left == 0)
        return DC_SUM_ERR_DEPTH;

    mid = lo + (hi - lo) / 2;

    enum dc_sum_status status =
        sum_range_dc(values, lo, mid, depth_left - 1, &left);
    if (status != DC_SUM_OK)
        return status;

    status = sum_range_dc(values, mid, hi, depth_left - 1, &right);
    if (status != DC_SUM_OK)
        return status;

    if (add_overflows_i64(left, right))
        return DC_SUM_ERR_OVERFLOW;

    *out_sum = left + right;
    return DC_SUM_OK;
}

enum dc_sum_status sum_divide_and_conquer(const int *values,
                                          size_t count,
                                          size_t max_depth,
                                          int64_t *out_sum)
{
    if (out_sum == NULL)
        return DC_SUM_ERR_NULL;
    if (values == NULL && count > 0)
        return DC_SUM_ERR_NULL;

    return sum_range_dc(values, 0, count, max_depth, out_sum);
}
```

This is not the simplest way to sum an array. It is a learning example for subproblem boundaries, base cases, combine steps, overflow checks, and recursion depth.

### C: Binary Search As Divide And Conquer

Binary search is divide-and-conquer where only one subproblem survives each step.

```c
#include <stddef.h>

int binary_search_int(const int *values,
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

        if (values[mid] == target) {
            *out_index = mid;
            return 0;
        }
        if (values[mid] < target)
            lo = mid + 1;
        else
            hi = mid;
    }

    return 1;
}
```

The range `[lo, hi)` always contains all positions where the target could still be. Each iteration shrinks that range.

### Python: Reference Divide-And-Conquer Sum

```python
def sum_dc(values, lo=0, hi=None):
    if hi is None:
        hi = len(values)
    if lo == hi:
        return 0
    if hi - lo == 1:
        return values[lo]

    mid = lo + (hi - lo) // 2
    return sum_dc(values, lo, mid) + sum_dc(values, mid, hi)
```

This reference implementation is useful for understanding the recursive shape. For large inputs, Python's built-in `sum` or an iterative loop is the practical choice.

## Recursive And Iterative Shapes

Divide-and-conquer is often taught recursively because the structure is direct. Some algorithms can be written iteratively:

- binary search
- bottom-up merge passes
- explicit-stack quicksort
- tree traversal with an explicit stack

Iterative versions make memory bounds more explicit, but they can obscure the conceptual split. It is often useful to understand the recursive model first and then choose the production shape based on constraints.

## Memory And Stack Considerations

Always account for:

- maximum recursion depth
- local variables per recursive call
- temporary buffers for combine steps
- whether the input is copied or referenced
- whether subproblems overlap
- whether output can be written in place

Merge-style algorithms often need temporary storage. Recursive tree or range algorithms need stack space. Parallel divide-and-conquer algorithms need work queues and result storage.

## Common Mistakes

- Splitting into subproblems that are not smaller.
- Mishandling empty and one-element ranges.
- Computing the midpoint as `(lo + hi) / 2`, which can overflow for large indexes.
- Forgetting to combine all required result fields.
- Allocating a temporary buffer at every recursive level without accounting for peak memory.
- Assuming recursive depth is safe because runtime is O(log n).
- Using divide-and-conquer where a simple scan is clearer and fast enough.

## Embedded And Systems Angle

- account for temporary buffers and stack depth
- prefer predictable split sizes when memory is bounded
- watch for cache behavior in recursive layouts
- switch to iterative or bottom-up forms when stack policy forbids recursion
- use simple loops for small fixed-size inputs when they meet the requirement

## Related Topics

- [Recursion Fundamentals](recursion-fundamentals.md)
- [Binary Search](../basic-algorithm-schemes/binary-search.md)
- [Sorting Fundamentals](../sorting-and-ordering/sorting-fundamentals.md)
