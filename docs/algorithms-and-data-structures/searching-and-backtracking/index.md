---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Searching And Backtracking

Searching algorithms explore a set of possible states until they find a required state or establish that no acceptable state exists. Backtracking is a disciplined form of depth-first search: make a choice, check the partial state, continue if it can still succeed, and undo the choice when the branch fails.

This section is about making that exploration explicit. The important design work is to define the state, choices, transitions, constraints, stopping policy, and resource bounds before writing the traversal.

## Learning Goals

After this section, you should be able to:

- model a search space as states, choices, transitions, and goals
- distinguish complete search from guided search and heuristic ordering
- represent partial solutions without losing the information needed to undo a choice
- implement recursive and iterative backtracking in C
- separate candidate generation from constraint checking
- identify pruning rules that preserve correctness
- state the worst-case time, stack, and storage costs of a search
- add cancellation, node limits, and diagnostics to bounded systems searches

## Search Vocabulary

Search space:
: The set of states the algorithm may visit.

State:
: The complete information needed to decide which choices are available and what happens next.

Choice:
: One decision that moves the search from a state to a successor state.

Transition:
: The state change caused by applying a choice.

Partial solution:
: A state that represents only part of a complete candidate.

Constraint:
: A rule that rejects a state or choice.

Goal:
: A state that satisfies the requested result condition.

Backtracking:
: Applying a choice, exploring the resulting state, and undoing the choice before trying another choice.

Pruning:
: Proving that a state or branch cannot produce an acceptable result and skipping its descendants.

Heuristic:
: A rule for ordering choices or selecting promising states. A heuristic may improve speed without changing which solutions are valid.

## A Search Design Workflow

Use this order when designing an explicit search:

1. Define the complete state, including ownership and bounds.
2. Define the initial state.
3. List the legal choices from each state.
4. Define how a choice changes the state.
5. State the goal condition.
6. State which partial states are already impossible.
7. Choose depth-first, breadth-first, best-first, or another traversal.
8. Decide whether the search must find one solution, all solutions, or the best solution.
9. Bound memory, depth, nodes, and runtime.
10. Add tests for success, failure, empty choices, duplicate states, and cancellation.

If the state or transition is underspecified, the search code usually becomes a collection of special cases. A clear model makes the traversal mechanical.

## Choosing A Search Strategy

| Need | Good starting strategy |
| --- | --- |
| Small finite space and one answer | depth-first search |
| Need the shallowest unweighted answer | breadth-first search |
| Need every combination or arrangement | backtracking |
| Need the best answer with a usable bound | branch-and-bound |
| Need a likely answer under a strict budget | heuristic or guided search |
| Need repeated lookup of known keys | a data structure such as a hash table or sorted array |

The strategy is part of the problem contract. A search that returns the first answer is not interchangeable with one that must prove optimality or enumerate every answer.

## Running Example: Bounded Configuration Search

Suppose a diagnostic utility must choose a subset of features. Each feature has a cost, some pairs conflict, and a required set must be enabled. A useful model is:

- state: the next feature index, selected feature bitmask, and accumulated cost
- choices: enable or disable the next feature
- transition: set the bit and add the cost, or leave the state unchanged
- invalid state: cost exceeds the budget or a conflict is selected
- goal: all required features are selected after every feature was considered
- representation: a `uint32_t` bitmask, because the example allows at most 32 features

The search space has at most `2^n` leaves. That is acceptable only when `n` is bounded or when strong constraints reduce the explored portion.

## Programming Examples

### C: First Valid Configuration

This example keeps all search state in caller-owned or stack-owned storage. It returns the first solution in deterministic enable-first order.

```c
#include <stddef.h>
#include <stdint.h>

enum config_search_status {
    CONFIG_SEARCH_FOUND = 0,
    CONFIG_SEARCH_NOT_FOUND,
    CONFIG_SEARCH_ERR_NULL,
    CONFIG_SEARCH_ERR_COUNT,
    CONFIG_SEARCH_ERR_DATA
};

struct config_search_context {
    const uint32_t *costs;
    const uint32_t *conflicts;
    size_t feature_count;
    uint32_t valid_mask;
    uint32_t budget;
    uint32_t required_mask;
};

static int search_configuration_impl(const struct config_search_context *ctx,
                                     size_t index,
                                     uint32_t selected,
                                     uint32_t cost,
                                     uint32_t *out_mask)
{
    uint32_t bit;

    if (index == ctx->feature_count) {
        if ((selected & ctx->required_mask) != ctx->required_mask)
            return 0;
        *out_mask = selected;
        return 1;
    }

    bit = UINT32_C(1) << index;

    if (cost <= ctx->budget &&
        ctx->costs[index] <= ctx->budget - cost &&
        (selected & ctx->conflicts[index]) == 0) {
        if (search_configuration_impl(ctx,
                                      index + 1,
                                      selected | bit,
                                      cost + ctx->costs[index],
                                      out_mask))
            return 1;
    }

    return search_configuration_impl(ctx,
                                     index + 1,
                                     selected,
                                     cost,
                                     out_mask);
}

enum config_search_status search_configuration(const uint32_t *costs,
                                               const uint32_t *conflicts,
                                               size_t feature_count,
                                               uint32_t budget,
                                               uint32_t required_mask,
                                               uint32_t *out_mask)
{
    struct config_search_context ctx;
    uint32_t valid_mask;

    if (out_mask == NULL)
        return CONFIG_SEARCH_ERR_NULL;
    if (feature_count > 32)
        return CONFIG_SEARCH_ERR_COUNT;
    if (costs == NULL || conflicts == NULL)
        return feature_count == 0 ? CONFIG_SEARCH_ERR_DATA
                                  : CONFIG_SEARCH_ERR_NULL;

    valid_mask = feature_count == 32
               ? UINT32_MAX
               : ((UINT32_C(1) << feature_count) - 1u);
    if ((required_mask & ~valid_mask) != 0)
        return CONFIG_SEARCH_ERR_DATA;

    for (size_t i = 0; i < feature_count; i++) {
        if ((conflicts[i] & ~valid_mask) != 0)
            return CONFIG_SEARCH_ERR_DATA;
    }

    ctx = (struct config_search_context){
        .costs = costs,
        .conflicts = conflicts,
        .feature_count = feature_count,
        .valid_mask = valid_mask,
        .budget = budget,
        .required_mask = required_mask
    };

    return search_configuration_impl(&ctx, 0, 0, 0, out_mask)
         ? CONFIG_SEARCH_FOUND
         : CONFIG_SEARCH_NOT_FOUND;
}
```

The `cost <= budget - cost_of_feature` form avoids unsigned overflow when testing whether a feature fits. The recursive depth is at most 32, the extra search state is O(1) per level, and the worst-case time is O(2^n).

The `valid_mask` field is retained in the context to make the modeled state explicit, even though validation uses it before traversal. A larger implementation could use it for additional pruning or diagnostics.

### Python: Small Reference Model

```python
def first_configuration(costs, conflicts, budget, required_mask):
    count = len(costs)
    if count != len(conflicts) or count > 32:
        raise ValueError("invalid feature data")

    def visit(index, selected, cost):
        if index == count:
            return selected if selected & required_mask == required_mask else None

        bit = 1 << index
        if (
            cost + costs[index] <= budget
            and selected & conflicts[index] == 0
        ):
            result = visit(index + 1, selected | bit, cost + costs[index])
            if result is not None:
                return result

        return visit(index + 1, selected, cost)

    result = visit(0, 0, 0)
    if result is None:
        raise LookupError("no configuration satisfies the constraints")
    return result
```

The Python version is useful as a test oracle for small feature sets. It has the same choice order and therefore makes differences in the C search easier to diagnose.

## Correctness Shape

A search correctness argument should state:

- every legal successor is generated, unless a documented pruning rule proves it useless
- every generated state satisfies the representation invariant
- a goal is accepted only when the goal predicate is true
- when the search reports not found, every unpruned branch has been exhausted

For backtracking, add one more point:

- after returning from a choice, the state is restored exactly to the state before that choice

That last rule is the central backtracking invariant. If apply and undo are not symmetric, later branches are explored from corrupted state.

## Complexity And Resource Bounds

If each state has at most `b` choices and the maximum depth is `d`, a simple depth-first search may visit O(`b^d`) states. Its active memory is usually O(`d`) when state is stored on a stack, plus any representation of the current candidate.

The number of generated states is not the only cost. Constraint checks, copying candidates, logging, allocation, and cache behavior can dominate small searches. Measure or count nodes when the target is resource-constrained.

## Common Mistakes

- Searching an informal idea instead of a defined state space.
- Forgetting whether the result is one solution, all solutions, or an optimum.
- Treating a partial solution as valid without checking the constraints that already apply.
- Mutating shared state without a matching undo operation.
- Assuming a heuristic makes an exhaustive search complete when a timeout or node limit stops it early.
- Allowing duplicate logical states to be visited indefinitely.
- Using unbounded recursion or allocation on a path that can be controlled by input.

## Embedded And Systems Angle

- bound search depth, node count, memory, and wall-clock work
- use deterministic choice order for reproducible diagnostics and tests
- keep candidate generation separate from constraint checking
- expose counters, cancellation, and limit-hit status to callers
- prefer fixed-size state and explicit stacks when recursion depth is not tightly bounded
- record enough state to resume, explain, or replay a failed search

## Pages In This Section

- [Search-Space Modeling](search-space-modeling.md)
- [Backtracking](backtracking.md)
- [Pruning And Search Heuristics](pruning-and-search-heuristics.md)

## Related Topics

- [Control Flow And Recursion](../control-flow-and-recursion/index.md)
- [Invariants And Correctness](../algorithmic-foundations/invariants-and-correctness.md)
- [Graph Algorithms](../graph-algorithms/index.md)
- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
