---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: advanced
last_reviewed: null
---

# Advanced Tree Algorithms

Advanced tree algorithms use ordering, ancestry, and subtree structure to answer queries or maintain dynamic collections. The basic representation and traversal pages explain how to visit a tree; this page focuses on the invariants that make lookup, update, prefix search, and aggregation efficient.

## Binary Search Tree Invariant

For a binary search tree (BST), every key in the left subtree compares before the node key and every key in the right subtree compares after it. Duplicate keys require a policy:

- reject duplicates
- store a count in one node
- place equals consistently on one side
- attach a stable record list to the key

The policy must be used by search, insertion, deletion, and validation. A tree that is locally ordered but handles equal keys inconsistently is not a reliable ordered collection.

Lookup and insertion take O(h), where `h` is tree height. A balanced tree has h = O(log n); an insertion order that is already sorted can produce h = O(n).

## Fixed-Node BST

Index-backed nodes avoid per-node allocation and make the structure serializable. The free-list and generation policy are part of the data structure contract.

### C: Bounded BST Lookup And Insert

```c
#include <stddef.h>

enum {
    BST_CAPACITY = 32,
    BST_NONE = (size_t)-1
};

enum bst_status {
    BST_OK = 0,
    BST_FOUND,
    BST_NOT_FOUND,
    BST_FULL,
    BST_DUPLICATE,
    BST_ERR_NULL
};

struct bst_node {
    int key;
    size_t left;
    size_t right;
    size_t parent;
    unsigned char used;
};

struct bounded_bst {
    struct bst_node nodes[BST_CAPACITY];
    size_t root;
    size_t count;
};

void bounded_bst_init(struct bounded_bst *tree)
{
    if (tree == NULL)
        return;
    tree->root = BST_NONE;
    tree->count = 0;
    for (size_t i = 0; i < BST_CAPACITY; i++) {
        tree->nodes[i].left = BST_NONE;
        tree->nodes[i].right = BST_NONE;
        tree->nodes[i].parent = BST_NONE;
        tree->nodes[i].used = 0;
    }
}

static size_t bst_free_node(const struct bounded_bst *tree)
{
    for (size_t i = 0; i < BST_CAPACITY; i++)
        if (!tree->nodes[i].used)
            return i;
    return BST_NONE;
}

enum bst_status bounded_bst_insert(struct bounded_bst *tree, int key)
{
    size_t parent = BST_NONE;
    size_t current;
    size_t slot;

    if (tree == NULL)
        return BST_ERR_NULL;
    current = tree->root;
    while (current != BST_NONE) {
        parent = current;
        if (key == tree->nodes[current].key)
            return BST_DUPLICATE;
        current = key < tree->nodes[current].key
                    ? tree->nodes[current].left
                    : tree->nodes[current].right;
    }

    slot = bst_free_node(tree);
    if (slot == BST_NONE)
        return BST_FULL;
    tree->nodes[slot] = (struct bst_node){
        .key = key,
        .left = BST_NONE,
        .right = BST_NONE,
        .parent = parent,
        .used = 1
    };
    if (parent == BST_NONE)
        tree->root = slot;
    else if (key < tree->nodes[parent].key)
        tree->nodes[parent].left = slot;
    else
        tree->nodes[parent].right = slot;
    tree->count++;
    return BST_OK;
}

enum bst_status bounded_bst_find(const struct bounded_bst *tree,
                                 int key,
                                 size_t *out_index)
{
    size_t current;

    if (tree == NULL || out_index == NULL)
        return BST_ERR_NULL;
    current = tree->root;
    while (current != BST_NONE) {
        if (key == tree->nodes[current].key) {
            *out_index = current;
            return BST_FOUND;
        }
        current = key < tree->nodes[current].key
                    ? tree->nodes[current].left
                    : tree->nodes[current].right;
    }
    return BST_NOT_FOUND;
}
```

The free-node scan is O(capacity), so this fixed example is optimized for predictable storage, not asymptotic allocation. A free list can make slot allocation O(1), but it introduces another invariant that must be validated after deserialization or corruption.

## BST Deletion

Deletion has three cases:

1. A leaf is detached from its parent.
2. A node with one child is replaced by that child.
3. A node with two children is replaced by its in-order successor or predecessor, then the successor/predecessor is deleted from its original position.

Parent pointers, root updates, and child links must all be updated consistently. A useful validator checks that every node is reachable once, parent links point back to the owner, and keys satisfy lower and upper bounds inherited from ancestors.

For a public mutable tree, avoid exposing raw node indexes without a generation counter. Removing a node and reusing its slot can otherwise make a stale handle refer to a different key.

## Balanced Trees

AVL trees maintain a tighter height bound and usually perform more rotations. Red-black trees allow a looser bound with fewer rotations and are common in general-purpose libraries. Treaps use randomized priorities and simpler balancing logic but do not give a deterministic height bound without additional assumptions.

Balance metadata is part of the invariant:

- AVL: the height difference of child subtrees is within one
- red-black: root/color, red-child, and black-height rules hold
- treap: heap order by priority and BST order by key both hold

For small bounded collections, a sorted array may outperform a pointer-rich balanced tree because of locality and simpler memory ownership. Choose a tree when update cost, range queries, or stable handles justify the complexity.

## Tries And Prefix Trees

A trie indexes keys by symbols rather than comparing whole keys. Lookup is O(k), where `k` is key length, independent of the number of stored keys. It is useful for command dispatch, routing prefixes, dictionary lookup, and completion.

The storage choice matters:

- a full child array is fast but wastes memory for sparse alphabets
- a sorted child list saves memory but adds per-level search
- a bitmap plus packed child array balances locality and density
- a compressed/radix trie merges chains with one child

Define whether keys are bytes, Unicode code points, normalized text, or case-sensitive strings. A trie cannot correct an ambiguous key encoding after insertion.

### Python: Prefix Lookup

```python
class TrieNode:
    def __init__(self):
        self.children = {}
        self.terminal = False


class Trie:
    def __init__(self):
        self.root = TrieNode()

    def insert(self, word):
        node = self.root
        for symbol in word:
            node = node.children.setdefault(symbol, TrieNode())
        node.terminal = True

    def has_prefix(self, prefix):
        node = self.root
        for symbol in prefix:
            node = node.children.get(symbol)
            if node is None:
                return False
        return True

    def words_with_prefix(self, prefix):
        node = self.root
        for symbol in prefix:
            node = node.children.get(symbol)
            if node is None:
                return []
        result = []

        def visit(current, suffix):
            if current.terminal:
                result.append(prefix + suffix)
            for symbol in sorted(current.children):
                visit(current.children[symbol], suffix + symbol)

        visit(node, "")
        return result
```

The recursive `visit` depth is proportional to the longest key. A C implementation should either bound key length or use an explicit stack when stack usage is part of the contract.

## Lowest Common Ancestor

The lowest common ancestor (LCA) of two nodes is their deepest shared ancestor. For a tree with parent pointers, mark the ancestors of one node and walk the other upward. With known depths, raise the deeper node first and then walk both upward.

For repeated queries on a static tree, binary lifting stores ancestors at powers of two and answers an LCA query in O(log n) after O(n log n) preprocessing. Euler-tour plus range-minimum-query methods offer different memory and query tradeoffs. Do not build advanced indexes for a handful of queries on a small tree.

## Subtree Aggregation And Tree DP

Post-order traversal computes values from children before parents. Examples include:

- subtree size and sum
- maximum depth below each node
- total resource demand of a dependency subtree
- whether a subtree satisfies a policy
- maximum independent-set value on a tree

The invariant is that when a node is finalized, every child contribution has already been validated and aggregated. For trees represented by indexes, detect invalid child references and repeated visits before trusting the result.

## Common Mistakes

- Assuming a BST remains logarithmic without a balancing or input-order policy.
- Handling duplicate keys differently in search and insertion.
- Forgetting one of the three deletion cases or failing to update the root.
- Exposing reusable array indexes as permanent node handles.
- Choosing a full trie child array without accounting for alphabet size.
- Using recursive traversal with unbounded input depth.
- Answering LCA queries across different trees as if they shared an ancestor.
- Aggregating a child before checking that it is valid and visited once.

## Embedded And Systems Angle

- calculate maximum node count, height, and metadata bytes before choosing pointers
- use index-backed nodes and free lists when serialization or relocation matters
- compare a sorted array with a balanced tree for small read-mostly collections
- bound key length and trie branching storage
- use explicit stacks for untrusted or deeply skewed trees
- make duplicate, stale-handle, deletion, and pool-exhaustion status visible

## Review Checklist

- What ordering and duplicate invariant does the tree promise?
- Is height bounded by construction, by input policy, or not at all?
- Are parent, child, root, free-list, and generation metadata consistent?
- Is the query count high enough to justify LCA or prefix indexes?
- Are aggregation and traversal depths bounded?
- Can validation detect cycles or repeated references in a supposedly acyclic tree?

## Related Topics

- [Tree Algorithms](index.md)
- [Tree Representations](tree-representations.md)
- [Tree Traversals](tree-traversals.md)
- [Data Modeling And Abstract Data Types](../algorithmic-foundations/data-modeling-and-abstract-data-types.md)
- [Heaps And Priority Queues](../data-structures-for-algorithms/heaps-and-priority-queues.md)
- [Memory Pools And Fixed-Size Allocators](../data-structures-for-algorithms/memory-pools-and-fixed-size-allocators.md)
