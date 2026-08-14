---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Greedy Algorithms

Greedy algorithms build a solution by repeatedly making a locally best choice and never revisiting an accepted choice. They are attractive because the implementation is often short and the working state is small. Their danger is equally simple: a locally attractive choice is not automatically part of a globally optimal solution.

## Greedy Choice And Optimal Substructure

Two properties commonly support a greedy proof:

- **Greedy-choice property:** there is an optimal solution whose first choice is the locally selected choice.
- **Optimal substructure:** after that choice, the remaining problem is itself optimally solved by the same method or a related smaller problem.

Optimal substructure alone is not enough. Weighted interval scheduling has optimal substructure, but choosing the interval with the earliest finish time can discard a more valuable combination. Dynamic programming is needed because the best choice depends on the value of the subproblem.

## A Greedy Proof

A practical proof template is:

1. Define the candidate set and the local ordering.
2. Select the candidate that the algorithm would accept first.
3. Take an arbitrary optimal solution.
4. If it already contains the candidate, continue with the remainder.
5. Otherwise, exchange the candidate with an element of the optimal solution.
6. Show that feasibility is preserved and the objective does not get worse.
7. Apply the argument repeatedly to the remaining problem.

The exchange step is the part that cannot be replaced by intuition. If replacing an optimal element can reduce value, violate a deadline, or make future choices impossible, the greedy rule is not proved.

## Invariant For A Partial Solution

For interval scheduling, a useful invariant is:

> After each selection, the selected intervals are mutually compatible and the finish time of the last selected interval is as small as possible among solutions with the same number of intervals.

The small finish time leaves the largest possible suffix of the timeline for future intervals. This is why earliest finish time is safe for the unweighted maximum-cardinality problem.

## Interval Scheduling

Given intervals `[start, finish)`, select the largest set of pairwise non-overlapping intervals. Sort by increasing finish time, accept an interval when `start >= last_finish`, and update `last_finish`.

The half-open convention matters. An interval ending at time 10 and one starting at time 10 are compatible. If the application treats endpoints as occupied, use the corresponding strict comparison and document it.

Complexity is O(n log n) for sorting and O(n) for selection. If intervals arrive already ordered by finish time, the scan is O(n).

### C: Bounded Interval Scheduling

```c
#include <stddef.h>

enum interval_status {
    INTERVAL_OK = 0,
    INTERVAL_ERR_NULL,
    INTERVAL_ERR_CAPACITY,
    INTERVAL_ERR_ORDER
};

struct interval {
    int start;
    int finish;
    size_t original_index;
};

/* The caller supplies intervals sorted by finish, then original_index. */
enum interval_status select_intervals(const struct interval *intervals,
                                      size_t count,
                                      size_t *selected,
                                      size_t selected_capacity,
                                      size_t *out_count)
{
    size_t written = 0;
    int last_finish = 0;
    int have_previous = 0;

    if (out_count == NULL)
        return INTERVAL_ERR_NULL;
    if (intervals == NULL && count > 0)
        return INTERVAL_ERR_NULL;
    if (selected == NULL && selected_capacity > 0)
        return INTERVAL_ERR_NULL;

    for (size_t i = 0; i < count; i++) {
        const struct interval *candidate = &intervals[i];

        if (candidate->start > candidate->finish)
            return INTERVAL_ERR_ORDER;
        if (have_previous && candidate->finish < last_finish)
            return INTERVAL_ERR_ORDER;
        if (have_previous && candidate->start < last_finish)
            continue;
        if (written == selected_capacity)
            return INTERVAL_ERR_CAPACITY;

        selected[written++] = candidate->original_index;
        last_finish = candidate->finish;
        have_previous = 1;
    }

    *out_count = written;
    return INTERVAL_OK;
}
```

The function intentionally does not sort. Sorting may allocate or mutate caller-owned data, so it is a separate operation with its own capacity and tie policy. A production interface can provide a bounded sort or require the caller to maintain the ordering precondition.

## Resource And Deadline Selection

Many scheduling problems use the same idea but have a different proof. For unit-time jobs with deadlines and rewards, process jobs by increasing deadline, tentatively accept each job, and remove the accepted job with the smallest reward when the set no longer fits. A heap is useful for the selected set.

This rule is valid only for the stated constraints. Variable durations, release times, setup costs, and precedence constraints change the problem. Do not transfer the interval-scheduling proof to a different scheduling objective.

## Coin Change: A Useful Counterexample

For denominations `{25, 10, 5, 1}`, repeatedly taking the largest denomination produces an optimal number of coins for every amount. That property is not true for arbitrary denominations. With `{1, 3, 4}` and amount `6`, greedy chooses `4 + 1 + 1` (three coins), while the optimum is `3 + 3` (two coins).

The counterexample identifies the missing proof property. If the denomination system is not known to be canonical, use dynamic programming or another method that can compare combinations.

## When Greedy Fails

Common warning signs include:

- a choice changes several future constraints at once
- the objective combines unrelated values or penalties
- a locally best item consumes a scarce resource needed by a later combination
- the problem asks for an exact count or sum rather than a feasible solution
- a small brute-force search quickly finds a counterexample

Greedy heuristics can still be useful as lower bounds, initial solutions, or bounded best-effort policies. Label the result correctly: “feasible,” “heuristic,” and “proven optimal” are different contracts.

## Deterministic Tie-Breaking

Equal local scores need a stable policy. Examples include:

- earliest finish, then earliest start, then original index
- equal reward, then lower resource use
- equal priority, then insertion sequence

Deterministic ties make logs reproducible and prevent behavior from depending on an incidental sort implementation or heap layout.

## Python: Reference And Exhaustive Check

```python
from itertools import combinations


def greedy_interval_count(intervals):
    ordered = sorted(intervals, key=lambda item: (item[1], item[0]))
    last_finish = None
    selected = []
    for start, finish in ordered:
        if start > finish:
            raise ValueError("finish must not precede start")
        if last_finish is None or start >= last_finish:
            selected.append((start, finish))
            last_finish = finish
    return selected


def exhaustive_interval_count(intervals):
    best = []
    for size in range(len(intervals) + 1):
        for candidate in combinations(intervals, size):
            ordered = sorted(candidate)
            if all(left[1] <= right[0]
                   for left, right in zip(ordered, ordered[1:])):
                if len(candidate) > len(best):
                    best = list(candidate)
    return best
```

For small generated inputs, compare only the objective value, not the exact selected set; multiple optimal sets may exist. The exhaustive function is intentionally slow and should remain a test oracle, not production code.

## Common Mistakes

- Calling a rule greedy without stating its objective and constraints.
- Using earliest start instead of earliest finish for unweighted interval scheduling.
- Assuming an optimal-substructure problem is automatically greedily solvable.
- Sorting without a deterministic secondary key.
- Mutating input while the API presents it as read-only.
- Returning a partial schedule as optimal after capacity exhaustion.
- Ignoring integer overflow in accumulated cost or reward.

## Embedded And Systems Angle

- pre-sort bounded candidate sets outside deadline-sensitive paths when possible
- state whether a result is optimal, feasible, or best effort
- use fixed-capacity heaps and arrays for bounded scheduling
- make tie-breaking deterministic for reproducible diagnostics
- reject unsupported constraints instead of silently applying a familiar greedy rule
- keep fallback to dynamic programming or a safe degraded policy explicit

## Review Checklist

- What is the local choice?
- What exchange or cut argument makes it safe?
- What is the partial-solution invariant?
- Are intervals open, closed, or half-open at their endpoints?
- Are the input ordering and objective preconditions checked?
- Does a tiny exhaustive oracle find counterexamples?
- Are capacity, overflow, and tie behavior visible to the caller?

## Related Topics

- [Dynamic Programming](dynamic-programming.md)
- [Searching And Backtracking](../searching-and-backtracking/index.md)
- [Sorting Fundamentals](../sorting-and-ordering/sorting-fundamentals.md)
- [Priority And Partial Ordering](../sorting-and-ordering/priority-and-partial-ordering.md)
- [Heaps And Priority Queues](../data-structures-for-algorithms/heaps-and-priority-queues.md)
