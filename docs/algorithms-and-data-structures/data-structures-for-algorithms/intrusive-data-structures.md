---
status: draft
reviewed: false
domain: algorithms-data-models
difficulty: intermediate
last_reviewed: null
---

# Intrusive Data Structures

An intrusive data structure stores its linkage fields inside the objects being linked. A task may contain its own list node; a packet may contain a queue link; a timer object may contain a tree hook.

Intrusive structures avoid a separate allocation per link and make object lifetime explicit. The cost is that the object type must reserve linkage fields and the same object cannot be in incompatible containers through one link at the same time.

## Ownership Model

The container does not own the object merely because it links the object. The caller or subsystem that owns the object must keep it alive until removal.

Define:

- whether an object may be linked once or in multiple containers
- which link field belongs to which container
- whether insertion rejects already-linked objects
- who removes the object before destruction
- whether removal while iterating is supported

An intrusive list has a useful invariant:

- the list head is either empty or linked to a valid object
- every forward link has a matching backward link
- no object appears twice in the same list
- an object is not freed while linked

## Sentinel Versus Null-Terminated Lists

A circular sentinel head avoids special cases for inserting or removing the first and last object. A null-terminated list may use less conceptual machinery but requires head and tail cases.

The sentinel itself is not an object in the application collection. Container iteration must stop when it reaches the sentinel address.

## Programming Examples

### C: Intrusive Doubly Linked List

```c
#include <stddef.h>

struct list_link {
    struct list_link *next;
    struct list_link *previous;
};

struct work_item {
    int priority;
    int value;
    struct list_link link;
};

struct work_list {
    struct list_link head;
};

static void list_init(struct work_list *list)
{
    list->head.next = &list->head;
    list->head.previous = &list->head;
}

static int list_is_empty(const struct work_list *list)
{
    return list->head.next == &list->head;
}

static void list_insert_before(struct list_link *position,
                               struct list_link *link)
{
    link->previous = position->previous;
    link->next = position;
    position->previous->next = link;
    position->previous = link;
}

static void list_remove(struct list_link *link)
{
    link->previous->next = link->next;
    link->next->previous = link->previous;
    link->next = NULL;
    link->previous = NULL;
}

static struct work_item *work_item_from_link(struct list_link *link)
{
    return (struct work_item *)((unsigned char *)link -
                                offsetof(struct work_item, link));
}

static void work_insert_by_priority(struct work_list *list,
                                    struct work_item *item)
{
    struct list_link *position = list->head.next;

    while (position != &list->head) {
        struct work_item *current = work_item_from_link(position);

        if (item->priority < current->priority)
            break;
        position = position->next;
    }
    list_insert_before(position, &item->link);
}

static struct work_item *work_pop_front(struct work_list *list)
{
    struct list_link *link;

    if (list_is_empty(list))
        return NULL;
    link = list->head.next;
    list_remove(link);
    return work_item_from_link(link);
}
```

The example inserts lower priorities first and removes the front item. It assumes the caller initializes `item->link` before first insertion and does not insert an already-linked item. A production API can add a linked flag or poison values in debug builds.

### C: Safe Removal While Iterating

```c
void remove_nonpositive(struct work_list *list)
{
    struct list_link *position;
    struct list_link *next;

    if (list == NULL)
        return;
    for (position = list->head.next;
         position != &list->head;
         position = next) {
        struct work_item *item = work_item_from_link(position);

        next = position->next;
        if (item->value <= 0)
            list_remove(position);
    }
}
```

Save the next link before removal. If the object is also freed, the saved link must point to a still-live list link, not into the object being destroyed.

### Python: Explicit Link Reference

```python
class Link:
    def __init__(self):
        self.next = self
        self.previous = self


class WorkItem:
    def __init__(self, value):
        self.value = value
        self.link = Link()


def unlink(link):
    link.previous.next = link.next
    link.next.previous = link.previous
    link.next = None
    link.previous = None
```

Python does not need a container-of operation because object references are direct, but the ownership and removal rules are the same.

## Multiple Intrusive Memberships

An object can participate in more than one container only with separate link fields:

```c
struct packet {
    struct list_link free_link;
    struct list_link ready_link;
};
```

Do not reuse one link field for a free list and a ready queue. The second insertion would overwrite the first container's links.

## Algorithmic Tradeoffs

Intrusive lists provide O(1) insertion/removal after a known position and avoid node allocation. They still have O(n) search and poor locality compared with arrays. Intrusive heaps or trees can avoid allocation too, but their swap or rotation operations must update object position metadata when handles are exposed.

## Common Mistakes

- Freeing an object while its link remains in a container.
- Inserting one link into two containers at the same time.
- Removing a node and then using its cleared links for iteration.
- Calling container recovery with the wrong containing type or link field.
- Forgetting to initialize a sentinel before the first operation.
- Assuming intrusive means thread-safe or ownership-safe automatically.

## Embedded And Systems Angle

- use intrusive structures when allocation and ownership must be explicit
- ensure an object is not linked in incompatible containers at once
- define lifetime rules for removal and cleanup
- use separate link fields for separate memberships
- add debug checks for linked state, sentinel integrity, and double removal

## Related Topics

- [Data Structures For Algorithms](index.md)
- [Linked Lists Stacks And Queues](linked-lists-stacks-and-queues.md)
- [Memory Pools And Fixed-Size Allocators](memory-pools-and-fixed-size-allocators.md)
- [Bounded Memory And Allocation Failure](../embedded-linux-algorithmic-constraints/bounded-memory-and-allocation-failure.md)
