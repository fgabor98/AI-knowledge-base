---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Control Flow And Recursion

Control flow is how an algorithm moves from one step to the next. Sequence, branching, iteration, and recursion are the basic shapes used to build every larger algorithm in this topic.

For systems and embedded work, control flow is not just a readability concern. It determines worst-case runtime, stack use, error handling, retry behavior, cancellation points, and how easily the code can be reviewed during failures.

## Learning Goals

After this section, you should be able to:

- describe an algorithm as sequence, branch, loop, or recursion
- choose clear branch structure for validation and error handling
- write loops with explicit bounds and termination conditions
- state loop invariants and progress measures
- recognize when recursion is natural and when it is unsafe
- convert simple recursive algorithms into iterative forms
- explain the shape of divide-and-conquer algorithms
- account for stack depth, temporary buffers, and worst-case behavior

## Core Vocabulary

Sequence:
: Steps executed in a fixed order.

Branch:
: A decision point such as `if`, `else`, `switch`, or an early return.

Iteration:
: Repetition through a loop.

Loop invariant:
: A fact that remains true before and after every loop iteration.

Termination condition:
: The condition that makes repetition stop.

Progress measure:
: A value that moves toward termination, such as an index increasing or a remaining count decreasing.

Recursion:
: A function solving a problem by calling itself on a smaller or simpler problem.

Base case:
: The case a recursive algorithm handles directly without another recursive call.

Recursive case:
: The case that reduces the problem and calls the same algorithm again.

Divide and conquer:
: A strategy that splits a problem into subproblems, solves them, and combines the results.

## Control-Flow Design Order

Use this order when designing a small algorithm:

1. Validate inputs and reject impossible work early.
2. Handle degenerate cases such as empty or one-element input.
3. Establish initial state.
4. Repeat work with a bounded loop or documented recursion.
5. Preserve invariants after each step.
6. Stop when the progress measure reaches the termination condition.
7. Return a result or status with clear postconditions.

This shape keeps error policy separate from core algorithm logic. It also makes the normal path easier to inspect.

## Branching For Validation

Branching is often clearest when validation happens before mutation.

```c
if (out_value == NULL)
    return -1;
if (readings == NULL && count > 0)
    return -1;
if (count == 0)
    return -1;
```

After these checks, the main algorithm can assume the basic preconditions hold. That reduces repeated defensive checks inside the loop.

## Iteration For Bounded Work

Most systems algorithms are built around bounded loops:

- scan an array
- copy a buffer
- validate a packet
- compact a table
- search fixed-size state
- drain at most N queue entries

A good loop makes three things visible:

- start state
- progress on each iteration
- termination condition

Example shape:

```c
for (size_t i = 0; i < count; i++) {
    /* process item i */
}
```

Here `i` is the progress measure and `i < count` is the termination condition.

## Recursion For Self-Similar Problems

Recursion is useful when the problem is naturally nested or self-similar:

- walking a tree
- parsing nested expressions
- solving a smaller range of an array
- exploring choices in backtracking

But recursion uses stack space for each active call. In embedded, kernel-adjacent, and hardening-sensitive code, recursive depth must be bounded by the problem model or replaced with an explicit stack.

## Programming Examples

### C: Control Flow For Validated Scan

This example validates readings and counts how many exceed a threshold. The control flow separates input validation, loop state, branch policy, and output.

```c
#include <stddef.h>

enum count_status {
    COUNT_OK = 0,
    COUNT_ERR_NULL,
    COUNT_ERR_RANGE
};

enum {
    TEMP_MIN_TENTHS_C = -400,
    TEMP_MAX_TENTHS_C = 1250
};

enum count_status count_above_threshold(const int *readings,
                                        size_t count,
                                        int threshold,
                                        size_t *out_count)
{
    size_t matches = 0;

    if (out_count == NULL)
        return COUNT_ERR_NULL;
    if (readings == NULL && count > 0)
        return COUNT_ERR_NULL;
    if (threshold < TEMP_MIN_TENTHS_C || threshold > TEMP_MAX_TENTHS_C)
        return COUNT_ERR_RANGE;

    for (size_t i = 0; i < count; i++) {
        int value = readings[i];

        if (value < TEMP_MIN_TENTHS_C || value > TEMP_MAX_TENTHS_C)
            return COUNT_ERR_RANGE;
        if (value > threshold)
            matches++;
    }

    *out_count = matches;
    return COUNT_OK;
}
```

The loop invariant is: after processing `i` elements, `matches` equals the number of valid processed readings greater than `threshold`.

### Python: Reference Behavior

Python can provide a compact behavior model for tests.

```python
TEMP_MIN_TENTHS_C = -400
TEMP_MAX_TENTHS_C = 1250


def count_above_threshold(readings, threshold):
    if readings is None:
        raise ValueError("readings must not be None")
    if threshold < TEMP_MIN_TENTHS_C or threshold > TEMP_MAX_TENTHS_C:
        raise ValueError("threshold out of range")

    matches = 0
    for value in readings:
        if value < TEMP_MIN_TENTHS_C or value > TEMP_MAX_TENTHS_C:
            raise ValueError("reading out of range")
        if value > threshold:
            matches += 1
    return matches
```

Use this as a reference model, not as proof that the C implementation handles pointer, capacity, and integer-size concerns.

## Choosing Iteration Or Recursion

Prefer iteration when:

- input size can be large or attacker-controlled
- stack use must be predictable
- the algorithm is naturally a scan
- failure or cancellation must be checked often
- the target coding standard discourages recursion

Use recursion when:

- the problem is naturally nested
- maximum depth is small and documented
- recursive code is significantly clearer
- the platform stack budget can support the worst case

When in doubt for embedded work, write the recursive version as a teaching model and the iterative version as the production implementation.

## Common Mistakes

- Hiding failure policy inside the middle of the algorithm.
- Writing loops without a visible progress measure.
- Assuming `while (true)` is safe because it usually exits.
- Using recursion without a depth bound.
- Mixing validation, mutation, and output writes in a way that leaves partial state after failure.
- Treating tail recursion as stack-safe in C. The C language does not guarantee tail-call optimization.

## Embedded And Systems Angle

- understand stack-depth risk before using recursion
- prefer bounded iteration where stack use must be predictable
- use explicit stacks when recursion is unsafe but depth-first behavior is needed
- document termination conditions in low-level loops
- design long loops with cancellation, watchdog, or chunking policy when needed

## Pages In This Section

- [Loop Invariants And Termination](loop-invariants-and-termination.md)
- [Recursion Fundamentals](recursion-fundamentals.md)
- [Divide And Conquer](divide-and-conquer.md)

## Related Topics

- [Basic Algorithm Schemes](../basic-algorithm-schemes/index.md)
- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Embedded Linux Algorithmic Constraints](../embedded-linux-algorithmic-constraints/index.md)
