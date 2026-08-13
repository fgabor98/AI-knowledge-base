---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Search-Space Modeling

Search-space modeling is the act of describing possible states, legal choices, transitions, constraints, and goals before implementing traversal. The model determines whether a search is complete, whether duplicate states are possible, and whether the resource cost can be bounded.

A search is easier to review when the traversal mechanics do not contain hidden domain rules. Put the domain rules in the state and constraint predicates, then let depth-first search, breadth-first search, or backtracking operate on that model.

## The State Model

A useful search state answers:

- where is the search in the decision sequence?
- what choices have already been made?
- which resources have been consumed?
- which constraints are currently active?
- what information is needed to generate the next choices?
- can two different paths represent the same logical state?

The state should contain enough information to determine the future. If two states have the same relevant future but are represented differently, the search may redo work. If the state omits relevant history, the algorithm may accept an invalid result.

## Choices And Transitions

For each state, define:

1. the legal choices
2. the next state after applying a choice
3. whether the choice can be undone
4. the cost of the transition
5. the condition under which the transition is rejected

For a backtracking search, the transition is often represented as three operations:

```text
apply(choice)
check(state)
undo(choice)
```

The `undo` operation must restore every field changed by `apply`. A partial restoration is a state corruption bug, not merely a performance problem.

## Search Trees And Search Graphs

If every path to a state is treated as a separate candidate, the representation is a search tree. If different paths can reach the same logical state, the problem is a search graph.

Tree search is appropriate when:

- path history is part of the candidate
- each combination should be considered separately
- the depth and branching factor are small enough

Graph search needs a visited-state policy when:

- cycles are possible
- multiple paths reach the same state
- revisiting a state cannot improve the result

The visited key must represent logical state, not necessarily the address or sequence of the object holding it.

## Partial Solutions

A partial solution is not just an incomplete output buffer. It is a state with an invariant.

For an assignment search, a partial solution might guarantee:

- every assigned task has a valid owner
- no assigned resources conflict
- current load is within capacity
- unassigned tasks remain representable

The search can reject a partial solution as soon as one of these facts becomes false. This is often the first and most valuable form of pruning.

## Estimating The Space

If a search has branching factor `b` and depth `d`, a rough upper bound for a tree is:

```text
1 + b + b² + ... + b^d = O(b^d)
```

For a binary decision at each of `n` positions, there are at most `2^n` leaves. For permutations of `n` items, there can be `n!` leaves. These estimates should be written down before choosing an implementation.

The estimate is an upper bound, not a performance guarantee. Constraints may cut off most branches, while weak constraints may leave the worst case nearly unchanged.

## Representation Choices

| Search data | Common representation | Important concern |
| --- | --- | --- |
| fixed number of yes/no choices | bitmask | limit the number of bits and validate masks |
| ordered assignment | fixed array plus depth | define which entries are initialized |
| remaining items | bitset or boolean array | update and restore membership symmetrically |
| path through a graph | vertex array plus visited set | distinguish vertex identity from path position |
| bounded resource totals | integer counters | prevent overflow and state impossible ranges |
| large or sparse state | caller-owned structure | define ownership and copy cost |

Use the smallest representation that keeps the future behavior unambiguous. Compact state can reduce memory and improve cache behavior, but only if the bit-level invariants are clear.

## Example Model: Feature Selection

Consider a configuration with `n` optional features. Each feature has a cost, some features conflict, and a required mask describes capabilities that must be present.

The state is:

```text
(next_feature, selected_mask, accumulated_cost)
```

The choices are:

```text
enable next_feature
disable next_feature
```

The transitions are:

- enabling sets the corresponding bit and adds the feature cost
- disabling leaves the selected mask and cost unchanged
- both advance `next_feature`

The constraints are:

- accumulated cost does not exceed the budget
- no selected feature conflicts with another selected feature

The goal is reached when all features have been considered and the required mask is included.

This model exposes the worst case immediately: two choices at each of `n` levels. It also exposes safe pruning opportunities, such as rejecting a cost-over-budget state before descending further.

## Programming Examples

### C: Explicit State And Deterministic Traversal

The following implementation is a small depth-first search over the feature model. It is intentionally similar to the overview example, but keeps the state and transition decisions visible for modeling purposes.

```c
#include <stddef.h>
#include <stdint.h>

enum feature_search_status {
    FEATURE_SEARCH_FOUND = 0,
    FEATURE_SEARCH_NOT_FOUND,
    FEATURE_SEARCH_ERR_NULL,
    FEATURE_SEARCH_ERR_COUNT,
    FEATURE_SEARCH_ERR_MASK
};

struct feature_state {
    const uint32_t *costs;
    const uint32_t *conflicts;
    size_t count;
    uint32_t budget;
    uint32_t required;
    uint32_t valid_mask;
};

static int feature_search(const struct feature_state *state,
                          size_t next,
                          uint32_t selected,
                          uint32_t cost,
                          uint32_t *out_selected)
{
    uint32_t bit;

    if (cost > state->budget)
        return 0;
    if (next == state->count) {
        if ((selected & state->required) != state->required)
            return 0;
        *out_selected = selected;
        return 1;
    }

    bit = UINT32_C(1) << next;

    /* Enable: the transition is legal only if cost and conflicts permit it. */
    if (cost <= state->budget &&
        state->costs[next] <= state->budget - cost &&
        (selected & state->conflicts[next]) == 0 &&
        feature_search(state,
                       next + 1,
                       selected | bit,
                       cost + state->costs[next],
                       out_selected))
        return 1;

    /* Disable: no state mutation is needed for this transition. */
    return feature_search(state, next + 1, selected, cost, out_selected);
}

enum feature_search_status find_feature_configuration(
    const uint32_t *costs,
    const uint32_t *conflicts,
    size_t count,
    uint32_t budget,
    uint32_t required,
    uint32_t *out_selected)
{
    struct feature_state state;

    if (out_selected == NULL)
        return FEATURE_SEARCH_ERR_NULL;
    if (count == 0 || count > 32)
        return FEATURE_SEARCH_ERR_COUNT;
    if (costs == NULL || conflicts == NULL)
        return FEATURE_SEARCH_ERR_NULL;

    state = (struct feature_state){
        .costs = costs,
        .conflicts = conflicts,
        .count = count,
        .budget = budget,
        .required = required,
        .valid_mask = count == 32
                    ? UINT32_MAX
                    : ((UINT32_C(1) << count) - 1u)
    };

    if ((required & ~state.valid_mask) != 0)
        return FEATURE_SEARCH_ERR_MASK;

    for (size_t i = 0; i < count; i++) {
        if ((conflicts[i] & ~state.valid_mask) != 0)
            return FEATURE_SEARCH_ERR_MASK;
    }

    return feature_search(&state, 0, 0, 0, out_selected)
         ? FEATURE_SEARCH_FOUND
         : FEATURE_SEARCH_NOT_FOUND;
}
```

The `next` field is not just a loop index. It is part of the state because it determines which choices remain. The bitmask represents the decisions already made, and `cost` represents a resource constraint derived from those decisions.

### Python: Enumerating A Small State Space

```python
def configurations(count):
    """Yield every yes/no configuration in deterministic order."""
    if count < 0:
        raise ValueError("count must not be negative")

    def visit(index, selected):
        if index == count:
            yield selected
            return

        bit = 1 << index
        yield from visit(index + 1, selected | bit)
        yield from visit(index + 1, selected)

    yield from visit(0, 0)
```

For `count == 3`, this yields eight leaves. The generator is a compact way to inspect the modeled space and write tests for a C implementation. It should not be used unbounded on a target where `2^count` is not acceptable.

## Correctness Questions

Before trusting a search, answer:

- Does every legal choice appear in the successor set?
- Can a transition produce a state that violates the representation invariant?
- Is a rejected state truly unable to produce a goal?
- Are two logically equal states recognized as equal when required?
- Does the goal predicate match the caller's requested result?
- Does a not-found result mean exhaustive search, or only that the budget expired?

For a tree search with no pruning, completeness follows from visiting every legal choice at every state. Once pruning or deduplication is added, each rule needs its own justification.

## Common Mistakes

- Omitting a field that affects future legal choices.
- Calling a path a state when different paths can lead to the same future.
- Using a visited set keyed by pointer identity instead of logical state.
- Mixing state mutation with constraint checking so that rejected choices are not fully undone.
- Ignoring copy cost when passing large states by value.
- Reporting "not found" after a node or time limit without indicating that the search was incomplete.

## Embedded And Systems Angle

- put maximum state sizes in the API contract
- use fixed arrays, masks, and caller-owned buffers where bounds are known
- separate traversal mechanics from device, protocol, or policy constraints
- add a node counter and cancellation check at a predictable point
- keep state snapshots small enough for diagnostics, tracing, or resume support

## Related Topics

- [Searching And Backtracking](index.md)
- [Backtracking](backtracking.md)
- [Pruning And Search Heuristics](pruning-and-search-heuristics.md)
- [Data Modeling And Abstract Data Types](../algorithmic-foundations/data-modeling-and-abstract-data-types.md)
- [Complexity And Efficiency](../complexity-and-efficiency/index.md)
