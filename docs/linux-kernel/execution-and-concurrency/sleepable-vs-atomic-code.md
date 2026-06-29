---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Sleepable Vs Atomic Code

## What Problem Does This Solve?

Drivers must separate operations that can block from operations that must complete without scheduling.

The core question:

```text
Can this code sleep right here?
```

If the answer is no, do not call APIs that may wait for memory, locks, hardware, bus transfers, firmware, userspace pages, or scheduler events.

## Core Concepts

- sleepable context
- atomic context
- `GFP_KERNEL`
- `GFP_ATOMIC`
- mutex
- spinlock
- hard IRQ
- softirq
- timer callback
- threaded IRQ
- workqueue
- blocking bus transfers
- `might_sleep()`
- deferral

## Mental Model

Sleepable code may block:

```text
mutex_lock()
I2C/SPI transfer
msleep()
wait_for_completion()
kmalloc(..., GFP_KERNEL)
```

Atomic code must not block:

```text
hard IRQ handler
timer callback
softirq/tasklet
spinlock-held region
interrupts-disabled region
```

If atomic code needs a sleepable operation, split the work.

## Sleepable Operations

Common operations that may sleep:

- `mutex_lock()`
- `msleep()`
- `usleep_range()`
- `wait_event_interruptible()`
- `wait_for_completion()`
- I2C transfers
- SPI synchronous transfers
- regmap over I2C/SPI
- firmware loading
- regulator enable/disable in many cases
- clock prepare/enable
- GPIO access through I2C/SPI expanders
- `copy_to_user()` and `copy_from_user()`
- memory allocation with `GFP_KERNEL`

Some APIs may sleep only for certain providers or transports. If unsure, assume they may sleep and use sleepable context.

## Atomic Contexts

Common atomic contexts:

- hard IRQ handler
- softirq
- tasklet
- `timer_list` callback
- `hrtimer` callback
- while holding a spinlock
- while preemption/interrupts are disabled

Atomic context is not just "interrupt code". Process context can become atomic when it holds the wrong lock.

## Allocation Flags

Sleepable allocation:

```c
buf = kzalloc(size, GFP_KERNEL);
if (!buf)
    return -ENOMEM;
```

Atomic allocation:

```c
buf = kzalloc(size, GFP_ATOMIC);
if (!buf)
    return -ENOMEM;
```

Prefer `GFP_KERNEL` in sleepable paths. Use `GFP_ATOMIC` only when you genuinely cannot sleep. For repeated atomic-path needs, preallocate.

Bad design:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    void *buf = kmalloc(4096, GFP_ATOMIC);
    ...
}
```

Better:

```text
allocate buffer in probe
hard IRQ records event
thread/work uses preallocated buffer
```

## Mutex Versus Spinlock

Mutex:

```c
mutex_lock(&priv->lock);
/* may sleep while waiting */
mutex_unlock(&priv->lock);
```

Use in sleepable context for longer critical sections or operations that may sleep.

Spinlock:

```c
spin_lock_irqsave(&priv->lock, flags);
/* no sleeping */
spin_unlock_irqrestore(&priv->lock, flags);
```

Use for short sections shared with IRQ/atomic context.

Do not use a spinlock to protect code that calls sleepable APIs.

## Splitting Work

Hard IRQ:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;

    if (!demo_irq_pending(priv))
        return IRQ_NONE;

    demo_ack_irq(priv);
    return IRQ_WAKE_THREAD;
}
```

Thread:

```c
static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_priv *priv = data;

    mutex_lock(&priv->lock);
    demo_read_status_over_i2c(priv);
    mutex_unlock(&priv->lock);

    return IRQ_HANDLED;
}
```

This is the clean pattern for sleepable follow-up work.

## Workqueue Deferral

Timer callback cannot sleep:

```c
static void demo_timer_fn(struct timer_list *t)
{
    struct demo_priv *priv = from_timer(priv, t, timer);

    schedule_work(&priv->work);
}
```

Work can sleep:

```c
static void demo_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, work);

    mutex_lock(&priv->lock);
    demo_refresh_state(priv);
    mutex_unlock(&priv->lock);
}
```

## Delays

Sleepable delays:

```c
msleep(20);
usleep_range(1000, 2000);
```

Atomic short busy waits:

```c
udelay(10);
ndelay(100);
```

Use busy waits only for very short hardware timing requirements. Long busy waits waste CPU and hurt latency.

## GPIO Example

SoC GPIO may be non-sleeping, but expander GPIO may sleep.

Atomic-safe only when provider cannot sleep:

```c
gpiod_set_value(desc, 1);
```

Sleepable version:

```c
gpiod_set_value_cansleep(desc, 1);
```

If the GPIO can come from an expander, use the cansleep variant from sleepable context.

## Regmap Example

MMIO regmap may be usable in atomic context depending on configuration. I2C/SPI regmap can sleep.

Safer rule:

```text
regmap over I2C/SPI -> sleepable context
regmap over MMIO -> check locking/config/context
```

When writing generic code, do not assume the transport.

## Context Audit Pattern

For each helper, write down:

```text
helper: demo_update_status()
called from:
  - sysfs show: sleepable
  - irq thread: sleepable
  - timer callback: atomic
uses:
  - mutex
  - regmap_read over I2C

conclusion:
  timer callback must not call it directly
```

Then split into:

```text
demo_mark_update_needed()  atomic-safe
demo_update_status()       sleepable
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `sleeping function called from invalid context` | blocking call in atomic context | stack trace |
| hard lockup or latency spike | long busy wait or IRQ handler | ftrace, watchdog logs |
| memory allocation warning | `GFP_KERNEL` in atomic path | allocation call site |
| deadlock | mutex under spinlock or wrong lock order | lockdep |
| GPIO warning | non-cansleep accessor on sleeping provider | `gpiod_cansleep()` |
| IRQ handler too slow | bus work in hard IRQ | threaded IRQ |

## Common Mistakes

- Fixing warnings by replacing every allocation with `GFP_ATOMIC`.
- Using spinlocks around sleepable hardware access.
- Calling helpers from new contexts without rechecking assumptions.
- Using `msleep()` in timer or hard IRQ paths.
- Treating regmap as always non-sleeping.
- Forgetting that `copy_to_user()` can fault and sleep.

## Practice Exercises

### Exercise 1: API Classification

Classify these as sleepable or atomic-safe in your driver:

```text
i2c_smbus_read_byte_data
readl
mutex_lock
spin_lock_irqsave
gpiod_get_value_cansleep
copy_to_user
queue_work
complete
```

### Exercise 2: Split A Helper

Take a helper that reads hardware over I2C and is called from a timer. Change the timer to schedule work, and call the helper from work context.

### Exercise 3: Allocation Audit

Search for allocations in IRQ/timer paths and decide whether they should be preallocated.

## Debugging Checklist

- Does this code path sleep?
- Does it hold a spinlock?
- Are interrupts disabled?
- Does it call bus, firmware, regulator, clock, GPIO, or userspace-copy APIs?
- Are allocation flags correct?
- Would a threaded IRQ or workqueue be simpler?
- Are debug configs enabled to catch violations?

## Related Topics

- [Context Rules](context-rules.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Workqueues](workqueues.md)
- [Kernel Memory Allocation](../memory-and-io/kernel-memory-allocation.md)
- [GPIO Consumer API](../driver-interfaces/gpio-consumer-api.md)

## Official References

- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [Unreliable Guide To Locking](https://docs.kernel.org/kernel-hacking/locking.html)
