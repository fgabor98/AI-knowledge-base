---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Pruning And Search Heuristics

Pruning reduces search work by refusing to explore a branch that cannot produce an acceptable result. A search heuristic orders the remaining choices so that useful results are found earlier or unpromising branches are delayed.

The distinction is important: a correct pruning rule preserves the result set, while a heuristic usually changes only the order. A time limit, approximate bound, or best-effort cutoff can make the overall search incomplete even when the underlying traversal is exhaustive.

## Safe Pruning

A branch may be pruned safely when there is a proof that no required result can occur below it. Common proofs include:

- a hard constraint is already violated
- a resource total already exceeds its limit
- the maximum possible remaining contribution cannot reach the target
- a lower bound is already worse than a known best solution
- a logical dependency makes every descendant invalid
- the logical state has already been explored under an equivalent or better cost

Write the proof next to the pruning condition. If the proof depends on an assumption such as non-negative values, sorted input, or an admissible bound, validate or document that assumption.

## Constraint Propagation

Constraint propagation derives consequences before making more choices. For example, assigning a value to one variable may remove that value from the domain of related variables. This can detect failure earlier than waiting for a complete candidate.

Propagation can change the state substantially, so it needs the same apply/undo discipline as a choice. A useful state invariant says exactly which domains, counters, and flags describe the current partial solution.

## Heuristic Ordering

Ordering does not usually affect correctness when every legal choice is still eventually visited. It can have a large effect on the time to the first solution and on the quality of an early best solution.

Useful ordering ideas include:

- try the most constrained variable first
- try the largest or most informative item first
- try the choice with the lowest estimated remaining cost
- try choices likely to satisfy required features early
- preserve a deterministic tie-breaker for reproducible behavior

Keep ordering separate from validity. A choice should not become invalid merely because it was tried later.

## Feasibility And Optimization

For a feasibility search, the goal is any valid solution. Pruning needs to prove that a branch has no valid solution.

For an optimization search, maintain a best known solution and prune a branch only when its optimistic bound cannot improve that solution. This is branch-and-bound.

Example:

```text
if current_cost + optimistic_remaining_cost >= best_cost:
    prune
```

The bound must be optimistic in the direction that preserves correctness. An underestimate is needed for a minimization problem; an overestimate is needed for a maximization problem.

## Duplicate-State Elimination

If different paths reach the same logical state, a visited set can prevent repeated work. The key must include every property that affects future choices and goal reachability.

For cost-sensitive searches, a simple visited bit is often insufficient. Reaching the same state with a lower cost may dominate an earlier, more expensive visit, while a higher-cost visit can be discarded.

Deduplication is a pruning rule. Its correctness needs a dominance argument, not just the observation that repeated work is slow.

## Running Example: Subset Sum

Given non-negative values and a target, find a subset whose sum equals the target.

A straightforward search chooses include or exclude for each item. Safe pruning rules include:

- stop if the current sum exceeds the target
- stop if the current sum plus every remaining value is still below the target
- stop if all items have been considered

The second rule depends on values being non-negative. With negative values, the remaining-sum bound is not valid without a different bound.

Trying larger values first is a heuristic ordering. It may reach an exact target sooner, but it does not by itself prove that a branch can be discarded.

## Programming Examples

### C: Bounded Subset Search With Safe Pruning

This implementation accepts at most 32 non-negative values. `max_nodes == 0` means no node limit; a nonzero limit makes operational interruption visible through a separate status.

```c
#include <stddef.h>
#include <stdint.h>

enum subset_status {
    SUBSET_FOUND = 0,
    SUBSET_NOT_FOUND,
    SUBSET_LIMIT_REACHED,
    SUBSET_ERR_NULL,
    SUBSET_ERR_COUNT,
    SUBSET_ERR_VALUE
};

struct subset_context {
    const uint32_t *values;
    uint64_t suffix_sum[33];
    size_t count;
    uint64_t target;
    size_t max_nodes;
    size_t nodes;
    int limit_reached;
};

static int subset_visit(struct subset_context *ctx,
                        size_t index,
                        uint64_t sum,
                        uint32_t selected,
                        uint32_t *out_selected)
{
    uint32_t bit;

    if (ctx->max_nodes != 0 && ctx->nodes >= ctx->max_nodes) {
        ctx->limit_reached = 1;
        return 0;
    }
    ctx->nodes++;

    if (sum == ctx->target) {
        *out_selected = selected;
        return 1;
    }
    if (sum > ctx->target || index == ctx->count)
        return 0;
    if (sum + ctx->suffix_sum[index] < ctx->target)
        return 0;

    bit = UINT32_C(1) << index;

    /* Try inclusion first. The order is a heuristic, not a proof. */
    if (ctx->values[index] <= ctx->target - sum &&
        subset_visit(ctx,
                     index + 1,
                     sum + ctx->values[index],
                     selected | bit,
                     out_selected))
        return 1;

    /* Exclusion remains necessary when inclusion did not succeed. */
    return subset_visit(ctx,
                        index + 1,
                        sum,
                        selected,
                        out_selected);
}

enum subset_status subset_sum_find(const uint32_t *values,
                                   size_t count,
                                   uint64_t target,
                                   size_t max_nodes,
                                   uint32_t *out_selected,
                                   size_t *out_nodes)
{
    struct subset_context ctx;

    if (out_selected == NULL || out_nodes == NULL)
        return SUBSET_ERR_NULL;
    if (count > 32)
        return SUBSET_ERR_COUNT;
    if (values == NULL && count > 0)
        return SUBSET_ERR_NULL;

    ctx = (struct subset_context){
        .values = values,
        .count = count,
        .target = target,
        .max_nodes = max_nodes
    };

    ctx.suffix_sum[count] = 0;
    for (size_t i = count; i > 0; i--) {
        uint64_t value = values[i - 1];

        if (UINT64_MAX - ctx.suffix_sum[i] < value)
            return SUBSET_ERR_VALUE;
        ctx.suffix_sum[i - 1] = ctx.suffix_sum[i] + value;
    }

    if (subset_visit(&ctx, 0, 0, 0, out_selected)) {
        *out_nodes = ctx.nodes;
        return SUBSET_FOUND;
    }

    *out_nodes = ctx.nodes;
    return ctx.limit_reached ? SUBSET_LIMIT_REACHED : SUBSET_NOT_FOUND;
}
```

The suffix sum is an upper bound on what the remaining items can add because all values are non-negative. The condition `sum + suffix_sum[index] < target` is therefore safe. The subtraction form `target - sum` is used only after `sum <= target`, so the inclusion check does not overflow.

The worst-case time remains O(`2^n`), but the bounds can eliminate large parts of the tree. Active recursion and the suffix array use O(`n`) memory.

### Python: Reference Model With Ordering

```python
def subset_sum(values, target, max_nodes=None):
    if any(value < 0 for value in values):
        raise ValueError("values must be non-negative")

    # Larger values first is a heuristic for reaching the target early.
    ordered = sorted(enumerate(values), key=lambda item: item[1], reverse=True)
    suffix = [0] * (len(ordered) + 1)
    for index in range(len(ordered) - 1, -1, -1):
        suffix[index] = suffix[index + 1] + ordered[index][1]

    nodes = 0

    def visit(index, total, selected):
        nonlocal nodes
        if max_nodes is not None and nodes >= max_nodes:
            raise TimeoutError("search node limit reached")
        nodes += 1

        if total == target:
            return selected.copy()
        if index == len(ordered) or total > target:
            return None
        if total + suffix[index] < target:
            return None

        original_index, value = ordered[index]
        selected.append(original_index)
        result = visit(index + 1, total + value, selected)
        if result is not None:
            return result
        selected.pop()

        return visit(index + 1, total, selected)

    return visit(0, 0, [])
```

The Python version sorts a copy to make the ordering heuristic explicit and retains original indexes in the result. The C example expects the caller to choose the input order, which avoids hidden allocation and copying.

## Correctness Of Pruning

For every pruning condition, document:

1. what property it checks
2. which assumptions make the property valid
3. why every descendant of a rejected state is unusable
4. whether it affects feasibility, optimality, or only search order

For the subset-sum upper bound:

- all remaining values are non-negative
- `suffix_sum[index]` is the sum of every value still available
- no descendant can exceed `sum + suffix_sum[index]`
- if that upper bound is below the target, no descendant can reach the target

This is a proof, not a performance guess.

## Instrumentation

Useful search diagnostics include:

- nodes visited
- deepest level reached
- branches rejected by each pruning rule
- maximum active memory
- elapsed time or deadline status
- first solution depth
- best solution cost and when it improved

Counters make a search failure explainable. They also show whether an optimization actually reduces work.

## Common Mistakes

- Applying a bound without checking its assumptions.
- Using a heuristic as if it proved impossibility.
- Pruning on the current cost when a later choice can reduce the cost.
- Deduplicating states without including all future-relevant fields.
- Sorting or reordering candidates but forgetting to preserve output identity.
- Returning `NOT_FOUND` when the node limit was reached.
- Logging every rejected branch on a large target and making diagnostics the dominant cost.

## Embedded And Systems Angle

- make heuristic limits configurable and observable when behavior matters
- distinguish correctness-preserving pruning from best-effort shortcuts
- use fixed-size counters and saturating or checked arithmetic for diagnostics
- prefer deterministic ordering for reproducible field failures
- check cancellation and node limits at a documented frequency
- record why branches were rejected when a failed search must be explained

## Related Topics

- [Search-Space Modeling](search-space-modeling.md)
- [Backtracking](backtracking.md)
- [Deterministic Runtime And Real-Time Tradeoffs](../embedded-linux-algorithmic-constraints/deterministic-runtime-and-real-time-tradeoffs.md)
- [Invariants And Correctness](../algorithmic-foundations/invariants-and-correctness.md)
- [Time And Space Complexity](../complexity-and-efficiency/time-and-space-complexity.md)
