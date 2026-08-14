---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Recursion And Stack-Depth Policy

Recursion is an algorithmic choice that stores one activation record per active level. It can make tree, divide-and-conquer, and backtracking code clear, but the stack cost depends on compiler, architecture, optimization, local variables, and input depth.

On embedded Linux, a recursion policy should be explicit wherever input or external state controls depth.

## Depth Budget

A depth budget should answer:

- what input property bounds depth
- maximum expected and maximum accepted depth
- estimated stack bytes per frame
- remaining stack margin for callers and signals
- what status is returned when the limit is reached
- whether an iterative implementation is available

Do not infer safety from a small source function. Compiler-generated spills, alignment, saved registers, and instrumentation can change frame size.

## When Recursion Fits

Recursion is a reasonable choice when:

- maximum depth is a small documented constant
- the input is validated before traversal
- the stack budget has been measured for the target build
- the clarity benefit is material

Prefer an explicit stack when depth is data-dependent, attacker-controlled, large, or must be paused and resumed.

## Programming Examples

### C: Depth-Limited Recursive Traversal

This traversal returns a distinct limit status before making a call beyond the configured maximum.

```c
#include <stddef.h>

enum depth_status {
    DEPTH_OK = 0,
    DEPTH_LIMIT,
    DEPTH_ERR_NULL,
    DEPTH_ERR_INDEX
};

struct depth_frame {
    size_t index;
    size_t depth;
};

struct depth_node {
    int value;
    size_t child_count;
    size_t children[4];
};

static enum depth_status visit_node(const struct depth_node *nodes,
                                    size_t node_count,
                                    size_t index,
                                    size_t depth,
                                    size_t max_depth,
                                    int *out_sum)
{
    if (index >= node_count)
        return DEPTH_ERR_INDEX;
    if (depth > max_depth)
        return DEPTH_LIMIT;

    *out_sum += nodes[index].value;
    for (size_t i = 0; i < nodes[index].child_count; i++) {
        enum depth_status status;

        if (i >= 4)
            return DEPTH_ERR_INDEX;
        status = visit_node(nodes,
                            node_count,
                            nodes[index].children[i],
                            depth + 1,
                            max_depth,
                            out_sum);
        if (status != DEPTH_OK)
            return status;
    }
    return DEPTH_OK;
}

enum depth_status sum_tree_bounded(const struct depth_node *nodes,
                                   size_t node_count,
                                   size_t root,
                                   size_t max_depth,
                                   int *out_sum)
{
    if ((nodes == NULL && node_count > 0) || out_sum == NULL)
        return DEPTH_ERR_NULL;
    if (node_count == 0)
        return DEPTH_ERR_INDEX;

    *out_sum = 0;
    return visit_node(nodes, node_count, root, 0, max_depth, out_sum);
}
```

The depth check protects the algorithmic contract, but it does not prove the C call stack is large enough for `max_depth`. Measure frame use and choose a lower policy limit when necessary.

### C: Explicit-Stack Rewrite

```c
enum depth_status sum_tree_iterative(const struct depth_node *nodes,
                                     size_t node_count,
                                     size_t root,
                                     size_t max_depth,
                                     int *out_sum)
{
    struct depth_frame stack[32];
    size_t stack_count = 0;

    if ((nodes == NULL && node_count > 0) || out_sum == NULL)
        return DEPTH_ERR_NULL;
    if (root >= node_count || max_depth >= 32)
        return DEPTH_ERR_INDEX;

    *out_sum = 0;
    stack[stack_count++] = (struct depth_frame){ .index = root, .depth = 0 };
    while (stack_count > 0) {
        struct depth_frame frame = stack[--stack_count];
        size_t index = frame.index;
        size_t depth = frame.depth;

        if (depth > max_depth)
            return DEPTH_LIMIT;
        if (index >= node_count)
            return DEPTH_ERR_INDEX;
        *out_sum += nodes[index].value;
        for (size_t i = 0; i < nodes[index].child_count; i++) {
            if (i >= 4 || stack_count == 32)
                return DEPTH_ERR_INDEX;
            stack[stack_count++] = (struct depth_frame){
                .index = nodes[index].children[i],
                .depth = depth + 1
            };
        }
    }
    return DEPTH_OK;
}
```

The explicit-stack example uses GNU `typeof` for brevity; portable C should name the frame struct instead. The important property is that the maximum stack array is visible and independently bounded.

### Python: Depth-Limited Reference

```python
def sum_tree(node, depth=0, max_depth=32):
    if node is None:
        return 0
    if depth > max_depth:
        raise RecursionError("depth limit reached")
    return node.value + sum(
        sum_tree(child, depth + 1, max_depth) for child in node.children
    )
```

The Python limit is a semantic reference. Python's own interpreter stack limit is a separate runtime constraint.

## Cycle Detection

Recursive tree code assumes the input is a tree. If malformed or externally supplied links can contain cycles, maintain a visited set or validate the graph first. A depth limit alone prevents infinite recursion but may return a misleading depth failure instead of identifying corruption.

## Stack Measurement

Measure rather than guess:

- inspect compiler frame reports where available
- use a known stack pattern and measure high-water mark in test builds
- test maximum-depth and worst-local-variable paths
- include signal, interrupt, tracing, and sanitizer overhead where relevant

The measured result belongs to a specific architecture, compiler, optimization, and configuration.

## Common Mistakes

- Assuming source-level frame size is obvious.
- Accepting input depth without checking a policy limit.
- Replacing recursion with an explicit stack but leaving the stack unbounded.
- Using a depth limit as the only corruption detector.
- Forgetting that callbacks or logging can add nested stack use.
- Returning success after skipping descendants beyond the depth limit.

## Embedded And Systems Angle

- avoid data-dependent recursion in kernel, driver, and constrained-thread contexts unless bounded
- document depth assumptions with the data model
- test pathological depth cases and stack high-water marks
- use explicit stacks when pause/resume or cancellation is required
- distinguish depth-limit failure from malformed topology

## Related Topics

- [Embedded Linux Algorithmic Constraints](index.md)
- [Recursion Fundamentals](../control-flow-and-recursion/recursion-fundamentals.md)
- [Tree Traversals](../tree-algorithms/tree-traversals.md)
- [Backtracking](../searching-and-backtracking/backtracking.md)
