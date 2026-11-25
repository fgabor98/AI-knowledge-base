---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# RCU

## What Problem Does This Solve?

Read-copy-update (RCU) protects data that is read very often and changed rarely. Readers take no lock, write no shared memory, and never wait for an updater. The cost moves to the updater, which must publish a new version and wait before freeing the old one.

A driver developer needs RCU for two reasons:

- core kernel interfaces already use it, so callbacks run inside RCU read-side sections (networking, notifier chains, many lookups)
- some driver data fits it exactly, for example a configuration object that an interrupt path reads for every event

It is not a general replacement for locks. Start with a mutex or spinlock and move to RCU when readers are hot or must not wait.

## Core Concepts

- read-side critical section
- `rcu_read_lock()`, `rcu_read_unlock()`
- `rcu_dereference()`, `rcu_assign_pointer()`, `rcu_replace_pointer()`
- grace period
- `synchronize_rcu()`, `kfree_rcu()`, `call_rcu()`, `rcu_barrier()`
- `__rcu` annotation and sparse
- SRCU

## Mental Model

RCU separates two steps that a lock combines: making old data unreachable, and freeing it.

```text
updater                              readers
-------                              -------
allocate and fill new object
publish pointer to new               new readers see new
(old is now unreachable)             readers already inside still use old
wait for a grace period      ------> every reader that could hold old has left
free old
```

A **grace period** ends when every read-side section that was running at its start has finished. Sections that start later cannot find the old object, so nobody waits for them.

RCU gives each reader a consistent *version*, not the *latest* version. A reader may use the old object for a short time after the update. If that is not acceptable, RCU is the wrong tool.

## Reading

```c
struct demo_cfg {
    u32 rate;
    u32 limit;
    struct rcu_head rcu;
};

struct demo_priv {
    struct demo_cfg __rcu *cfg;
    struct mutex cfg_lock;      /* serializes updaters */
};

static u32 demo_limit(struct demo_priv *priv)
{
    const struct demo_cfg *cfg;
    u32 limit = 0;

    rcu_read_lock();
    cfg = rcu_dereference(priv->cfg);
    if (cfg)
        limit = cfg->limit;
    rcu_read_unlock();

    return limit;
}
```

Rules for the read side:

- Load the pointer with `rcu_dereference()`, once, and use that local copy. Loading it twice can return two different objects.
- Do not use the pointer after `rcu_read_unlock()`. Copy out the values, or take a reference while still inside.
- Do not sleep: no mutex, no `GFP_KERNEL`, no `copy_to_user()`. Preemption of the reader is allowed on preemptible kernels; voluntary blocking is not.
- Read-side sections nest and may be used in hard IRQ context.

To keep an object beyond the section, take a reference that can fail:

```c
rcu_read_lock();
obj = rcu_dereference(priv->obj);
if (obj && !kref_get_unless_zero(&obj->ref))
    obj = NULL;
rcu_read_unlock();
```

## Updating

```c
static int demo_set_cfg(struct demo_priv *priv, u32 rate, u32 limit)
{
    struct demo_cfg *new, *old;

    new = kzalloc(sizeof(*new), GFP_KERNEL);
    if (!new)
        return -ENOMEM;
    new->rate = rate;
    new->limit = limit;

    mutex_lock(&priv->cfg_lock);
    old = rcu_replace_pointer(priv->cfg, new,
                              lockdep_is_held(&priv->cfg_lock));
    mutex_unlock(&priv->cfg_lock);

    if (old)
        kfree_rcu(old, rcu);
    return 0;
}
```

- Fill the new object completely *before* publishing it. `rcu_assign_pointer()` and `rcu_replace_pointer()` order those writes before the pointer store.
- Never modify a published object in place. Readers may be between two of its fields. Publishing a copy is what makes multi-field updates appear atomic.
- RCU does not serialize updaters. Use an ordinary lock among them.
- On the update side use `rcu_dereference_protected(ptr, condition)`, where the condition names the lock. Lockdep checks it.

## Reclaiming The Old Object

| API | Behavior | Use |
| --- | --- | --- |
| `synchronize_rcu()` | blocks until a grace period has passed | process context, occasional updates, when the code after it needs the guarantee |
| `kfree_rcu(ptr, field)` | frees after a grace period, does not block | the usual choice for a plain `kfree()` |
| `call_rcu(head, fn)` | runs `fn` in softirq context after a grace period | cleanup beyond `kfree()`; `fn` must not sleep |
| `synchronize_rcu_expedited()` | faster, disturbs all CPUs | rarely, never from a path userspace can trigger in a loop |

`synchronize_rcu()` usually takes milliseconds. Do not call it under a lock that readers or hot paths need.

### Module Unload

If a module used `call_rcu()` with its own callback, the callback may still be pending when the module is removed. Stop new `call_rcu()` calls first, then wait:

```c
static void __exit demo_exit(void)
{
    demo_unregister();      /* no new updates after this */
    rcu_barrier();          /* wait for all pending call_rcu callbacks */
}
```

`synchronize_rcu()` is not a substitute: it waits for readers, not for callbacks. `kfree_rcu()` needs no `rcu_barrier()` unless the objects come from a `kmem_cache` the module destroys.

## RCU-Protected Lists

The list macros have `_rcu` variants that publish and traverse correctly:

```c
/* reader */
rcu_read_lock();
list_for_each_entry_rcu(item, &priv->items, node)
    demo_visit(item);
rcu_read_unlock();

/* updater */
mutex_lock(&priv->lock);
list_del_rcu(&item->node);
mutex_unlock(&priv->lock);
kfree_rcu(item, rcu);
```

Use `list_add_rcu()` to insert. Do not reinitialize or re-add a removed node before a grace period: a reader may still be following its `next` pointer.

## SRCU: Readers That Sleep

Sleepable RCU (SRCU) allows blocking in the read side. Each `struct srcu_struct` is its own domain, so a slow reader delays only its own users:

```c
int idx;

idx = srcu_read_lock(&priv->srcu);
cfg = srcu_dereference(priv->cfg, &priv->srcu);
ret = demo_slow_transfer(priv, cfg);
srcu_read_unlock(&priv->srcu, idx);
```

Initialize it with `init_srcu_struct()` and free it with `cleanup_srcu_struct()`. SRCU is a common way to block removal while file operations are still running: take it in every operation, set a "gone" flag in `remove()`, then call `synchronize_srcu()`.

## RCU Versus Other Mechanisms

| Situation | Prefer |
| --- | --- |
| process context only, rare access | mutex |
| short critical section shared with an IRQ | spinlock |
| readers must see the newest value | lock |
| read-mostly pointer to an object, readers hot or atomic | RCU |
| read-mostly small values, writer must never wait for readers | [seqlock](locking-and-atomics.md#sequence-locks) |
| readers sleep | SRCU or rwsem |
| object lifetime only | `kref`, often combined with RCU for lookup |

## Runnable Example

`examples/linux-kernel/small-labs/14-rcu` runs two readers against a writer. With `broken=1` the writer edits the object in place and the readers report torn values; in RCU mode they report none.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| use-after-free in a reader | freed without a grace period, or pointer used after unlock | KASAN report, every use after `rcu_read_unlock()` |
| readers see half-updated data | published object modified in place | updater code |
| `suspicious RCU usage` | accessor used without the matching protection | `CONFIG_PROVE_RCU` report names the line |
| RCU stall warning | a reader or an IRQ-off/preempt-off loop runs too long | the CPU and stack in the report |
| crash after `rmmod` | `call_rcu()` callback ran in unloaded code | missing `rcu_barrier()` |
| sleeping function called from invalid context | blocking call in the read side | stack trace; use SRCU if blocking is required |

## Common Mistakes

- Plain assignment instead of `rcu_assign_pointer()`.
- Calling `rcu_dereference()` outside a read-side section.
- Believing that RCU protects against concurrent writers.
- Using RCU for data whose readers must never act on an old value.
- Treating `synchronize_rcu()` as cheap.

## Practice Exercises

1. Run Lab 14 in both modes, then replace `kfree_rcu()` with `kfree()` in a KASAN kernel and explain the report.
2. Convert a mutex-protected configuration structure to RCU. List every reader and state why it may use an old version.
3. In the kernel source, find a driver that uses `synchronize_srcu()` in its remove path and identify what each read-side section protects.

## Debugging Checklist

- Build with `CONFIG_PROVE_RCU`, KASAN, and sparse (`make C=1`); annotate pointers with `__rcu`.
- Is every access made through an RCU accessor?
- Is the object complete before it is published?
- Is there a grace period between unpublishing and freeing?
- Does module exit stop new callbacks and then call `rcu_barrier()`?
- Can the read side sleep, directly or through a called function?

## Related Topics

- [Locking And Atomics](locking-and-atomics.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)
- [Kernel Threads](kernel-threads.md)
- [Network Device Drivers](../driver-interfaces/network-device-drivers.md)

## Official References

- [What is RCU?](https://docs.kernel.org/RCU/whatisRCU.html)
- [Review checklist for RCU patches](https://docs.kernel.org/RCU/checklist.html)
- [RCU and unloadable modules](https://docs.kernel.org/RCU/rcubarrier.html)
- [Using RCU to protect read-mostly linked lists](https://docs.kernel.org/RCU/listRCU.html)
- [Using RCU's CPU stall detector](https://docs.kernel.org/RCU/stallwarn.html)
