---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Workqueues

## What Problem Does This Solve?

Workqueues defer work from contexts that cannot sleep or should not run long operations inline.

They are one of the most important driver tools because many driver entry points are not allowed to do the full job immediately.

Examples:

- an IRQ handler needs to acknowledge hardware quickly, then read status over I2C
- a timer needs to notice a timeout, then reset a device through sleepable APIs
- a sysfs write needs to kick a longer firmware transaction
- a driver needs to retry work after a delay
- probe needs to schedule initialization that should happen after registration

A workqueue gives you process context later. That solves the sleepability problem, but it creates a new lifetime problem: the driver data used by the work item must remain valid until the work item is no longer queued or running.

## Core Concepts

- work item
- delayed work
- system workqueues
- custom workqueues
- cancellation
- flushing
- teardown ordering
- work item lifetime

## Mental Model

Workqueues run later in process context. That solves context constraints but introduces lifetime and teardown constraints.

Think of a work item as a callback stored inside your driver object:

```text
driver object owns struct work_struct
interrupt/timer/sysfs queues the work
kernel worker thread later calls the function
work function recovers driver object with container_of()
remove path must stop future queueing and wait for running work
```

The work item is not a heap-allocated job by default. It is usually embedded in the object it operates on. That is why object lifetime is central.

```c
struct demo_priv {
    struct device *dev;
    struct mutex lock;
    struct work_struct irq_work;
    struct delayed_work poll_work;
    bool stopping;
    bool event_pending;
};
```

If `struct demo_priv` is freed while `irq_work` or `poll_work` can still run, the callback will use freed memory.

## Work Item Basics

A normal work item uses `struct work_struct`.

Initialize it once before queueing:

```c
static void demo_irq_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, irq_work);

    mutex_lock(&priv->lock);
    if (!priv->stopping)
        demo_process_event(priv);
    mutex_unlock(&priv->lock);
}

static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    mutex_init(&priv->lock);
    INIT_WORK(&priv->irq_work, demo_irq_work_fn);

    platform_set_drvdata(pdev, priv);
    return 0;
}
```

Queue it from a place that cannot do the slow work:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;

    demo_ack_irq(priv);
    schedule_work(&priv->irq_work);

    return IRQ_HANDLED;
}
```

The callback runs in worker-thread context, so it may usually sleep:

```c
static void demo_process_event(struct demo_priv *priv)
{
    int ret;

    ret = demo_read_status_over_i2c(priv);
    if (ret)
        dev_warn(priv->dev, "status read failed: %d\n", ret);
}
```

That I2C read would be illegal in a hard IRQ handler but is legal in a normal workqueue callback.

## `schedule_work()` And `queue_work()`

`schedule_work()` queues work on the default system workqueue.

```c
schedule_work(&priv->irq_work);
```

`queue_work()` queues work on a selected workqueue:

```c
queue_work(priv->wq, &priv->irq_work);
```

Both return `false` when the work item was already pending and therefore was not queued again.

That behavior matters. A single `struct work_struct` represents one pending execution, not an unbounded list of events.

```c
if (!schedule_work(&priv->irq_work))
    atomic_inc(&priv->coalesced_events);
```

This does not run the callback once per interrupt. It coalesces many queue attempts into one execution while the work is pending.

Use this intentionally:

- good for "status changed, process latest state"
- bad for "one work execution must represent exactly one hardware packet"

For exact event counts, keep an explicit queue, FIFO, bitmap, or counter protected by a lock, and let the work item drain that state.

## Coalesced Event Example

The typical pattern is:

1. IRQ records a fact.
2. IRQ queues work.
3. Work consumes all pending facts.

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    unsigned long flags;

    spin_lock_irqsave(&priv->state_lock, flags);
    priv->event_pending = true;
    spin_unlock_irqrestore(&priv->state_lock, flags);

    schedule_work(&priv->irq_work);
    return IRQ_HANDLED;
}

static void demo_irq_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, irq_work);
    bool pending;
    unsigned long flags;

    spin_lock_irqsave(&priv->state_lock, flags);
    pending = priv->event_pending;
    priv->event_pending = false;
    spin_unlock_irqrestore(&priv->state_lock, flags);

    if (!pending)
        return;

    mutex_lock(&priv->lock);
    if (!priv->stopping)
        demo_process_event(priv);
    mutex_unlock(&priv->lock);
}
```

Here the spinlock protects state shared with the IRQ handler, while the mutex protects sleepable device transactions in worker context.

## Delayed Work Basics

Delayed work combines a timer with a work item. The timer expires first, then the work function runs in worker-thread context.

Use delayed work when the eventual callback needs to sleep.

```c
struct demo_priv {
    struct delayed_work poll_work;
    unsigned int poll_interval_ms;
    bool stopping;
};
```

Initialize:

```c
INIT_DELAYED_WORK(&priv->poll_work, demo_poll_work_fn);
```

Queue:

```c
schedule_delayed_work(&priv->poll_work,
                      msecs_to_jiffies(priv->poll_interval_ms));
```

Callback:

```c
static void demo_poll_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(to_delayed_work(work),
                     struct demo_priv, poll_work);

    mutex_lock(&priv->lock);
    if (!priv->stopping)
        demo_poll_hardware(priv);
    mutex_unlock(&priv->lock);

    if (!READ_ONCE(priv->stopping))
        schedule_delayed_work(&priv->poll_work,
                              msecs_to_jiffies(priv->poll_interval_ms));
}
```

The requeue check is not optional. Without it, teardown can cancel the currently pending delayed work while the running callback immediately schedules a new one.

## `mod_delayed_work()`

`mod_delayed_work()` is useful when you want to start or update the delay for the same delayed work item.

```c
mod_delayed_work(system_wq, &priv->poll_work, msecs_to_jiffies(100));
```

Example: debounce a GPIO edge.

```c
static irqreturn_t demo_gpio_irq(int irq, void *data)
{
    struct demo_priv *priv = data;

    mod_delayed_work(system_wq, &priv->debounce_work,
                     msecs_to_jiffies(20));
    return IRQ_HANDLED;
}
```

Each edge pushes the work out by 20 ms. The work function then reads the stable GPIO state in sleepable context if the GPIO provider may sleep.

## Normal Work Versus Delayed Work Versus Timer

| Need | Use |
| --- | --- |
| Run sleepable follow-up soon | normal work |
| Run sleepable follow-up after delay | delayed work |
| Atomic timeout callback | timer |
| High precision atomic callback | hrtimer |
| Direct IRQ-related sleepable handler | threaded IRQ |

Use a plain timer only when the timer callback itself can do all required work without sleeping, or when the callback simply queues work.

Use delayed work when the delayed callback needs to call mutexes, I2C/SPI, regulator, firmware, or other sleepable APIs.

## System Workqueues

Most drivers start with system workqueues:

```c
schedule_work(&priv->work);
schedule_delayed_work(&priv->dwork, delay);
queue_work(system_wq, &priv->work);
```

This is appropriate for short, bounded work that does not need special ordering or forward-progress guarantees.

Avoid abusing shared system queues:

- do not run unbounded loops
- do not block indefinitely
- do not perform large CPU-heavy processing
- do not use shared queues when the work is part of memory reclaim
- do not assume ordering against unrelated work

When those constraints do not fit, use a dedicated workqueue.

## Dedicated Workqueues

A dedicated workqueue gives the driver a named execution domain with selected attributes.

```c
priv->wq = alloc_workqueue("demo_wq", WQ_MEM_RECLAIM, 0);
if (!priv->wq)
    return -ENOMEM;

INIT_WORK(&priv->work, demo_work_fn);
```

Queue:

```c
queue_work(priv->wq, &priv->work);
```

Teardown:

```c
cancel_work_sync(&priv->work);
destroy_workqueue(priv->wq);
```

Choose a dedicated workqueue when:

- the work may be involved in memory reclaim and needs `WQ_MEM_RECLAIM`
- the driver needs strict ordering with `alloc_ordered_workqueue()`
- the work is long enough that using a shared queue would be unfriendly
- the driver has many related work items that need one flush domain
- the work needs specific attributes such as unbound or high-priority execution

Do not create a private workqueue for every trivial deferred callback. Each dedicated queue adds naming, teardown, and reasoning overhead.

## Cancellation And Flushing

Workqueue teardown has two separate questions:

```text
Do I need to wait for this item to finish?
Do I need to prevent it from being queued again?
```

Common APIs:

| API | Meaning |
| --- | --- |
| `flush_work()` | wait for currently queued/running work to finish |
| `cancel_work_sync()` | cancel if pending and wait if running |
| `flush_delayed_work()` | wait for delayed work, running it if needed |
| `cancel_delayed_work_sync()` | cancel delayed work and wait if running |
| `flush_workqueue()` | wait for currently queued work on a queue |
| `destroy_workqueue()` | tear down a dedicated workqueue after users are stopped |

`flush_work()` does not prevent a callback from requeueing itself. `cancel_work_sync()` does not prevent another CPU from queueing the work again after cancellation returns. The driver must block new queueing through state and ordering.

Typical remove sequence:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    mutex_lock(&priv->lock);
    priv->stopping = true;
    mutex_unlock(&priv->lock);

    disable_irq(priv->irq);

    cancel_delayed_work_sync(&priv->poll_work);
    cancel_work_sync(&priv->irq_work);

    demo_hw_stop(priv);
}
```

If IRQs can queue work, disable the IRQ before canceling the work. Otherwise an IRQ can queue the work immediately after cancellation.

## Requeueing Work During Teardown

Self-requeueing work is common for polling and retry loops.

Wrong:

```c
static void demo_retry_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(to_delayed_work(work),
                     struct demo_priv, retry_work);

    if (demo_try_command(priv) == -EAGAIN)
        schedule_delayed_work(&priv->retry_work, msecs_to_jiffies(50));
}
```

This can requeue after remove begins.

Better:

```c
static void demo_retry_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(to_delayed_work(work),
                     struct demo_priv, retry_work);

    if (READ_ONCE(priv->stopping))
        return;

    if (demo_try_command(priv) == -EAGAIN &&
        !READ_ONCE(priv->stopping))
        schedule_delayed_work(&priv->retry_work, msecs_to_jiffies(50));
}
```

Remove:

```c
WRITE_ONCE(priv->stopping, true);
cancel_delayed_work_sync(&priv->retry_work);
```

If the flag is protected by a lock, take the same lock in both places. Use `READ_ONCE()`/`WRITE_ONCE()` only for simple lockless flags where that is the intended synchronization style.

## Work Item Lifetime

The object containing the work item must outlive:

- pending work
- running work
- work that can be queued by another callback
- delayed work timer state

This is why the work item is usually canceled in `remove()`, `disconnect()`, `release()`, or the object's final put path.

Wrong:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    kfree(priv); /* work may still run */
}
```

Better:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    WRITE_ONCE(priv->stopping, true);
    disable_irq(priv->irq);
    cancel_delayed_work_sync(&priv->poll_work);
    cancel_work_sync(&priv->irq_work);
    kfree(priv);
}
```

With devm allocation, the memory is freed automatically after remove returns, but the same rule applies: cancel or flush async users before returning from remove.

## Locking Inside Work Functions

A work function can sleep, so it may take mutexes.

```c
static void demo_config_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, config_work);

    mutex_lock(&priv->lock);
    demo_program_registers(priv);
    mutex_unlock(&priv->lock);
}
```

But the work function may also share state with IRQ handlers or timers. That state still needs an atomic-context-safe lock.

```c
spin_lock_irqsave(&priv->state_lock, flags);
status = priv->irq_status;
priv->irq_status = 0;
spin_unlock_irqrestore(&priv->state_lock, flags);
```

Do not hold a spinlock while calling sleepable APIs:

```c
spin_lock_irqsave(&priv->state_lock, flags);
status = priv->irq_status;
spin_unlock_irqrestore(&priv->state_lock, flags);

demo_i2c_transfer(priv, status);
```

The workqueue context permits sleeping, but the spinlock still does not.

## Error Handling Pattern

When queueing work during probe, initialize the work item before any path can queue it.

```c
static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    INIT_WORK(&priv->irq_work, demo_irq_work_fn);
    INIT_DELAYED_WORK(&priv->poll_work, demo_poll_work_fn);

    ret = devm_request_irq(&pdev->dev, priv->irq, demo_irq,
                           0, dev_name(&pdev->dev), priv);
    if (ret)
        return ret;

    schedule_delayed_work(&priv->poll_work, msecs_to_jiffies(1000));
    return 0;
}
```

If an error path happens after work has been queued, cancel it before returning:

```c
ret = demo_register_userspace(priv);
if (ret) {
    cancel_delayed_work_sync(&priv->poll_work);
    return ret;
}
```

Device-managed cleanup does not automatically cancel arbitrary work items unless you registered a cleanup action to do it.

## Device-Managed Cleanup Helper

For probe paths with several failure exits, `devm_add_action_or_reset()` can help centralize cancellation.

```c
static void demo_cancel_work(void *data)
{
    struct demo_priv *priv = data;

    WRITE_ONCE(priv->stopping, true);
    cancel_delayed_work_sync(&priv->poll_work);
    cancel_work_sync(&priv->irq_work);
}

static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    INIT_WORK(&priv->irq_work, demo_irq_work_fn);
    INIT_DELAYED_WORK(&priv->poll_work, demo_poll_work_fn);

    ret = devm_add_action_or_reset(&pdev->dev, demo_cancel_work, priv);
    if (ret)
        return ret;

    return 0;
}
```

Use this carefully: the cleanup action must run after all possible queueing sources have been stopped or made harmless.

## Workqueue Versus Threaded IRQ

Threaded IRQs and workqueues both provide sleepable context, but they express different intent.

Prefer threaded IRQ when:

- the work is a direct continuation of one interrupt
- the interrupt line should stay masked until the thread completes
- `IRQF_ONESHOT` fits the hardware
- the handler mostly services that interrupt

Prefer workqueue when:

- work can be scheduled from several sources
- work is a retry, poll, timeout, or state-machine step
- work should be coalesced
- the IRQ is only one producer of a broader state machine

Do not stack them unnecessarily. A threaded IRQ that only queues work may be a sign that the workqueue is not needed, unless there is a specific reason.

## Workqueue Debugging

Useful observations:

- `/proc/interrupts` can show whether IRQs are still firing
- `ps -eLf | grep kworker` can show worker activity
- ftrace can trace workqueue queue/execute events
- dynamic debug can be added around queue and callback paths
- KASAN catches many use-after-free bugs
- lockdep catches many lock inversions in work functions

Add temporary logs at the points where work is queued, starts, exits, and is canceled:

```c
dev_dbg(priv->dev, "queue irq_work stopping=%d\n",
        READ_ONCE(priv->stopping));
```

Avoid excessive logging in high-frequency IRQ paths; use tracepoints or rate-limited logs.

## Practice Exercises

### Exercise 1: IRQ To Workqueue

Create a small driver skeleton with:

```text
hard IRQ handler
spinlock-protected event flag
work item
mutex-protected sleepable processing
remove path with disable_irq() and cancel_work_sync()
```

Explain why the IRQ handler cannot call the sleepable processing function directly.

### Exercise 2: Delayed Polling

Add delayed work that polls every second and stops cleanly during remove.

Test the teardown reasoning:

```text
Can the delayed work requeue itself after cancellation?
Which flag prevents requeue?
Which function waits for a running callback?
```

### Exercise 3: Coalescing

Queue the same work item ten times before it runs. Explain why the callback may run once, and design a counter or list if all ten events need to be represented.

## Debugging Checklist

- Check whether work can requeue itself.
- Use the right cancel or flush primitive.
- Stop hardware before freeing state used by work.
- Avoid unbounded work in shared system workqueues.
- Confirm all queueing sources are blocked before cancellation returns.
- Confirm the work item's containing object remains alive.
- Check whether repeated `schedule_work()` calls are intentionally coalesced.
- Do not hold spinlocks across sleepable calls inside work functions.

## Related Topics

- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)
- [Driver Binding, Probe, And Remove](../fundamentals/driver-binding-probe-remove.md)
- [Concurrency Managed Workqueues](concurrency-managed-workqueues.md)
- [Timers](timers.md)

## Official References

- [Workqueue](https://docs.kernel.org/core-api/workqueue.html)
- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
