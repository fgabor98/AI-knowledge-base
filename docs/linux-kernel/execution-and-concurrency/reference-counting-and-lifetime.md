---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Reference Counting And Lifetime

## What Problem Does This Solve?

Kernel objects often outlive the function that created them. Reference counting prevents use-after-free and premature teardown.

Driver code is full of asynchronous boundaries:

- IRQ handlers
- threaded IRQ handlers
- timers
- hrtimers
- workqueues
- file descriptors
- sysfs callbacks
- runtime PM callbacks
- subsystem unregister callbacks

A pointer that crosses one of those boundaries needs a lifetime rule. Locks protect concurrent access to state. References protect the existence of the object.

Those are different problems.

## Core Concepts

- object ownership
- kref
- device references
- file private data
- callback lifetime
- teardown state
- use-after-free
- double free

## Mental Model

Every pointer crossing an async boundary needs a lifetime story: who owns it, who can use it, and what prevents it from being freed too early.

Ask four questions:

```text
Who allocated this object?
Who owns the initial reference?
Which callbacks can use it later?
What must happen before the final free?
```

Then make teardown follow that story.

## Locks Versus Lifetime

A lock does not keep memory alive unless the code also guarantees the object cannot be freed while the lock is acquired.

Wrong mental model:

```text
I take priv->lock, so priv cannot be freed.
```

Correct mental model:

```text
I need a valid priv pointer before I can take priv->lock.
Something else must guarantee priv still exists.
```

That "something else" may be:

- device core lifetime
- file descriptor reference
- kref/refcount
- work item ownership
- RCU, in advanced cases
- teardown ordering that stops all users before free

## Common Driver Lifetimes

| Object | Typical Owner | Common Lifetime End |
| --- | --- | --- |
| device private data from probe | bound device | after remove and devm cleanup |
| character device open state | file descriptor | release |
| queued work item owner | containing driver object | after work canceled/flushed |
| timer owner | containing driver object | after timer shutdown/cancel |
| userspace buffer | syscall duration or pinned pages | after copy/unpin |
| `struct device` pointer | device core refcount | after `put_device()` |

The table is only a starting point. Subsystems can add their own rules.

## Device-Managed Allocation Is Not Async Cleanup

`devm_kzalloc()` frees memory automatically when the device is detached, but it does not make async callbacks disappear.

Wrong:

```c
priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
INIT_WORK(&priv->work, demo_work_fn);
schedule_work(&priv->work);

/* remove returns without canceling work */
```

The devm memory may be freed after remove returns while the work can still run.

Better:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    WRITE_ONCE(priv->stopping, true);
    disable_irq(priv->irq);
    cancel_work_sync(&priv->work);
    timer_shutdown_sync(&priv->timer);
}
```

Device-managed allocation reduces error-path boilerplate. It does not replace teardown synchronization.

## Teardown Ordering

A safe teardown usually follows this shape:

```text
mark object stopping
block new users
stop hardware from producing new events
disable/free IRQs or make handlers harmless
stop timers and hrtimers
cancel delayed work
cancel normal work
wake sleeping waiters
unregister userspace/subsystem interfaces
wait for active references/users
free memory
```

The exact order depends on which callback can queue or rearm which other callback.

Draw the graph:

```text
IRQ -> queues work
timer -> queues work
work -> rearms timer
read() -> waits on wait queue
remove -> must stop all of them
```

Then break the graph during teardown so nothing can reach freed memory.

## File Private Data

Character drivers commonly store driver state in `file->private_data`.

Open:

```c
static int demo_open(struct inode *inode, struct file *filp)
{
    struct demo_priv *priv =
        container_of(inode->i_cdev, struct demo_priv, cdev);

    filp->private_data = priv;
    return 0;
}
```

This is only safe if `priv` cannot be freed while the file is open. Device removal can race with open files, so real drivers need a lifetime rule.

There is also a lookup-lifetime problem: `open()` must get from `inode` to `priv` while `priv` is still valid. Character-device registration, parent device lifetime, locking, or another lookup mechanism must guarantee that the object remains alive long enough for `open()` to take its own reference. The examples below focus on the driver-owned reference after lookup succeeds.

## Blocking New Opens During Removal

Use a removal flag protected by a lock.

```c
static int demo_open(struct inode *inode, struct file *filp)
{
    struct demo_priv *priv =
        container_of(inode->i_cdev, struct demo_priv, cdev);
    int ret = 0;

    mutex_lock(&priv->open_lock);
    if (priv->going_away)
        ret = -ENODEV;
    else
        filp->private_data = priv;
    mutex_unlock(&priv->open_lock);

    return ret;
}
```

Remove:

```c
mutex_lock(&priv->open_lock);
priv->going_away = true;
mutex_unlock(&priv->open_lock);

cdev_del(&priv->cdev);
```

This blocks future opens, but it does not handle files already open. Existing file descriptors still need valid state or an explicit reference.

## Reference Counting With `kref`

`kref` is a common object lifetime helper.

```c
struct demo_priv {
    struct kref ref;
    struct mutex lock;
    bool going_away;
};
```

Release function:

```c
static void demo_release_ref(struct kref *ref)
{
    struct demo_priv *priv = container_of(ref, struct demo_priv, ref);

    kfree(priv);
}
```

Get and put helpers:

```c
static struct demo_priv *demo_get(struct demo_priv *priv)
{
    kref_get(&priv->ref);
    return priv;
}

static void demo_put(struct demo_priv *priv)
{
    kref_put(&priv->ref, demo_release_ref);
}
```

Initialize:

```c
kref_init(&priv->ref);
```

The initial reference is usually owned by the device binding or parent object. Open files, queued work, or other async users take additional references.

## File References With `kref`

Open:

```c
static int demo_open(struct inode *inode, struct file *filp)
{
    struct demo_priv *priv =
        container_of(inode->i_cdev, struct demo_priv, cdev);
    int ret = 0;

    mutex_lock(&priv->open_lock);
    if (priv->going_away) {
        ret = -ENODEV;
    } else {
        demo_get(priv);
        filp->private_data = priv;
    }
    mutex_unlock(&priv->open_lock);

    return ret;
}
```

Release:

```c
static int demo_release(struct inode *inode, struct file *filp)
{
    struct demo_priv *priv = filp->private_data;

    demo_put(priv);
    return 0;
}
```

Remove drops the initial reference after unregistering and stopping async work:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    mutex_lock(&priv->open_lock);
    priv->going_away = true;
    mutex_unlock(&priv->open_lock);

    cdev_del(&priv->cdev);
    WRITE_ONCE(priv->stopping, true);
    wake_up_interruptible_all(&priv->read_wq);
    cancel_work_sync(&priv->work);
    timer_shutdown_sync(&priv->timer);

    demo_put(priv); /* drop initial/device reference */
}
```

The object is freed only after all open files and async users drop their references.

## Work Item References

If work can outlive the caller that queued it, hold a reference while it is pending or running.

You still need a valid pointer before trying to take a reference. `kref_get_unless_zero()` is useful when the pointer is obtained under a lookup lock, RCU read-side protection, or another rule that keeps the memory from being freed during the get attempt.

```c
static bool demo_queue_work(struct demo_priv *priv)
{
    if (!kref_get_unless_zero(&priv->ref))
        return false;

    if (!queue_work(system_wq, &priv->work)) {
        demo_put(priv);
        return false;
    }

    return true;
}
```

Work function:

```c
static void demo_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, work);

    if (!READ_ONCE(priv->stopping))
        demo_do_work(priv);

    demo_put(priv);
}
```

This pattern is useful when the object's lifetime is independent from device-managed memory. If the work item is embedded in a devm-allocated object tied to device removal, the simpler and more common pattern is to cancel the work before remove returns.

Be careful with coalesced work. If `queue_work()` returns false because the work is already pending, the new reference must be dropped.

## Timer And Hrtimer Lifetime

Timers are dangerous because they can fire after the function that armed them has returned.

Rule:

```text
the object containing the timer must remain alive until the timer is synchronously stopped and cannot be rearmed
```

Remove example:

```c
WRITE_ONCE(priv->stopping, true);
timer_shutdown_sync(&priv->timer);
hrtimer_cancel(&priv->hrtimer);
```

If a work item can rearm the timer, cancel or stop that work too.

```c
WRITE_ONCE(priv->stopping, true);
cancel_work_sync(&priv->rearm_work);
timer_shutdown_sync(&priv->timer);
```

The correct order depends on the rearm graph.

## IRQ Handler Lifetime

An IRQ handler receives `dev_id`, often your driver object:

```c
devm_request_irq(dev, irq, demo_irq, 0, dev_name(dev), priv);
```

The object must remain valid while the IRQ can call the handler.

Teardown must ensure:

- hardware no longer produces interrupts, or handler is harmless
- IRQ is disabled/freed before memory can be freed
- work queued by the IRQ is canceled

With device-managed IRQs, the free happens during devm cleanup. You still need to stop dependent work and timers in the right order before the backing state becomes invalid.

## Device References

If code stores a `struct device *` beyond the immediate call path, it may need a device reference.

```c
get_device(dev);
priv->dev = dev;
```

Drop:

```c
put_device(priv->dev);
```

Many drivers use the device pointer only while the parent device lifetime is already guaranteed by probe/remove ordering. Do not add device references everywhere by habit. Add them when an async object may outlive the normal device callback lifetime.

## `refcount_t`

`refcount_t` is a lower-level reference count primitive.

```c
struct demo_obj {
    refcount_t refs;
};
```

Initialize:

```c
refcount_set(&obj->refs, 1);
```

Get:

```c
if (!refcount_inc_not_zero(&obj->refs))
    return NULL;
```

Put:

```c
if (refcount_dec_and_test(&obj->refs))
    kfree(obj);
```

Use `kref` when you want a structured release callback. Use `refcount_t` when the object model is simple and the release path is explicit.

Do not use `atomic_t` for object lifetime in new code.

## Active Users Without Full Object Refs

Sometimes remove needs to wait for operations in progress, but the object lifetime itself is already guaranteed.

Example:

```c
struct demo_priv {
    atomic_t active_ops;
    wait_queue_head_t active_wq;
    bool stopping;
};
```

Begin operation:

```c
if (READ_ONCE(priv->stopping))
    return -ENODEV;

atomic_inc(&priv->active_ops);

if (READ_ONCE(priv->stopping)) {
    if (atomic_dec_and_test(&priv->active_ops))
        wake_up(&priv->active_wq);
    return -ENODEV;
}
```

End operation:

```c
if (atomic_dec_and_test(&priv->active_ops))
    wake_up(&priv->active_wq);
```

Remove:

```c
WRITE_ONCE(priv->stopping, true);
wait_event(priv->active_wq, atomic_read(&priv->active_ops) == 0);
```

This pattern must be designed carefully to avoid a race where a new operation starts after remove checks the count. A mutex around "check stopping and increment active" is often clearer.

## Teardown State

A simple `stopping` or `going_away` flag is often necessary but not sufficient.

Use it to:

- reject new operations
- make callbacks return early
- stop self-requeueing work
- wake waiters with an error
- prevent timer rearming

Example:

```c
if (READ_ONCE(priv->stopping))
    return -ENODEV;
```

For compound state, protect the flag with the same lock as the state machine:

```c
mutex_lock(&priv->lock);
if (priv->stopping) {
    mutex_unlock(&priv->lock);
    return -ENODEV;
}
demo_start_transaction_locked(priv);
mutex_unlock(&priv->lock);
```

## Use-After-Free Pattern

Common sequence:

```text
probe allocates priv
IRQ queues work
remove frees priv
work runs and uses priv
```

Fix:

```text
remove sets stopping
remove disables IRQ
remove cancels work synchronously
remove frees priv only after cancellation returns
```

Code:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    WRITE_ONCE(priv->stopping, true);
    disable_irq(priv->irq);
    cancel_work_sync(&priv->irq_work);
}
```

If the work can be queued from more than IRQ, stop those producers too.

## Double Free Pattern

Double free happens when ownership is unclear.

Wrong:

```c
demo_put(obj);
kfree(obj);
```

If `demo_put()` can run the release callback, the direct `kfree()` is a second free.

Better:

```c
demo_put(obj);
obj = NULL;
```

After transferring ownership, the old owner should not free the object.

## Ownership Transfer

Make ownership transfer explicit.

Example:

```c
cmd = kzalloc(sizeof(*cmd), GFP_KERNEL);
if (!cmd)
    return -ENOMEM;

INIT_WORK(&cmd->work, demo_cmd_work_fn);

if (!queue_work(priv->wq, &cmd->work)) {
    kfree(cmd);
    return -EBUSY;
}

cmd = NULL; /* work function now owns it */
```

Work function:

```c
static void demo_cmd_work_fn(struct work_struct *work)
{
    struct demo_cmd *cmd = container_of(work, struct demo_cmd, work);

    demo_execute_cmd(cmd);
    kfree(cmd);
}
```

This avoids two owners believing they should free the same object.

## Debugging Lifetime Bugs

Use development configs:

```text
CONFIG_KASAN
CONFIG_KCSAN
CONFIG_DEBUG_OBJECTS_TIMERS
CONFIG_DEBUG_OBJECTS_WORK
CONFIG_REFCOUNT_FULL
CONFIG_DEBUG_KOBJECT_RELEASE
```

Useful symptoms:

- crash after module unload
- crash after unplug/remove
- crash when closing the last file descriptor
- warning from debugobjects about active timers/work
- KASAN use-after-free report
- refcount saturation warning

When reading a crash, identify:

```text
freed object type
callback that used it
who should have canceled or referenced it
which teardown path missed it
```

## Practice Exercises

### Exercise 1: Async Boundary Map

For a driver object, list every pointer crossing:

```text
IRQ
timer
workqueue
file private_data
sysfs
runtime PM
```

For each pointer, write what keeps it alive.

### Exercise 2: Open File Lifetime

Add a `going_away` flag and reference counting to a character device so open files remain safe after device removal begins.

### Exercise 3: Work Lifetime

Take a work item that can run after the queuer returns. Either:

```text
cancel it before object free
```

or:

```text
hold a reference while it is pending/running
```

Explain which approach fits better.

## Debugging Checklist

- Check all async callbacks.
- Check open file descriptors during device removal.
- Use KASAN-enabled kernels during development.
- Avoid freeing state before work, timers, IRQs, and users are stopped.
- Remember that devm allocation does not cancel work or timers for you.
- Use `kref` or `refcount_t` for object references.
- Block new users before waiting for existing users to drain.
- Wake sleeping waiters during teardown.
- Draw the callback graph before writing remove.

## Related Topics

- [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)
- [Workqueues](workqueues.md)
- [Oops, Panic, And Crash Logs](../debugging/oops-panic-crash-logs.md)
- [Timers](timers.md)
- [Wait Queues And Completions](wait-queues-and-completions.md)

## Official References

- [Adding reference counters: krefs](https://docs.kernel.org/core-api/kref.html)
- [refcount_t API compared to atomic_t](https://docs.kernel.org/core-api/refcount-vs-atomic.html)
- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
