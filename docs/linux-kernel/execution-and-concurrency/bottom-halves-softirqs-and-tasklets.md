---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Bottom Halves, Softirqs, And Tasklets

## What Problem Does This Solve?

Bottom-half mechanisms defer interrupt-related work out of the hard IRQ path.

The goal is to keep hard IRQ handlers short:

```text
top half:
  acknowledge/classify urgent interrupt work

bottom half:
  process deferred work later
```

In modern driver work, the most common choices are threaded IRQs and workqueues. Softirqs and tasklets still appear in existing kernel code, especially networking and older drivers, so you need to understand their constraints.

## Core Concepts

- top half
- bottom half
- hard IRQ
- softirq
- tasklet
- threaded IRQ
- workqueue
- atomic context
- BH-disabled region
- teardown ordering
- legacy migration

## Mental Model

Deferred work must match the context requirement.

```text
needs to sleep?
  -> threaded IRQ or workqueue

must remain atomic and very fast?
  -> softirq/tasklet-style mechanism may appear in existing code

needs longer processing?
  -> workqueue or dedicated kernel thread/subsystem mechanism
```

Do not use a tasklet or softirq just because you want something to run later. Use workqueues or threaded IRQs when the work can sleep.

## Top Half Versus Bottom Half

Hard IRQ top half:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;

    if (!demo_irq_pending(priv))
        return IRQ_NONE;

    demo_ack_irq(priv);
    schedule_work(&priv->work);
    return IRQ_HANDLED;
}
```

Bottom half in workqueue:

```c
static void demo_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, work);

    mutex_lock(&priv->lock);
    demo_process_event(priv);
    mutex_unlock(&priv->lock);
}
```

This is often clearer than legacy bottom-half APIs.

## Softirqs

Softirqs are low-level, statically defined deferred execution contexts. They are heavily used by core subsystems such as networking and timers.

Properties:

- run in atomic context
- cannot sleep
- may run on multiple CPUs
- require careful locking
- not normally created by ordinary driver authors

If you are writing an ordinary device driver, you usually do not add new softirq types.

## Tasklets

Tasklets are a legacy bottom-half mechanism built on softirq infrastructure.

Properties:

- run in atomic context
- cannot sleep
- historically used for serialized deferred interrupt work
- appear in older drivers
- often replaceable by threaded IRQs or workqueues

When maintaining old code, check whether the tasklet does anything sleepable or too long-running. If so, migrate it.

## Threaded IRQs As Modern Bottom Halves

Threaded IRQs are often the cleanest bottom-half replacement for interrupt-driven devices:

```c
ret = devm_request_threaded_irq(dev, irq,
                                demo_irq,
                                demo_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(dev), priv);
```

Use when:

- work is directly tied to an IRQ
- I2C/SPI/regmap-over-bus access is needed
- mutexes are needed
- IRQ masking while processing matters

## Workqueues As Deferred Work

Use workqueues when:

- deferred work is not one-to-one with an IRQ
- work can be coalesced
- the same work may be scheduled by timer, sysfs, IRQ, and file operations
- processing may take longer
- retries or state machines are involved

Workqueues can sleep, but must be canceled or flushed during teardown.

## Choosing A Mechanism

| Need | Prefer |
| --- | --- |
| Sleepable follow-up to one IRQ | threaded IRQ |
| Deferred work from several sources | workqueue |
| Periodic sleepable polling | delayed work |
| Precise atomic timeout callback | hrtimer |
| Existing networking fast path | softirq/NAPI patterns |
| Legacy atomic deferred callback | tasklet, but consider migration |

## Teardown

Every deferred mechanism needs teardown:

Work:

```c
cancel_work_sync(&priv->work);
```

Delayed work:

```c
cancel_delayed_work_sync(&priv->dwork);
```

Timer:

```c
del_timer_sync(&priv->timer);
```

Tasklet in older code:

```c
tasklet_kill(&priv->tasklet);
```

The `_sync`/kill operation matters because it waits for callbacks that may already be running.

## Common Migration: Tasklet To Workqueue

Old:

```c
static void demo_tasklet_fn(unsigned long data)
{
    struct demo_priv *priv = (void *)data;

    demo_read_status_over_i2c(priv); /* wrong: can sleep */
}
```

Better:

```c
static void demo_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, work);

    demo_read_status_over_i2c(priv);
}
```

IRQ:

```c
schedule_work(&priv->work);
```

Remove:

```c
cancel_work_sync(&priv->work);
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| sleep warning in tasklet | sleepable API in atomic bottom half | tasklet body |
| use-after-free on unload | deferred callback not killed/canceled | remove path |
| CPU spike | bottom half requeues itself too fast | scheduling logic |
| missed event | top half acknowledged before state captured | hardware sequence |
| deadlock | bottom half and process context lock inversion | lockdep |
| long latency | too much work in atomic bottom half | ftrace |

## Common Mistakes

- Assuming deferred means sleepable.
- Adding new tasklets when a threaded IRQ or workqueue fits.
- Forgetting to cancel deferred callbacks in remove.
- Using softirq/tasklet mechanisms for long work.
- Sharing state between top and bottom halves without locking.
- Migrating tasklet code to workqueue but forgetting teardown changes.

## Practice Exercises

### Exercise 1: Classify Deferred Work

For each deferred callback in a driver, identify:

```text
mechanism
context
may sleep?
teardown function
shared state locks
```

### Exercise 2: Move Sleepable Tasklet Work

Take a tasklet-like example that does I2C/SPI work and convert it to a workqueue or threaded IRQ.

### Exercise 3: Teardown Audit

Verify every deferred callback has a matching synchronous cancellation or destroy path.

## Debugging Checklist

- Is the deferred handler sleepable or atomic?
- Does it call bus, regulator, clock, firmware, or userspace-copy APIs?
- Can it run after remove begins?
- Is it canceled or killed before state is freed?
- Can it requeue itself?
- Is shared state protected consistently?
- Is a threaded IRQ or workqueue simpler?

## Related Topics

- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Workqueues](workqueues.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)

## Official References

- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Workqueue](https://docs.kernel.org/core-api/workqueue.html)
