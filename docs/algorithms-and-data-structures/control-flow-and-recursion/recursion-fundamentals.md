---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Recursion Fundamentals

Recursion solves a problem by reducing it to a smaller version of the same problem. A recursive algorithm needs a base case, a recursive case, and a guarantee that the recursive calls move toward the base case.

Recursion is often the clearest way to explain tree traversal, divide-and-conquer, and backtracking. In production systems code, it also requires stack-depth discipline.

## Recursive Shape

Every recursive function should make these parts visible:

- base case
- recursive case
- smaller input
- result combination
- maximum or expected depth

Example shape:

```c
result solve(problem)
{
    if (problem_is_small_enough(problem))
        return direct_answer(problem);

    return combine(solve(smaller_problem(problem)));
}
```

If the "smaller problem" is not actually smaller, recursion may never terminate.

## Base Cases

A base case handles the smallest problem directly.

Examples:

- empty list
- one-element range
- null tree node
- zero remaining choices
- maximum depth reached

Most recursive bugs come from missing or wrong base cases.

## Recursive Cases

The recursive case must make progress. That usually means:

- reducing a count
- moving to a child node
- splitting a range into smaller ranges
- consuming one token
- adding one decision to a partial solution

Progress should be stated as part of the algorithm, not assumed.

## The Call Stack

Each recursive call needs stack space for:

- return address
- parameters
- local variables
- saved registers and ABI bookkeeping

The exact size depends on compiler, architecture, optimization, and function body. The safe algorithmic question is simpler:

> What is the maximum number of active recursive calls?

If that maximum is not bounded by the problem model, do not use recursion in constrained C code.

## Example: Sum A Range Recursively

Problem:

> Sum `count` readings.

Recursive model:

- Base case: `count == 0`, sum is `0`.
- Recursive case: first element plus sum of the remaining `count - 1` elements.
- Depth: exactly `count + 1` calls.

This is a good teaching example and a poor production implementation for large arrays. A loop is simpler and uses constant stack space.

## Programming Examples

### C: Recursive Sum With A Depth Limit

This example is intentionally defensive. It demonstrates recursion mechanics while enforcing a maximum depth.

```c
#include <stddef.h>
#include <stdint.h>

enum recursive_sum_status {
    RECURSIVE_SUM_OK = 0,
    RECURSIVE_SUM_ERR_NULL,
    RECURSIVE_SUM_ERR_DEPTH,
    RECURSIVE_SUM_ERR_OVERFLOW
};

static int add_overflows_i64(int64_t a, int64_t b)
{
    if (b > 0 && a > INT64_MAX - b)
        return 1;
    if (b < 0 && a < INT64_MIN - b)
        return 1;
    return 0;
}

static enum recursive_sum_status sum_recursive_impl(const int *values,
                                                    size_t count,
                                                    size_t depth_left,
                                                    int64_t *out_sum)
{
    int64_t rest;

    if (count == 0) {
        *out_sum = 0;
        return RECURSIVE_SUM_OK;
    }
    if (depth_left == 0)
        return RECURSIVE_SUM_ERR_DEPTH;

    enum recursive_sum_status status =
        sum_recursive_impl(values + 1, count - 1, depth_left - 1, &rest);
    if (status != RECURSIVE_SUM_OK)
        return status;

    if (add_overflows_i64((int64_t)values[0], rest))
        return RECURSIVE_SUM_ERR_OVERFLOW;

    *out_sum = (int64_t)values[0] + rest;
    return RECURSIVE_SUM_OK;
}

enum recursive_sum_status sum_recursive(const int *values,
                                        size_t count,
                                        size_t max_depth,
                                        int64_t *out_sum)
{
    if (out_sum == NULL)
        return RECURSIVE_SUM_ERR_NULL;
    if (values == NULL && count > 0)
        return RECURSIVE_SUM_ERR_NULL;

    return sum_recursive_impl(values, count, max_depth, out_sum);
}
```

This recursive version has O(n) runtime and O(n) stack depth. The `max_depth` argument makes that cost explicit.

### C: Iterative Equivalent

The iterative version is usually the better systems implementation.

```c
#include <stddef.h>
#include <stdint.h>

int sum_iterative(const int *values, size_t count, int64_t *out_sum)
{
    int64_t sum = 0;

    if (out_sum == NULL)
        return -1;
    if (values == NULL && count > 0)
        return -1;

    for (size_t i = 0; i < count; i++)
        sum += values[i];

    *out_sum = sum;
    return 0;
}
```

Both algorithms compute the same mathematical result. The iterative one uses O(1) stack space.

### Python: Recursive Teaching Model

```python
def sum_recursive(values):
    if not values:
        return 0
    return values[0] + sum_recursive(values[1:])


def sum_iterative(values):
    total = 0
    for value in values:
        total += value
    return total
```

This Python version is useful for showing the idea. It is not efficient for large lists because slicing creates new lists.

## Recursion Vs Iteration

Use recursion when:

- the problem is naturally nested
- the maximum depth is small and documented
- recursive code is much clearer than manual stack management

Use iteration when:

- the problem is a linear scan
- maximum depth depends on input data
- the call stack is small
- worst-case behavior must be obvious
- the code runs in a kernel, driver, interrupt-adjacent, or constrained thread context

## Explicit Stack Pattern

Recursive depth-first algorithms can often be rewritten with an explicit stack. The explicit stack has a capacity that can be checked and tested.

```c
struct frame {
    size_t index;
};

/* A real implementation would define what index means for the problem.
 * The important point is that stack capacity is now data, not call-stack risk.
 */
```

This pattern appears in depth-first graph search, tree traversal, and backtracking.

## Tail Recursion

A tail-recursive function returns the result of the recursive call directly.

```c
return helper(next_state);
```

Some compilers can optimize this into a loop in some cases, but C does not guarantee that optimization. Do not rely on tail recursion to make stack use safe.

## Common Mistakes

- Missing the base case.
- Making a recursive call with the same input.
- Assuming recursion depth is small without proving it.
- Using recursion for simple array scans in constrained code.
- Relying on tail-call optimization in C.
- Allocating large local objects in recursive functions.
- Handling invalid input only after the first recursive call.

## Embedded And Systems Angle

- set recursion policy according to stack limits and worst-case depth
- prefer explicit stacks where depth is data-dependent and unbounded
- document depth assumptions near the algorithm
- keep recursive stack frames small when recursion is deliberately allowed

## Related Topics

- [Control Flow And Recursion](index.md)
- [Recursion And Stack-Depth Policy](../embedded-linux-algorithmic-constraints/recursion-and-stack-depth-policy.md)
- [Depth-First Search](../graph-algorithms/depth-first-search.md)
