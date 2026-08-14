---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Dynamic Programming

Dynamic programming (DP) solves a problem by identifying smaller states whose answers can be reused. It is useful when subproblems overlap and an optimal answer can be composed from answers to smaller states.

The difficult part is state design. A table with too little information produces an incorrect transition; a table with too much information wastes memory and work.

## State, Transition, And Base Case

Describe a state in one sentence. For example:

> `best[i][capacity]` is the maximum value obtainable using the first `i` items with at most `capacity` units available.

Then define:

- **base cases:** states with no remaining choices or empty input
- **transition:** how a state is built from smaller states
- **answer state:** where the requested result lives
- **reconstruction state:** optional information needed to recover decisions

If the state meaning cannot be stated without referring to the implementation's incidental indexes, it is probably not ready.

## Overlapping Subproblems And Optimal Substructure

Memoization starts with a recursive definition and caches each state the first time it is computed. Tabulation fills states in an order that guarantees dependencies are ready. Both compute the same recurrence when the state and base cases match.

DP is different from divide-and-conquer because independent subproblems are not repeatedly recomputed. It is different from greedy algorithms because it retains enough alternatives to compare competing choices.

## Dependency Order

For a one-dimensional recurrence such as:

```text
best[i] = max(best[i - 1], best[i - 2] + value[i])
```

increasing `i` is valid. A two-dimensional table may require row-major order, reverse capacity order, or a topological order over arbitrary states. Derive the fill order from the dependency graph, not from habit.

## 0/1 Knapsack

For each item with weight `w` and value `v`:

```text
best[i][c] = max(
    best[i - 1][c],
    best[i - 1][c - w] + v       when w <= c
)
```

The `i - 1` in the second term prevents reusing the same item. With a one-dimensional table, iterate capacity downward for 0/1 knapsack. Iterating upward changes the algorithm into an unbounded-knapsack variant.

Time is O(nC), and memory is O(nC) for reconstruction or O(C) for value only. `C` must be a real bounded capacity; pseudo-polynomial complexity can become impractical when the numeric capacity is large.

### C: Fixed-Capacity 0/1 Knapsack

```c
#include <limits.h>
#include <stddef.h>

enum {
    KNAPSACK_MAX_CAPACITY = 128,
    KNAPSACK_MAX_ITEMS = 32
};

enum knapsack_status {
    KNAPSACK_OK = 0,
    KNAPSACK_ERR_NULL,
    KNAPSACK_ERR_CAPACITY,
    KNAPSACK_ERR_ITEM,
    KNAPSACK_ERR_OVERFLOW
};

struct knapsack_item {
    size_t weight;
    int value;
};

enum knapsack_status knapsack_best_value(
    const struct knapsack_item *items,
    size_t item_count,
    size_t capacity,
    int *out_value)
{
    int best[KNAPSACK_MAX_CAPACITY + 1] = { 0 };

    if (out_value == NULL)
        return KNAPSACK_ERR_NULL;
    if (items == NULL && item_count > 0)
        return KNAPSACK_ERR_NULL;
    if (item_count > KNAPSACK_MAX_ITEMS ||
        capacity > KNAPSACK_MAX_CAPACITY)
        return KNAPSACK_ERR_CAPACITY;

    for (size_t item = 0; item < item_count; item++) {
        const struct knapsack_item *current = &items[item];

        if (current->weight > capacity)
            continue;
        if (current->weight == 0)
            return KNAPSACK_ERR_ITEM;
        if (current->value < 0)
            return KNAPSACK_ERR_ITEM;

        for (size_t used = capacity;
             used >= current->weight;
             used--) {
            int previous = best[used - current->weight];

            if (current->value > INT_MAX - previous)
                return KNAPSACK_ERR_OVERFLOW;
            if (previous + current->value > best[used])
                best[used] = previous + current->value;
            if (used == current->weight)
                break;
        }
    }

    *out_value = best[capacity];
    return KNAPSACK_OK;
}
```

The downward loop ensures an item contributes at most once. The explicit break avoids unsigned `size_t` wraparound. The example rejects zero-weight items to keep the contract simple; another valid contract could accept them after checking their value once.

## Reconstruction

Value-only DP is often enough for a score, but callers may need the actual choices. Options include:

- retain a decision bit for every state
- retain predecessor indexes
- recompute the transition while walking backward
- divide the problem into checkpoints to reduce storage

Recomputation saves memory but adds work. Decision bits use one bit per state when the transition has only a small number of alternatives. State the tradeoff instead of returning an unexplained list of choices.

## Sequence Matching And Edit Distance

For longest common subsequence, `lcs[i][j]` can mean the best match using prefixes of lengths `i` and `j`. Matching final elements extends the diagonal; otherwise take the better of the neighboring states. Edit distance uses the same grid but assigns costs to insertion, deletion, and substitution.

The full table is O(mn) time and memory. If only the distance is needed, two rows are enough. If the input dimensions are unbounded, impose a maximum, use a banded approximation, or reject the request before allocating a large table.

### Python: Levenshtein Distance With Two Rows

```python
def edit_distance(left, right):
    if len(left) < len(right):
        left, right = right, left

    previous = list(range(len(right) + 1))
    for left_index, left_value in enumerate(left, start=1):
        current = [left_index]
        for right_index, right_value in enumerate(right, start=1):
            insertion = current[right_index - 1] + 1
            deletion = previous[right_index] + 1
            substitution = previous[right_index - 1]
            if left_value != right_value:
                substitution += 1
            current.append(min(insertion, deletion, substitution))
        previous = current
    return previous[-1]
```

The shorter string is kept as the table dimension, reducing auxiliary memory to O(min(m, n)). This version computes the value but cannot reconstruct an edit script without retaining more information.

## Impossible, Zero, And Negative States

Use a distinct impossible sentinel. Zero may be a valid score, and a negative score may be a valid cost. If an impossible state is represented by a large integer, never add to it without first checking that it is finite; otherwise sentinel arithmetic can become a valid-looking result.

## DP Versus Backtracking And Greedy

- Use greedy when a proof shows that a local choice is safe.
- Use DP when many choices lead to the same bounded state and the objective combines them.
- Use backtracking when the answer is a feasible configuration and pruning can avoid most combinations.
- Use a hybrid when DP supplies lower bounds or memoized subproblems inside a search.

The input limits often decide the choice. A table with millions of states may be worse than a carefully bounded search; an exponential search with no strong pruning may be unacceptable even for moderate input.

## Common Mistakes

- Defining a state that omits information needed by future decisions.
- Filling the table in an order that reads uninitialized dependencies.
- Iterating 0/1 knapsack capacity upward and accidentally reusing items.
- Confusing a zero score with an impossible state.
- Forgetting that numeric capacity produces pseudo-polynomial complexity.
- Returning only the score when the caller needs the selected items.
- Using recursion without bounding depth or memo-table size.

## Embedded And Systems Angle

- calculate table bytes, initialization work, and maximum state count before deployment
- use rolling rows or bitsets when reconstruction is not required
- cap input dimensions and return a resource-limit status before allocation
- choose accumulator widths and checked arithmetic explicitly
- keep a simple reference recurrence for testing optimized or packed implementations
- consider a bounded approximate result when an exact table cannot fit the deadline

## Review Checklist

- Can every state be described in one sentence?
- Are base cases and impossible states distinct?
- Does every transition reduce the problem or follow a valid dependency order?
- Is the memory bound stated for both value-only and reconstruction modes?
- Are loop directions preventing accidental reuse?
- Are overflow, capacity, and cancellation policies explicit?
- Has the recurrence been compared with an exhaustive oracle on small inputs?

## Related Topics

- [Greedy Algorithms](greedy-algorithms.md)
- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
- [Arrays Buffers And Records](../data-structures-for-algorithms/arrays-buffers-and-records.md)
- [Practical Sequence Patterns](../basic-algorithm-schemes/practical-sequence-patterns.md)
