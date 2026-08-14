---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: beginner
last_reviewed: null
---

# Tree Representations

A tree models hierarchical relationships. It has a root, zero or more children per node, and no path from a node back to one of its ancestors. The representation determines how ownership, traversal, insertion, deletion, and maximum depth are handled.

## Tree Vocabulary

Root:
: The node with no parent.

Parent and child:
: A directed containment relationship from an upper node to a lower node.

Leaf:
: A node with no children.

Subtree:
: A node together with all of its descendants.

Depth:
: The number of edges from the root to a node.

Height:
: The greatest depth in the tree, or another convention explicitly defined by the API.

Binary tree:
: A tree in which each node has at most a left and a right child.

Binary search tree:
: A binary tree with an ordering invariant, such as all left keys being less than the node key and all right keys being greater.

Do not confuse a general tree with a binary search tree. A binary tree has a shape constraint; a search tree has an additional ordering constraint.

## Representation Choices

| Representation | Strength | Cost or risk |
| --- | --- | --- |
| pointer-linked nodes | flexible shape and direct links | allocation, ownership, fragmentation |
| index-linked nodes | fixed storage and relocatable references | index validation and sentinel policy |
| complete-tree array | compact heap-like layout | wasted slots for sparse shapes |
| parent-child records | explicit ownership and metadata | more fields and consistency checks |

Use an array-backed representation when the maximum node count is known or when relocation and serialization matter. Use pointers only when their lifetime and allocation policy are explicit.

## Representation Invariants

A valid binary tree should define:

- which index represents no child
- whether every node is reachable from the root
- whether a node can have more than one parent
- whether cycles are forbidden and how validation detects them
- whether `count` includes allocated nodes or reachable nodes
- who owns node storage and when a node may be removed

For an index-based tree, a useful invariant is:

> Every non-sentinel child index is less than the node capacity, and every reachable node is visited at most once from the root.

## Programming Examples

### C: Fixed-Capacity Index-Based Binary Tree

The tree stores child indexes instead of pointers. `TREE_NONE` represents no child. Nodes are allocated from a fixed pool and linked only after their indexes are validated.

```c
#include <stddef.h>

enum {
    TREE_MAX_NODES = 32,
    TREE_NONE = (size_t)-1
};

enum tree_status {
    TREE_OK = 0,
    TREE_FULL,
    TREE_ERR_NULL,
    TREE_ERR_INDEX
};

struct tree_node {
    int value;
    size_t left;
    size_t right;
};

struct binary_tree {
    struct tree_node nodes[TREE_MAX_NODES];
    size_t count;
    size_t root;
};

void binary_tree_init(struct binary_tree *tree)
{
    if (tree == NULL)
        return;

    tree->count = 0;
    tree->root = TREE_NONE;
}

enum tree_status binary_tree_new_node(struct binary_tree *tree,
                                      int value,
                                      size_t *out_index)
{
    size_t index;

    if (tree == NULL || out_index == NULL)
        return TREE_ERR_NULL;
    if (tree->count == TREE_MAX_NODES)
        return TREE_FULL;

    index = tree->count++;
    tree->nodes[index] = (struct tree_node){
        .value = value,
        .left = TREE_NONE,
        .right = TREE_NONE
    };
    *out_index = index;
    return TREE_OK;
}

enum tree_status binary_tree_set_root(struct binary_tree *tree,
                                      size_t root)
{
    if (tree == NULL)
        return TREE_ERR_NULL;
    if (root != TREE_NONE && root >= tree->count)
        return TREE_ERR_INDEX;
    tree->root = root;
    return TREE_OK;
}

enum tree_status binary_tree_link_left(struct binary_tree *tree,
                                       size_t parent,
                                       size_t child)
{
    if (tree == NULL)
        return TREE_ERR_NULL;
    if (parent >= tree->count ||
        (child != TREE_NONE && child >= tree->count))
        return TREE_ERR_INDEX;

    tree->nodes[parent].left = child;
    return TREE_OK;
}

enum tree_status binary_tree_link_right(struct binary_tree *tree,
                                        size_t parent,
                                        size_t child)
{
    if (tree == NULL)
        return TREE_ERR_NULL;
    if (parent >= tree->count ||
        (child != TREE_NONE && child >= tree->count))
        return TREE_ERR_INDEX;

    tree->nodes[parent].right = child;
    return TREE_OK;
}
```

The link functions validate indexes but do not prove that a new link preserves the tree shape. A complete validator should detect cycles and multiple parents before the tree is exposed to traversal algorithms.

### Python: Pointer-Like Reference Model

```python
class Node:
    def __init__(self, value, left=None, right=None):
        self.value = value
        self.left = left
        self.right = right


def validate_tree(root):
    seen = set()

    def visit(node):
        if node is None:
            return
        identity = id(node)
        if identity in seen:
            raise ValueError("cycle or shared child detected")
        seen.add(identity)
        visit(node.left)
        visit(node.right)

    visit(root)
```

The identity set detects cycles and shared subtrees. A directed acyclic graph can share children, but that is not a tree and needs a different ownership and traversal contract.

## Ownership And Lifetime

Pointer-linked trees require answers to these questions:

- does the parent own and destroy its children?
- can a node appear in more than one tree?
- can callers retain a node pointer after removal?
- who updates parent links during rotation or deletion?
- what happens if allocation fails halfway through construction?

Index-based trees move the lifetime problem into pool management, but they do not eliminate it. A released index must not be reused while an old link still references it.

## Binary Search Tree Invariant

For a strict integer binary search tree:

- every key in the left subtree is less than the node key
- every key in the right subtree is greater than the node key
- duplicates are either rejected or assigned to a documented side

An inorder traversal produces sorted keys only when this invariant holds. A binary tree with arbitrary child links does not automatically support ordered lookup.

## Common Mistakes

- Treating a binary tree as a binary search tree without an ordering invariant.
- Using zero as the no-child sentinel when index zero is a valid node.
- Reusing an index while an old parent still points to it.
- Assuming child links alone prove acyclicity or unique ownership.
- Deleting a node without defining what happens to its descendants.
- Storing pointers into movable storage without an update strategy.

## Embedded And Systems Angle

- prefer index-based trees when storage is fixed, relocatable, or serialized
- make parent ownership and child lifetime explicit
- account for maximum depth, node count, and validator cost
- avoid hidden allocation in insertion and balancing operations
- use compact indexes only when their range and sentinel values are validated

## Related Topics

- [Tree Algorithms](index.md)
- [Tree Traversals](tree-traversals.md)
- [Expression Trees And Stack Evaluation](expression-trees-and-stack-evaluation.md)
- [Data Modeling And Abstract Data Types](../algorithmic-foundations/data-modeling-and-abstract-data-types.md)
- [Memory Pools And Fixed-Size Allocators](../data-structures-for-algorithms/memory-pools-and-fixed-size-allocators.md)
