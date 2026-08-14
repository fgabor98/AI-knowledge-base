---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Tree Traversals

A tree traversal visits every reachable node according to an order chosen for the task. Preorder, inorder, and postorder are depth-first orders; level-order is breadth-first.

The traversal order is part of the algorithm's meaning. It determines when a parent is observed relative to its children and therefore affects serialization, expression evaluation, deletion, and search-tree output.

## Traversal Orders

For a node with left subtree `L`, value `N`, and right subtree `R`:

Preorder:
: `N, L, R`; useful for copying or serializing a tree with parent information first.

Inorder:
: `L, N, R`; produces sorted keys for a valid binary search tree.

Postorder:
: `L, R, N`; useful when children must be processed before a parent, such as cleanup or expression evaluation.

Level-order:
: Visit depth 0, then depth 1, and so on; useful for breadth-first views and shortest-depth measurements.

## Recursive Traversal Invariant

For a recursive traversal of subtree `T`:

> The function emits exactly the nodes reachable from `T`, once each, in the requested local order, and does not change the tree.

The recursion depth is O(h), where `h` is tree height. A balanced tree has O(log n) depth, but an unbalanced tree can have O(n) depth.

## Programming Examples

### C: Recursive Depth-First Orders

This example uses the index-based binary tree representation from the previous chapter and writes the traversal into a caller-provided output array.

```c
#include <stddef.h>

enum {
    TRAVERSAL_MAX_NODES = 32,
    TRAVERSAL_NONE = (size_t)-1
};

enum traversal_status {
    TRAVERSAL_OK = 0,
    TRAVERSAL_ERR_NULL,
    TRAVERSAL_ERR_INDEX,
    TRAVERSAL_ERR_OUTPUT
};

struct traversal_node {
    int value;
    size_t left;
    size_t right;
};

struct traversal_tree {
    struct traversal_node nodes[TRAVERSAL_MAX_NODES];
    size_t count;
    size_t root;
};

static enum traversal_status traversal_visit(
    const struct traversal_tree *tree,
    size_t index,
    int order,
    int *out_values,
    size_t out_capacity,
    size_t *out_count)
{
    const struct traversal_node *node;
    enum traversal_status status;

    if (index == TRAVERSAL_NONE)
        return TRAVERSAL_OK;
    if (index >= tree->count)
        return TRAVERSAL_ERR_INDEX;

    node = &tree->nodes[index];
    if (order == 0) {
        if (*out_count == out_capacity)
            return TRAVERSAL_ERR_OUTPUT;
        out_values[(*out_count)++] = node->value;
    }

    status = traversal_visit(tree,
                             node->left,
                             order,
                             out_values,
                             out_capacity,
                             out_count);
    if (status != TRAVERSAL_OK)
        return status;

    if (order == 1) {
        if (*out_count == out_capacity)
            return TRAVERSAL_ERR_OUTPUT;
        out_values[(*out_count)++] = node->value;
    }

    status = traversal_visit(tree,
                             node->right,
                             order,
                             out_values,
                             out_capacity,
                             out_count);
    if (status != TRAVERSAL_OK)
        return status;

    if (order == 2) {
        if (*out_count == out_capacity)
            return TRAVERSAL_ERR_OUTPUT;
        out_values[(*out_count)++] = node->value;
    }

    return TRAVERSAL_OK;
}

enum traversal_status tree_depth_first(const struct traversal_tree *tree,
                                       int order,
                                       int *out_values,
                                       size_t out_capacity,
                                       size_t *out_count)
{
    if (tree == NULL || out_values == NULL || out_count == NULL)
        return TRAVERSAL_ERR_NULL;
    if (order < 0 || order > 2)
        return TRAVERSAL_ERR_INDEX;

    *out_count = 0;
    return traversal_visit(tree,
                           tree->root,
                           order,
                           out_values,
                           out_capacity,
                           out_count);
}
```

Here `order` means 0 for preorder, 1 for inorder, and 2 for postorder. A production API may use an enum instead of integer values; the example keeps the three insertion points visible.

### C: Iterative Inorder Traversal

An explicit stack avoids recursion and makes depth capacity visible.

```c
enum traversal_status tree_inorder_iterative(
    const struct traversal_tree *tree,
    int *out_values,
    size_t out_capacity,
    size_t *out_count)
{
    size_t stack[TRAVERSAL_MAX_NODES];
    size_t stack_count = 0;
    size_t current;

    if (tree == NULL || out_values == NULL || out_count == NULL)
        return TRAVERSAL_ERR_NULL;

    *out_count = 0;
    current = tree->root;

    while (current != TRAVERSAL_NONE || stack_count > 0) {
        while (current != TRAVERSAL_NONE) {
            if (current >= tree->count ||
                stack_count == TRAVERSAL_MAX_NODES)
                return TRAVERSAL_ERR_INDEX;
            stack[stack_count++] = current;
            current = tree->nodes[current].left;
        }

        current = stack[--stack_count];
        if (*out_count == out_capacity)
            return TRAVERSAL_ERR_OUTPUT;
        out_values[(*out_count)++] = tree->nodes[current].value;
        current = tree->nodes[current].right;
    }

    return TRAVERSAL_OK;
}
```

The stack holds the path to the next node. The fixed capacity is safe only if the tree has at most `TRAVERSAL_MAX_NODES` reachable nodes and cycles have been ruled out.

### C: Level-Order Traversal

```c
enum traversal_status tree_level_order(
    const struct traversal_tree *tree,
    int *out_values,
    size_t out_capacity,
    size_t *out_count)
{
    size_t queue[TRAVERSAL_MAX_NODES];
    size_t head = 0;
    size_t tail = 0;

    if (tree == NULL || out_values == NULL || out_count == NULL)
        return TRAVERSAL_ERR_NULL;

    *out_count = 0;
    if (tree->root == TRAVERSAL_NONE)
        return TRAVERSAL_OK;
    if (tree->root >= tree->count)
        return TRAVERSAL_ERR_INDEX;

    queue[tail++] = tree->root;
    while (head < tail) {
        size_t index = queue[head++];
        const struct traversal_node *node = &tree->nodes[index];

        if (*out_count == out_capacity)
            return TRAVERSAL_ERR_OUTPUT;
        out_values[(*out_count)++] = node->value;

        if (node->left != TRAVERSAL_NONE) {
            if (node->left >= tree->count || tail == TRAVERSAL_MAX_NODES)
                return TRAVERSAL_ERR_INDEX;
            queue[tail++] = node->left;
        }
        if (node->right != TRAVERSAL_NONE) {
            if (node->right >= tree->count || tail == TRAVERSAL_MAX_NODES)
                return TRAVERSAL_ERR_INDEX;
            queue[tail++] = node->right;
        }
    }
    return TRAVERSAL_OK;
}
```

Level-order storage is O(w), where `w` is maximum tree width, and is bounded by the node count in this implementation.

### Python: Traversal Reference

```python
from collections import deque


def preorder(node):
    if node is None:
        return []
    return [node.value] + preorder(node.left) + preorder(node.right)


def inorder(node):
    if node is None:
        return []
    return inorder(node.left) + [node.value] + inorder(node.right)


def postorder(node):
    if node is None:
        return []
    return postorder(node.left) + postorder(node.right) + [node.value]


def level_order(root):
    if root is None:
        return []
    result = []
    queue = deque([root])
    while queue:
        node = queue.popleft()
        result.append(node.value)
        if node.left is not None:
            queue.append(node.left)
        if node.right is not None:
            queue.append(node.right)
    return result
```

The recursive Python helpers allocate intermediate lists for clarity. A production reference model can use generators if the tree is large.

## Matching Order To A Task

- use preorder when a parent declaration must precede its children
- use inorder for sorted output from a valid binary search tree
- use postorder before freeing, folding, or evaluating child-dependent state
- use level-order for depth, nearest-node, and breadth-limited work

Traversal is not automatically safe for mutation. If deleting nodes during traversal, save the next link before mutation or use a traversal designed for that update policy.

## Common Mistakes

- Calling inorder output sorted when the tree is not a search tree.
- Using recursive traversal on a degenerate tree with unbounded depth.
- Forgetting to validate child indexes before dereferencing them.
- Reusing a queue or stack without defining capacity and empty behavior.
- Mutating links while a traversal still relies on them.
- Assuming level-order output is a sorted or search order.

## Embedded And Systems Angle

- replace recursive traversal with explicit stacks when depth is input-controlled
- use queues and stacks with known capacities
- choose traversal order based on cleanup, evaluation, serialization, or scheduling needs
- validate the tree once when malformed links are possible
- expose maximum depth and width in resource-sensitive APIs

## Related Topics

- [Tree Algorithms](index.md)
- [Tree Representations](tree-representations.md)
- [Expression Trees And Stack Evaluation](expression-trees-and-stack-evaluation.md)
- [Recursion And Stack-Depth Policy](../embedded-linux-algorithmic-constraints/recursion-and-stack-depth-policy.md)
- [Linked Lists Stacks And Queues](../data-structures-for-algorithms/linked-lists-stacks-and-queues.md)
