---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Locking And Atomics

## What Problem Does This Solve?

Locking protects shared state from concurrent access across CPUs, interrupts, timers, workqueues, and userspace-facing callbacks.

In driver code, the hard part is rarely "which lock API exists." The hard part is knowing every context that can touch a piece of state and choosing a primitive that is legal in all of those contexts.

Examples of shared state:

- device register cache
- RX/TX queues
- interrupt status bits
- open count
- removal flag
- current power state
- workqueue state machine
- buffer ownership
- wait-queue condition
- reference count

If two callbacks can access the same state at the same time, the driver needs a synchronization story.

## Core Concepts

- mutex
- spinlock
- rwsem
- atomic variables
- memory ordering overview
- lock ordering
- lockdep
- interrupt-safe locking

## Mental Model

Choose the simplest lock that is legal in all contexts that touch the state. Then document and preserve lock ordering.

Start with an access map:

```text
state: rx_ready
accessed by:
  hard IRQ handler
  read()
  poll()
  remove()

requirements:
  IRQ handler cannot sleep
  read() may sleep while waiting
  state update must wake waiters

primitive:
  spinlock for rx_ready and queue metadata
  wait queue for sleeping readers
```

Do not start with "I like mutexes" or "atomics are faster." Start with context.

## Lock Selection Table

| Shared State Touched By | Common Primitive | Why |
| --- | --- | --- |
| only process context | `struct mutex` | simple, sleepable, good for device transactions |
| process context and workqueue | `struct mutex` | both may sleep |
| hard IRQ and process context | `spinlock_t` with IRQ-safe variants | hard IRQ cannot sleep |
| timer callback and process context | `spinlock_t` | timer callback cannot sleep |
| simple independent counter | `atomic_t` or `atomic64_t` | one variable, one atomic operation |
| object reference count | `refcount_t` or `kref` | refcount semantics and overflow hardening |
| many readers, rare sleepable writers | `struct rw_semaphore` | sleepable read/write locking |
| one-time event wait | `struct completion` | not a general data lock |
| condition wait | wait queue plus condition lock | wakeups are notifications |

The primitive must be legal for the most constrained context that touches the state.

## Mutexes

Mutexes are sleepable locks. They are the default choice for process-context driver state.

Use mutexes in:

- probe/remove
- file operations
- sysfs show/store
- workqueue callbacks
- threaded IRQ handlers
- runtime PM callbacks, subject to subsystem rules

Do not use mutexes in:

- hard IRQ handlers
- timer callbacks
- hrtimer callbacks
- code holding a spinlock
- atomic context

Example:

```c
struct demo_priv {
    struct mutex lock;
    u8 config;
};

static ssize_t mode_store(struct device *dev,
                          struct device_attribute *attr,
                          const char *buf, size_t count)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    unsigned long mode;
    int ret;

    ret = kstrtoul(buf, 0, &mode);
    if (ret)
        return ret;

    mutex_lock(&priv->lock);
    ret = demo_program_mode(priv, mode);
    if (!ret)
        priv->config = mode;
    mutex_unlock(&priv->lock);

    return ret ? ret : count;
}
```

This is appropriate because sysfs store runs in process context and `demo_program_mode()` may sleep.

## Interruptible Mutex Locking

If a file operation may block for a long time acquiring a mutex, consider interruptible locking.

```c
ret = mutex_lock_interruptible(&priv->lock);
if (ret)
    return ret;
```

This lets a signal interrupt the wait. It is useful for user-facing paths where indefinite uninterruptible sleep is unfriendly.

Do not use interruptible locking blindly inside internal paths where callers cannot handle `-EINTR` correctly.

## Spinlocks

Spinlocks are atomic-context locks. They do not sleep. Hold them for short critical sections only.

Use spinlocks for state shared with:

- hard IRQ handlers
- timer callbacks
- hrtimer callbacks
- other atomic contexts

Example:

```c
struct demo_priv {
    spinlock_t lock;
    bool data_ready;
    u32 irq_status;
};
```

IRQ handler:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->irq_status = demo_read_irq_status(priv);
    priv->data_ready = true;
    spin_unlock_irqrestore(&priv->lock, flags);

    wake_up_interruptible(&priv->wait);
    return IRQ_HANDLED;
}
```

Reader:

```c
spin_lock_irqsave(&priv->lock, flags);
ready = priv->data_ready;
status = priv->irq_status;
spin_unlock_irqrestore(&priv->lock, flags);
```

The reader uses `irqsave` because the same lock is taken by the hard IRQ handler. If the reader took the lock with local IRQs enabled and the device interrupted on the same CPU, the IRQ handler could spin waiting for a lock held by the interrupted code.

## Spinlock Rules

While holding a spinlock, do not:

- sleep
- call `mutex_lock()`
- call I2C/SPI synchronous transfers
- allocate with `GFP_KERNEL`
- call `copy_to_user()` or `copy_from_user()`
- call `msleep()` or `usleep_range()`
- perform unbounded loops

Keep the critical section small:

```c
spin_lock_irqsave(&priv->lock, flags);
status = priv->irq_status;
priv->irq_status = 0;
spin_unlock_irqrestore(&priv->lock, flags);

demo_process_status_sleepable(priv, status);
```

The spinlock protects the shared status variable. The sleepable processing happens after unlocking.

## Choosing Spinlock Variants

| API | Use |
| --- | --- |
| `spin_lock()` | lock not used by IRQ on same CPU and IRQ state does not matter |
| `spin_lock_irq()` | disable local IRQs while locked |
| `spin_lock_irqsave()` | save current IRQ state and disable local IRQs |
| `spin_lock_bh()` | protect against softirq/bottom-half users |

In ordinary drivers, `spin_lock_irqsave()` is often the conservative choice for state shared with hard IRQ handlers.

Use the matching unlock:

```c
spin_lock_irqsave(&priv->lock, flags);
...
spin_unlock_irqrestore(&priv->lock, flags);
```

Do not mix variants casually. The lock and unlock must match.

## Bottom-Half Locking

If state is shared with softirq or tasklet context, use bottom-half-aware locking.

```c
spin_lock_bh(&priv->lock);
...
spin_unlock_bh(&priv->lock);
```

Ordinary drivers more often use threaded IRQs or workqueues, but older tasklet-based drivers still need this pattern.

## Reader/Writer Semaphores

`struct rw_semaphore` is a sleepable reader/writer lock.

Use it when:

- readers are frequent
- writers are rare
- all users are sleepable
- read-side critical sections may sleep

Example:

```c
down_read(&priv->config_sem);
demo_copy_config(snapshot, &priv->config);
up_read(&priv->config_sem);

down_write(&priv->config_sem);
demo_update_config(&priv->config, new_config);
up_write(&priv->config_sem);
```

Do not use `rw_semaphore` in hard IRQ or timer callbacks.

For many drivers, a mutex is simpler and good enough. Use reader/writer locking only when it solves a measured or structurally clear problem.

## Atomics

Atomic variables are for operations that are truly atomic enough by themselves.

Good atomic use:

```c
atomic_t irq_count;

atomic_inc(&priv->irq_count);
count = atomic_read(&priv->irq_count);
```

Good use with a simple state bit:

```c
if (atomic_xchg(&priv->reset_pending, 1) == 0)
    schedule_work(&priv->reset_work);
```

Bad atomic use:

```c
if (atomic_read(&priv->available) > 0) {
    atomic_dec(&priv->available);
    use_buffer(priv);
}
```

The check and the buffer use are a compound invariant. Another CPU can change the state between operations. Use a lock or a proper atomic compare/exchange pattern and protect the associated data.

## `atomic_t` Is Not A Lock

Atomics do not protect related fields.

Wrong:

```c
atomic_set(&priv->ready, 1);
priv->buffer = buf;
```

A reader that sees `ready` may not have a coherent lifetime or ordering story for `buffer`.

Better:

```c
spin_lock_irqsave(&priv->lock, flags);
priv->buffer = buf;
priv->ready = true;
spin_unlock_irqrestore(&priv->lock, flags);

wake_up_interruptible(&priv->wait);
```

If you intentionally write lockless code, use explicit memory-ordering primitives and explain them in comments. Most beginner driver code should prefer locks.

## Reference Counts

Use `refcount_t` or `kref` for object lifetime, not plain `atomic_t`.

```c
struct demo_obj {
    struct kref ref;
};
```

`refcount_t` and `kref` express lifetime semantics and provide stronger checking against overflow and misuse.

Use atomics for counters. Use refcount primitives for object references.

## `READ_ONCE()` And `WRITE_ONCE()`

Use `READ_ONCE()` and `WRITE_ONCE()` for simple lockless variables where you need to prevent compiler inventiveness around repeated loads or stores.

Example stopping flag:

```c
WRITE_ONCE(priv->stopping, true);

if (READ_ONCE(priv->stopping))
    return;
```

This does not make compound state safe. It is appropriate for simple flags where all code is designed around that lockless use.

If the flag orders access to other data, you may need acquire/release semantics or a lock.

## Memory Ordering Overview

Locks provide ordering. If both writer and reader use the same lock, writes before unlock are visible to readers after lock.

```c
spin_lock_irqsave(&priv->lock, flags);
priv->ready = true;
priv->value = value;
spin_unlock_irqrestore(&priv->lock, flags);
```

The matching locked reader sees coherent state:

```c
spin_lock_irqsave(&priv->lock, flags);
ready = priv->ready;
value = priv->value;
spin_unlock_irqrestore(&priv->lock, flags);
```

Lockless publication needs explicit ordering:

```c
priv->value = value;
smp_store_release(&priv->ready, true);
```

Reader:

```c
if (smp_load_acquire(&priv->ready))
    use_value(priv->value);
```

Do not introduce lockless memory ordering until you can explain why a normal lock is unsuitable. It is easy to get wrong and hard to test.

## Lock Ordering

Deadlocks often come from inconsistent lock order.

Bad:

```text
path A: lock device_lock, then buffer_lock
path B: lock buffer_lock, then device_lock
```

Good:

```text
always take device_lock before buffer_lock
```

Document lock ordering near the structure or in a short comment near the locks:

```c
struct demo_priv {
    /*
     * Lock order:
     *   config_lock -> queue_lock
     */
    struct mutex config_lock;
    spinlock_t queue_lock;
};
```

Avoid nested locks when one lock or a clearer state split is enough.

## Locking With Wait Queues

The condition waited on must be protected consistently.

Producer:

```c
spin_lock_irqsave(&priv->lock, flags);
priv->data_ready = true;
spin_unlock_irqrestore(&priv->lock, flags);

wake_up_interruptible(&priv->wait);
```

Consumer:

```c
ret = wait_event_interruptible(priv->wait,
                               READ_ONCE(priv->data_ready));
if (ret)
    return ret;
```

For compound conditions, use the same lock around condition checks and updates, or use wait-event lock variants where appropriate.

Wakeups do not store state. The condition stores state.

## Locking With Workqueues

Workqueue callbacks may sleep, but they may still share state with atomic contexts.

Pattern:

```c
static void demo_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, work);
    u32 status;
    unsigned long flags;

    spin_lock_irqsave(&priv->irq_lock, flags);
    status = priv->pending_status;
    priv->pending_status = 0;
    spin_unlock_irqrestore(&priv->irq_lock, flags);

    mutex_lock(&priv->io_lock);
    demo_handle_status(priv, status);
    mutex_unlock(&priv->io_lock);
}
```

Use the spinlock only for IRQ-shared state. Use the mutex for sleepable device I/O.

## Locking With Remove

Removal is a concurrent state transition.

Common pattern:

```c
mutex_lock(&priv->lock);
priv->stopping = true;
mutex_unlock(&priv->lock);

disable_irq(priv->irq);
cancel_work_sync(&priv->work);
timer_shutdown_sync(&priv->timer);
```

Do not hold `priv->lock` while canceling work if the work function also needs `priv->lock`.

Wrong:

```c
mutex_lock(&priv->lock);
cancel_work_sync(&priv->work);
mutex_unlock(&priv->lock);
```

This can deadlock if the work is running and waiting for the mutex.

## Lockdep

Lockdep tracks lock dependencies and reports possible deadlocks.

Enable it in development kernels when possible:

```text
CONFIG_LOCKDEP
CONFIG_PROVE_LOCKING
CONFIG_DEBUG_SPINLOCK
CONFIG_DEBUG_MUTEXES
```

Lockdep reports are often warnings about real design bugs even if the system did not hang during the test.

Read lockdep reports for:

- locks involved
- acquisition order
- call sites
- IRQ context markings
- whether the lock was taken with IRQs enabled or disabled

## PREEMPT_RT Note

On PREEMPT_RT kernels, some locking internals differ from non-RT kernels. For early driver learning, keep the portable rule:

```text
do not sleep while holding spinlocks or in callbacks documented as atomic
```

If writing RT-sensitive code, read the current RT locking rules for the target kernel. Some low-level code requires `raw_spinlock_t`, but ordinary drivers should not start there.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| "sleeping function called from invalid context" | slept under spinlock or in IRQ/timer | stack trace and lock held |
| deadlock on remove | cancel/flush while holding callback lock | remove path |
| occasional corrupted queue | missing lock around list/FIFO | all producers/consumers |
| lost state update | atomic used for compound state | invariants |
| lockdep warning | inconsistent lock order or IRQ usage | lockdep trace |
| IRQ handler spins forever | process context took lock without disabling local IRQs | spinlock variant |
| use-after-free despite locks | lifetime not protected | refs and teardown |

## Practice Exercises

### Exercise 1: Access Map

Pick one state variable in a driver and write:

```text
variable:
read by:
written by:
contexts:
may sleep:
lock:
```

Repeat until every shared state variable has an owner and lock.

### Exercise 2: Split Spinlock And Mutex Work

Take a function that holds a spinlock while doing too much work. Split it into:

```text
small locked snapshot
unlocked sleepable processing
locked state update if needed
```

### Exercise 3: Atomic Audit

Find every `atomic_t`. Decide whether it is:

```text
simple counter
state flag
reference count
compound invariant in disguise
```

Replace reference counts with `refcount_t` or `kref`.

## Debugging Checklist

- Check all access paths to shared state.
- Check whether locks can sleep.
- Check lock ordering and nested locks.
- Avoid using atomics as a substitute for protecting compound state.
- Use IRQ-safe spinlock variants for state shared with hard IRQ.
- Do not hold callback locks while canceling work or timers.
- Enable lockdep on development kernels.
- Prefer locks over custom lockless memory ordering while learning.

## Related Topics

- [Context Rules](context-rules.md)
- [Wait Queues And Completions](wait-queues-and-completions.md)
- [Kernel Debugging Basics](../debugging/index.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)

## Official References

- [Unreliable Guide To Locking](https://docs.kernel.org/kernel-hacking/locking.html)
- [refcount_t API compared to atomic_t](https://docs.kernel.org/core-api/refcount-vs-atomic.html)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
