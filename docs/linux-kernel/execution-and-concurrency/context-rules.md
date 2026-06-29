---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Context Rules

## What Problem Does This Solve?

Kernel APIs are only valid in certain execution contexts. Calling a sleepable API from atomic context can deadlock, corrupt state, or trigger warnings such as:

```text
BUG: sleeping function called from invalid context
scheduling while atomic
```

Before calling an API, decide:

```text
Where am I running?
May I sleep?
What locks do I already hold?
What can run concurrently with me?
```

## Core Concepts

- process context
- interrupt context
- hard IRQ
- threaded IRQ
- softirq context
- tasklet context
- timer callback context
- workqueue context
- preemption
- scheduling
- sleepable APIs
- atomic context
- spinlock-held regions
- `might_sleep()`
- lockdep

## Mental Model

Context determines what is legal.

```text
sleepable context
  may block and let scheduler run something else

atomic context
  must not sleep
  must finish quickly
```

Some code normally runs in process context but becomes atomic because it holds a spinlock or disabled interrupts.

## Common Driver Contexts

| Context | May Sleep? | Typical Entry Points |
| --- | ---: | --- |
| probe/remove | yes | bus driver callbacks |
| sysfs show/store | yes | user reads/writes sysfs |
| file operations | yes | `open`, `read`, `write`, `ioctl` |
| workqueue callback | yes | `work_struct` function |
| threaded IRQ handler | yes | IRQ thread function |
| hard IRQ handler | no | primary IRQ handler |
| softirq/tasklet | no | network, block, legacy bottom halves |
| timer callback | no | `timer_list` callback |
| hrtimer callback | no | `hrtimer` callback |
| spinlock-held region | no | any context while lock held |

## Process Context

Process context can usually sleep.

Examples:

```c
static ssize_t threshold_store(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf, size_t count)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    unsigned int val;
    int ret;

    ret = kstrtouint(buf, 0, &val);
    if (ret)
        return ret;

    mutex_lock(&priv->lock);
    priv->threshold = val;
    mutex_unlock(&priv->lock);

    return count;
}
```

This can take a mutex because sysfs callbacks run in process context.

But process context can still become non-sleepable:

```c
spin_lock(&priv->lock);
ret = i2c_smbus_read_byte_data(client, REG_STATUS); /* wrong */
spin_unlock(&priv->lock);
```

The spinlock makes the region atomic.

## Hard IRQ Context

Hard IRQ handlers must be short and non-sleeping.

Allowed style:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    u32 status;

    status = readl(priv->base + DEMO_STATUS);
    if (!(status & DEMO_IRQ_PENDING))
        return IRQ_NONE;

    writel(DEMO_IRQ_PENDING, priv->base + DEMO_STATUS);
    return IRQ_WAKE_THREAD;
}
```

Not allowed:

```c
mutex_lock(&priv->lock);
msleep(20);
i2c_smbus_read_byte_data(client, REG_STATUS);
```

Use a threaded IRQ or workqueue for sleepable follow-up work.

## Threaded IRQ Context

Threaded IRQ handlers run in a kernel thread and can sleep:

```c
static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_priv *priv = data;
    unsigned int status;

    mutex_lock(&priv->lock);
    regmap_read(priv->regmap, DEMO_STATUS, &status);
    mutex_unlock(&priv->lock);

    return IRQ_HANDLED;
}
```

This is the normal choice for I2C/SPI devices with interrupt lines.

## Workqueue Context

Workqueue callbacks run in process context:

```c
static void demo_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, work);

    mutex_lock(&priv->lock);
    demo_read_status_over_i2c(priv);
    mutex_unlock(&priv->lock);
}
```

They can sleep, but they are asynchronous. Teardown must cancel or flush them before freeing state:

```c
cancel_work_sync(&priv->work);
```

## Timer Context

Timer callbacks cannot sleep:

```c
static void demo_timer_fn(struct timer_list *t)
{
    struct demo_priv *priv = from_timer(priv, t, timer);

    priv->expired = true;
    schedule_work(&priv->work);
}
```

Use a timer to mark a timeout or schedule follow-up. Do not do sleepable work directly in the timer callback.

## Softirq And Tasklet Context

Softirq and tasklet-style callbacks are atomic. They cannot sleep.

You may encounter them in:

- networking
- block layer
- older drivers
- legacy bottom-half code

For most new ordinary drivers, threaded IRQs and workqueues are easier to reason about.

## Spinlock-Held Regions

Even in process context:

```c
spin_lock_irqsave(&priv->irq_lock, flags);
/* atomic region */
spin_unlock_irqrestore(&priv->irq_lock, flags);
```

Inside this region:

- no sleeping
- no mutexes
- no I2C/SPI transfers
- no `GFP_KERNEL` allocations
- no copy to/from userspace

Keep spinlock regions small.

## Allocation Context

Sleepable:

```c
buf = kzalloc(size, GFP_KERNEL);
```

Atomic:

```c
buf = kzalloc(size, GFP_ATOMIC);
```

`GFP_ATOMIC` is for rare cases where allocation cannot sleep. It is not a general fix for poor design. Prefer preallocation or deferral to process context when possible.

## Detecting Context Bugs

Enable debug configs and use:

```c
might_sleep();
```

in code that must only be called from sleepable context.

Warnings to investigate:

```text
BUG: sleeping function called from invalid context
scheduling while atomic
possible circular locking dependency detected
```

Tools:

- lockdep
- `CONFIG_DEBUG_ATOMIC_SLEEP`
- KASAN/KCSAN for related bugs
- ftrace to inspect callback paths

## PREEMPT_RT Note

PREEMPT_RT changes how some kernel contexts and locks are implemented. For example, many interrupt handlers run threaded and many `spinlock_t` instances become sleepable rtmutex-backed locks.

Driver rule of thumb:

- obey the normal API context rules
- do not rely on implementation details
- use `raw_spinlock_t` only for truly raw/low-level non-sleeping sections
- test on the kernel configuration your product uses

## Annotating A Driver

Add comments where context is non-obvious:

```c
/* Hard IRQ context: no sleeping, only acknowledge and wake thread. */
static irqreturn_t demo_irq(int irq, void *data)
{
    ...
}

/* Workqueue context: may sleep. */
static void demo_work_fn(struct work_struct *work)
{
    ...
}
```

This is especially helpful when a helper is called from multiple contexts.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `scheduling while atomic` | sleeping under spinlock or IRQ context | stack trace, locks |
| `might_sleep()` warning | sleepable API called from atomic path | callback context |
| intermittent deadlock | lock order or waiting under wrong lock | lockdep |
| I2C/SPI warning in IRQ | bus transfer in hard handler | threaded IRQ |
| remove crash | async callback after free | work/timer/IRQ teardown |
| allocation failure in IRQ | inappropriate dynamic allocation | preallocate or defer |

## Common Mistakes

- Thinking "called by a user" always means sleepable.
- Forgetting a spinlock makes the region atomic.
- Calling regmap over I2C/SPI from hard IRQ context.
- Using `GFP_ATOMIC` instead of fixing execution context.
- Assuming timer callbacks can sleep.
- Ignoring PREEMPT_RT differences until late.
- Failing to document helpers that must be called only from one context.

## Practice Exercises

### Exercise 1: Callback Context Table

For one driver, list each callback and whether it may sleep:

```text
probe: yes
remove: yes
irq: no
irq_thread: yes
work: yes
timer: no
sysfs store: yes
```

### Exercise 2: Move Sleepable Work

Find a sleepable operation in a hard IRQ path and move it to a threaded IRQ or workqueue.

### Exercise 3: Force A Warning In A Lab

In a disposable lab module, intentionally call `might_sleep()` under a spinlock and observe the warning. Remove the bug after confirming the tool catches it.

## Debugging Checklist

- What callback is running?
- Is this hard IRQ, softirq, timer, workqueue, threaded IRQ, or process context?
- Are interrupts disabled?
- Is a spinlock held?
- Could this helper sleep?
- Are allocation flags correct?
- Does the code call any bus, regulator, clock, firmware, or userspace-copy API?
- Should this work be deferred?

## Related Topics

- [Sleepable Vs Atomic Code](sleepable-vs-atomic-code.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Workqueues](workqueues.md)
- [Locking And Atomics](locking-and-atomics.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)

## Official References

- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Unreliable Guide To Locking](https://docs.kernel.org/kernel-hacking/locking.html)
