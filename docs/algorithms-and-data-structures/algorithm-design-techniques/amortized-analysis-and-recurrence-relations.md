---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Amortized Analysis And Recurrence Relations

Amortized analysis bounds the total cost of a sequence of operations, even when some individual operations are expensive. Recurrence relations describe how recursive or divide-and-conquer work grows. Both tools explain behavior that a single-operation average can hide.

An amortized bound is not a probability statement and is not the same as a worst-case bound for every operation. A dynamic array can have O(1) amortized append while one append performs O(n) copying.

## Three Amortized Methods

### Aggregate Method

Calculate the total cost of a sequence of `m` operations and divide by `m`. If a dynamic array doubles, the total number of copied elements across `m` appends is less than `m` plus the initial capacity, so the sequence cost is O(m).

### Accounting Method

Charge some operations more than their immediate cost and store the difference as credit. A cheap append can pay for the future copy of that element. The proof must show that credit never becomes negative.

### Potential Method

Define a non-negative potential `Phi(state)` representing prepaid work. For operation `i`:

```text
amortized_cost(i) = actual_cost(i) + Phi(after) - Phi(before)
```

Summing over a sequence telescopes the potential terms. If the initial potential is zero or bounded and the final potential is non-negative, the sum of amortized costs bounds the actual total cost.

## Dynamic Array Example

Suppose a vector doubles capacity when full. Let the potential be proportional to the number of occupied slots beyond half capacity. Ordinary appends increase potential by enough to pay for their future movement. A resize spends the saved potential while copying the old elements.

The growth factor affects constants and unused memory:

| Growth policy | Copy behavior | Unused capacity |
| --- | --- | --- |
| fixed increment | repeated O(n²) total copying | low after each growth |
| doubling | O(n) total copying over n appends | up to roughly half capacity |
| factor near one | more frequent copying | less slack |

In embedded code, a fixed-capacity buffer may be preferable even when dynamic growth has a good amortized bound. Allocation, copying, and cache disruption still happen at a particular operation.

## Stack And Queue Sequences

The classic two-stack queue has O(1) amortized enqueue and dequeue. An item moves from the input stack to the output stack at most once. The expensive transfer is charged to the enqueue that placed the item in the input stack.

Similarly, a stack operation that pops many elements can be O(n) in isolation but O(1) amortized if each element can be popped only once after being pushed. The analysis depends on the allowed operation sequence; it does not make a single large pop constant time.

## Fixed-Capacity C Example

```c
#include <stddef.h>

enum {
    AMORTIZED_STACK_CAPACITY = 16
};

enum stack_status {
    AMORTIZED_STACK_OK = 0,
    AMORTIZED_STACK_EMPTY,
    AMORTIZED_STACK_FULL,
    AMORTIZED_STACK_ERR_NULL
};

struct bounded_stack {
    int values[AMORTIZED_STACK_CAPACITY];
    size_t count;
};

enum stack_status bounded_push(struct bounded_stack *stack, int value)
{
    if (stack == NULL)
        return AMORTIZED_STACK_ERR_NULL;
    if (stack->count == AMORTIZED_STACK_CAPACITY)
        return AMORTIZED_STACK_FULL;
    stack->values[stack->count++] = value;
    return AMORTIZED_STACK_OK;
}

enum stack_status bounded_pop(struct bounded_stack *stack, int *out_value)
{
    if (stack == NULL || out_value == NULL)
        return AMORTIZED_STACK_ERR_NULL;
    if (stack->count == 0)
        return AMORTIZED_STACK_EMPTY;
    *out_value = stack->values[--stack->count];
    return AMORTIZED_STACK_OK;
}

size_t bounded_pop_many(struct bounded_stack *stack,
                        size_t maximum,
                        int *output,
                        size_t output_capacity)
{
    size_t written = 0;

    if (stack == NULL || (output == NULL && output_capacity > 0))
        return 0;
    while (written < maximum && written < output_capacity &&
           stack->count > 0)
        output[written++] = stack->values[--stack->count];
    return written;
}
```

The `pop_many` operation is bounded by both the caller's maximum and output capacity. Its total cost across a sequence is proportional to the number of values removed, but callers that require a per-call deadline must also bound `maximum`.

## Recurrence Relations

Write a recurrence from the actual control flow. If an algorithm makes two recursive calls on halves and performs linear combine work:

```text
T(n) = 2T(n / 2) + c n
```

The recursion tree has O(log n) levels, and each level does O(n) total combine work, giving O(n log n). If only one half is recursed into and the partition scan is linear:

```text
T(n) = T(n / 2) + c n = O(n)
```

Do not infer a recurrence from a function name. Account for loops, copies, comparisons, allocations, and failed branches in the implementation.

## Recursion Depth And Stack

A recurrence can have good time complexity while using too much call stack. Track the maximum depth separately. Tail recursion may not be eliminated in C, and a balanced recursion can still exceed a small embedded stack if input limits are not enforced.

For a bounded maximum, an iterative version or an explicit caller-owned work stack often gives a clearer memory contract.

## Amortized Versus Real-Time Guarantees

Amortized O(1) means that a sequence has a bounded total cost proportional to its length. It does not guarantee:

- no allocation
- no long individual operation
- no cache miss or lock wait
- no interrupt latency spike
- no deadline miss

If each operation has a deadline, combine amortized reasoning with an individual work cap, preallocation, incremental copying, or a different data structure.

## Python: Operation Accounting

```python
def dynamic_array_copy_cost(capacity, appends):
    """Return total element copies for a doubling array."""
    if capacity <= 0:
        raise ValueError("capacity must be positive")
    size = 0
    copies = 0
    for _ in range(appends):
        if size == capacity:
            copies += size
            capacity *= 2
        size += 1
    return copies


def two_stack_queue_cost(items):
    input_stack = list(items)
    output_stack = []
    moves = 0
    while input_stack:
        output_stack.append(input_stack.pop())
        moves += 1
    return moves
```

The functions expose the accounting rather than hiding it behind a library container. Tests can check that doubling performs fewer than a linear number of copies over a long sequence, while still testing the individual resize operation separately.

## Common Mistakes

- Calling an average operation cost an amortized proof without bounding sequences.
- Forgetting the initial or final potential term.
- Allowing stored accounting credit to become negative.
- Ignoring copies, allocation, or destructor work in a growth recurrence.
- Treating O(1) amortized as O(1) worst-case latency.
- Measuring only the mean and missing resize or rebalancing spikes.
- Forgetting recursion depth and stack bytes.

## Embedded And Systems Angle

- use amortized analysis to understand queues and buffers, not to waive deadline analysis
- prefer fixed-capacity or incremental-growth designs on critical paths
- include cache traffic, DMA visibility, locks, and allocation in the cost model
- derive explicit maximum operation counts from bounded inputs
- keep expensive rebalancing or copying outside interrupt context
- record the worst individual operation as well as sequence-wide totals

## Review Checklist

- What sequence is being bounded?
- What is the actual cost of a cheap and an expensive operation?
- What credit or potential pays for deferred work?
- Is the potential non-negative and correctly initialized?
- What recurrence describes each recursive branch and combine step?
- Are stack depth, allocation, and deadline behavior analyzed separately?

## Related Topics

- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
- [Control Flow And Recursion](../control-flow-and-recursion/index.md)
- [Arrays Buffers And Records](../data-structures-for-algorithms/arrays-buffers-and-records.md)
- [Ring Buffers](../data-structures-for-algorithms/ring-buffers.md)
- [Deterministic Runtime And Real-Time Tradeoffs](../embedded-linux-algorithmic-constraints/deterministic-runtime-and-real-time-tradeoffs.md)
