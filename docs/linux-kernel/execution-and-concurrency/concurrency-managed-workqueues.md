---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Concurrency Managed Workqueues

## What Problem Does This Solve?

Concurrency managed workqueues let the kernel manage worker creation and concurrency so deferred work can run efficiently without each driver creating ad hoc threads.

The key idea is that a workqueue is not simply "my private thread." Modern workqueues separate:

- the workqueue object visible to drivers
- worker pools managed by the kernel
- per-CPU or unbound execution policy
- attributes such as priority, reclaim behavior, and concurrency limits

This lets many subsystems share worker infrastructure while still giving drivers control when they need it.

## Core Concepts

- work item
- worker pool
- shared workqueue
- dedicated workqueue
- ordered workqueue
- delayed work
- flushing
- cancellation
- CPU affinity overview

## Mental Model

Use the common workqueue infrastructure unless the driver has a clear ordering, concurrency, or isolation requirement that needs a dedicated queue.

```text
work item:
  callback to run later

workqueue:
  queueing domain, attributes, flush domain, forward-progress domain

worker pool:
  backend kworkers that actually execute work

cmwq:
  kernel-managed worker pools shared across workqueues
```

Do not assume one workqueue equals one thread. That assumption leads to wrong ordering and concurrency reasoning.

## Why CMWQ Exists

Older workqueue designs could waste many worker threads while still providing poor concurrency. Concurrency managed workqueues solve this by sharing backend worker pools and letting the kernel create enough workers to keep progress without creating a thread per small driver need.

For driver authors, the practical result is:

- default system workqueues are often sufficient
- dedicated workqueues are for specific policy needs
- `max_active` limits concurrency, but does not replace proper locking
- ordered workqueues are explicit
- reclaim-sensitive work must use `WQ_MEM_RECLAIM`

## User-Facing Workqueues Versus Worker Pools

When a driver calls:

```c
queue_work(priv->wq, &priv->work);
```

it queues a work item to a workqueue object. The work item is then executed by a worker selected according to the workqueue's attributes.

Driver code should reason about:

- what workqueue the item is queued on
- whether that queue is bound or unbound
- whether multiple instances can run concurrently
- whether ordering matters
- whether the queue has reclaim guarantees
- when the queue is flushed or destroyed

Driver code should not rely on:

- a stable worker thread identity
- exactly one thread existing for the queue
- unrelated work ordering
- implicit serialization unless the queue guarantees it

## System Workqueues

Common system queues include:

| Queue | Typical Use |
| --- | --- |
| `system_wq` | general short work |
| `system_highpri_wq` | high-priority short work |
| `system_long_wq` | longer-running work that should not block normal short work |
| `system_unbound_wq` | work that should not be CPU-local |
| `system_freezable_wq` | work that participates in suspend freezer behavior |
| `system_power_efficient_wq` | work where power-efficient placement is preferred |

Most simple drivers can use:

```c
schedule_work(&priv->work);
```

or:

```c
queue_work(system_wq, &priv->work);
```

Use a named system queue only when its policy matches the driver requirement. For example, use `system_long_wq` for bounded but longer work, not for arbitrary unbounded blocking.

## Dedicated Workqueue Allocation

Create a dedicated workqueue with `alloc_workqueue()`.

```c
priv->wq = alloc_workqueue("demo_wq", WQ_MEM_RECLAIM, 0);
if (!priv->wq)
    return -ENOMEM;
```

The arguments are:

```text
name
flags
max_active
```

The name appears in worker names and debugging output, so choose something specific enough to recognize.

Queue work:

```c
queue_work(priv->wq, &priv->rx_work);
queue_delayed_work(priv->wq, &priv->retry_work, msecs_to_jiffies(100));
```

Destroy:

```c
cancel_delayed_work_sync(&priv->retry_work);
cancel_work_sync(&priv->rx_work);
destroy_workqueue(priv->wq);
```

Set a stopping flag and stop queueing sources before cancellation and destruction.

## Important Flags

| Flag | Meaning | Driver Use |
| --- | --- | --- |
| `WQ_UNBOUND` | workers are not tied to the queueing CPU | long or variable-concurrency work where locality is not important |
| `WQ_MEM_RECLAIM` | reserves forward progress under memory pressure | required for work used in reclaim paths |
| `WQ_HIGHPRI` | use high-priority worker pools | latency-sensitive short work |
| `WQ_FREEZABLE` | participates in system suspend freezer | work that should pause during suspend freeze |
| `WQ_CPU_INTENSIVE` | CPU-heavy bound work does not count against normal concurrency | bounded CPU-heavy work |

Flags are policy. Do not add them because they sound stronger. Add them because the driver has that specific requirement.

## `WQ_MEM_RECLAIM`

Use `WQ_MEM_RECLAIM` when the workqueue may be needed to make progress during memory reclaim.

Example cases:

- storage path work that may be needed to write pages
- filesystem or block-layer related work
- driver work called from shrinkers or reclaim-triggered paths
- work needed to free memory or complete memory allocation dependencies

Pattern:

```c
priv->wq = alloc_workqueue("demo_reclaim", WQ_MEM_RECLAIM, 0);
if (!priv->wq)
    return -ENOMEM;
```

Without this flag, reclaim can wait for work that cannot get an execution context because the system is already under memory pressure.

For ordinary GPIO, input, IIO, or simple character drivers that are not in reclaim paths, this flag is often unnecessary.

## `WQ_UNBOUND`

Bound workqueues prefer CPU locality. Unbound workqueues are not tied to the CPU that queued the work.

Use `WQ_UNBOUND` when:

- CPU locality is not useful
- concurrency requirements vary widely
- long work should be scheduled by the global scheduler
- the queueing CPU is not meaningful for the work

Example:

```c
priv->wq = alloc_workqueue("demo_unbound",
                           WQ_UNBOUND | WQ_MEM_RECLAIM,
                           0);
```

Do not use `WQ_UNBOUND` to avoid thinking about locking. Unbound work may run concurrently on different CPUs, so shared state still needs protection.

## Ordered Workqueues

Use an ordered workqueue when work items must execute one at a time in queue order.

```c
priv->wq = alloc_ordered_workqueue("demo_ordered", WQ_MEM_RECLAIM);
if (!priv->wq)
    return -ENOMEM;
```

Example: a device command queue where command B must not be sent before command A finishes.

```c
struct demo_cmd {
    struct work_struct work;
    struct demo_priv *priv;
    u8 opcode;
};

static void demo_cmd_work_fn(struct work_struct *work)
{
    struct demo_cmd *cmd = container_of(work, struct demo_cmd, work);

    demo_send_command(cmd->priv, cmd->opcode);
    kfree(cmd);
}
```

Queue each command on the ordered queue:

```c
INIT_WORK(&cmd->work, demo_cmd_work_fn);
queue_work(priv->wq, &cmd->work);
```

Do not approximate global ordering by assuming `max_active = 1` on an arbitrary queue gives the exact semantics you want. Use `alloc_ordered_workqueue()` when strict queue ordering is the requirement.

## `max_active`

`max_active` limits how many work items from a workqueue may execute concurrently per relevant pool. Passing `0` selects the default.

```c
priv->wq = alloc_workqueue("demo_limited", 0, 4);
```

Use it as a throttle, not as a correctness mechanism.

Good use:

```text
at most four expensive transactions should be in flight
```

Bad use:

```text
state is unprotected, so set max_active to 1 and hope races disappear
```

Even with a low `max_active`, state can still be touched by IRQ handlers, timers, sysfs, file operations, runtime PM callbacks, and remove paths. Locking is still required.

## Queue Choice Examples

### Simple IRQ Follow-Up

```c
INIT_WORK(&priv->work, demo_work_fn);
schedule_work(&priv->work);
```

Use the system workqueue. Keep the callback bounded.

### Long Firmware Load

```c
queue_work(system_long_wq, &priv->fw_work);
```

Or allocate a dedicated queue if the driver needs cancellation and flush isolation across several work items.

### Ordered Command Stream

```c
priv->wq = alloc_ordered_workqueue("demo_cmd", WQ_MEM_RECLAIM);
```

Use for serialized device commands.

### Reclaim-Sensitive Storage Path

```c
priv->wq = alloc_workqueue("demo_io", WQ_MEM_RECLAIM, 0);
```

Use for forward progress under memory pressure.

### Freezable Periodic Work

```c
priv->wq = alloc_workqueue("demo_pm", WQ_FREEZABLE, 0);
```

Use when work should not run through the freezer phase of suspend.

## Flushing A Workqueue

`flush_workqueue()` waits for work already queued on that workqueue to finish.

```c
flush_workqueue(priv->wq);
```

This is useful when a driver needs a synchronization point for many work items. It does not permanently stop future queueing.

For teardown, combine:

1. prevent new queueing
2. stop external producers such as IRQs or timers
3. cancel specific work items that can be pending
4. flush or destroy the queue

```c
WRITE_ONCE(priv->stopping, true);
disable_irq(priv->irq);
timer_shutdown_sync(&priv->timer);
cancel_delayed_work_sync(&priv->retry_work);
cancel_work_sync(&priv->rx_work);
destroy_workqueue(priv->wq);
```

Use `timer_delete_sync()` or the older `del_timer_sync()` style only when your target kernel uses that API and you have separately prevented rearming. For final teardown in modern kernels, `timer_shutdown_sync()` is designed to prevent rearming.

## Deadlock Patterns

### Work Flushes Itself

Wrong:

```c
static void demo_work_fn(struct work_struct *work)
{
    struct demo_priv *priv = container_of(work, struct demo_priv, work);

    cancel_work_sync(&priv->work);
}
```

A work item must not synchronously cancel or flush itself.

### Holding A Lock Needed By Work

Wrong:

```c
mutex_lock(&priv->lock);
cancel_work_sync(&priv->work);
mutex_unlock(&priv->lock);
```

If `demo_work_fn()` takes `priv->lock`, remove can deadlock waiting for work while work waits for the mutex.

Better:

```c
WRITE_ONCE(priv->stopping, true);
cancel_work_sync(&priv->work);

mutex_lock(&priv->lock);
demo_finish_teardown(priv);
mutex_unlock(&priv->lock);
```

Do not hold locks across synchronous cancellation unless you have verified the callback cannot need them.

### Work And Timer Rearm Each Other

Pattern:

```text
timer callback queues work
work callback rearms timer
remove cancels work
work rearms timer again
timer queues work again
```

Use a stopping flag and, on modern kernels, `timer_shutdown_sync()` for final teardown where rearming must be prevented.

## Power Management Interactions

Work can race with suspend, resume, and runtime PM.

Questions to answer:

- Can work access registers while the device is runtime suspended?
- Does the work need `pm_runtime_get_sync()` or `pm_runtime_resume_and_get()`?
- Should work be canceled before suspend?
- Should delayed work be freezable?
- Can resume requeue work safely?

Example:

```c
static void demo_work_fn(struct work_struct *work)
{
    struct demo_priv *priv = container_of(work, struct demo_priv, work);
    int ret;

    ret = pm_runtime_resume_and_get(priv->dev);
    if (ret < 0)
        return;

    mutex_lock(&priv->lock);
    if (!priv->stopping)
        demo_touch_registers(priv);
    mutex_unlock(&priv->lock);

    pm_runtime_put_autosuspend(priv->dev);
}
```

The exact PM calls depend on subsystem rules, but the principle is stable: a workqueue callback is asynchronous and must not assume the device is powered.

## Observability

Workqueue behavior can be inspected with:

- tracepoints for workqueue queue and execute events
- ftrace function graph tracing
- `ps` output showing `kworker` threads
- lockdep for lock inversions
- KASAN for lifetime bugs
- dynamic debug around queue and callback paths

When debugging, log the workqueue name, work item purpose, stopping flag, and whether the work can requeue itself.

## Practice Exercises

### Exercise 1: Choose The Queue

For each case, choose system workqueue, `system_long_wq`, dedicated workqueue, ordered workqueue, or threaded IRQ:

```text
short GPIO debounce that can sleep
ordered command stream
storage reclaim path
long firmware parsing
one interrupt that needs I2C reads
periodic runtime PM check
```

Defend the queue choice and teardown plan.

### Exercise 2: Ordered Commands

Build a command-work structure that queues three commands and guarantees command order. Explain why `alloc_ordered_workqueue()` is clearer than relying on worker implementation details.

### Exercise 3: Flush Deadlock Audit

Find every `flush_work*()` or `cancel_work_sync()` call in a driver and list which locks are held. Confirm the corresponding work callback does not need those locks.

## Debugging Checklist

- Check whether work can requeue itself.
- Check cancellation and flush semantics.
- Avoid long blocking work on shared queues.
- Use ordered workqueues only when ordering is required.
- Use `WQ_MEM_RECLAIM` when reclaim paths depend on the work.
- Do not assume one workqueue equals one thread.
- Do not use `max_active` as a replacement for locks.
- Stop queueing sources before destroying a dedicated queue.

## Related Topics

- [Workqueues](workqueues.md)
- [Bottom Halves, Softirqs, And Tasklets](bottom-halves-softirqs-and-tasklets.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)
- [Timers](timers.md)

## Official References

- [Workqueue](https://docs.kernel.org/core-api/workqueue.html)
- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
