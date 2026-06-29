---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Hrtimers

## What Problem Does This Solve?

High-resolution timers provide precise timer callbacks when jiffies-based timers are not accurate enough.

Standard timers are based on jiffies and are appropriate for many driver timeouts. Hrtimers are for cases where the driver needs finer precision or more exact expiry behavior.

Examples:

- short protocol timing where jiffies granularity is too coarse
- precise sampling deadlines
- high-resolution software timeouts
- watchdog-like checks with sub-jiffy requirements

Hrtimers are not a way to make sleepable code precise. The callback still runs in atomic context and must not sleep.

## Core Concepts

- high-resolution timer
- monotonic time
- relative expiry
- absolute expiry
- callback context
- restart behavior
- cancellation
- precision and overhead

## Mental Model

Use hrtimers for precise timing, not for sleepable work. If the callback needs to sleep, use the hrtimer only to schedule sleepable follow-up work.

```text
initialize hrtimer with clock and mode
start with ktime_t expiry
callback runs in atomic context
callback returns restart or no-restart
teardown cancels before freeing state
```

Precision has a cost. Do not replace every normal timer with an hrtimer.

## Basic Hrtimer Structure

```c
#include <linux/hrtimer.h>

struct demo_priv {
    struct hrtimer sample_timer;
    spinlock_t lock;
    bool sample_due;
    bool stopping;
};
```

Initialize:

```c
hrtimer_init(&priv->sample_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
priv->sample_timer.function = demo_sample_timer_fn;
```

Start:

```c
hrtimer_start(&priv->sample_timer,
              ms_to_ktime(10),
              HRTIMER_MODE_REL);
```

Cancel during teardown:

```c
hrtimer_cancel(&priv->sample_timer);
```

## One-Shot Hrtimer

```c
static enum hrtimer_restart demo_sample_timer_fn(struct hrtimer *timer)
{
    struct demo_priv *priv =
        container_of(timer, struct demo_priv, sample_timer);
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->sample_due = true;
    spin_unlock_irqrestore(&priv->lock, flags);

    wake_up_interruptible(&priv->wait);

    return HRTIMER_NORESTART;
}
```

The callback returns `HRTIMER_NORESTART` for one-shot behavior.

Arm it:

```c
hrtimer_start(&priv->sample_timer,
              ns_to_ktime(500000),
              HRTIMER_MODE_REL);
```

That requests a relative timeout of 500000 ns.

## Periodic Hrtimer

For periodic behavior, forward the timer and return `HRTIMER_RESTART`.

```c
static enum hrtimer_restart demo_periodic_fn(struct hrtimer *timer)
{
    struct demo_priv *priv =
        container_of(timer, struct demo_priv, sample_timer);
    ktime_t interval = ms_to_ktime(5);

    if (READ_ONCE(priv->stopping))
        return HRTIMER_NORESTART;

    atomic_inc(&priv->ticks);
    hrtimer_forward_now(timer, interval);

    return HRTIMER_RESTART;
}
```

`hrtimer_forward_now()` avoids drift from simply restarting relative to the callback execution time.

Teardown:

```c
WRITE_ONCE(priv->stopping, true);
hrtimer_cancel(&priv->sample_timer);
```

## Callback Context

Hrtimer callbacks are atomic-context callbacks. Do not sleep.

Do not call:

```c
mutex_lock(&priv->lock);
i2c_transfer(...);
spi_sync(...);
msleep(1);
copy_to_user(...);
request_firmware(...);
```

Use:

```c
spin_lock_irqsave(&priv->lock, flags);
atomic_inc(&priv->count);
wake_up_interruptible(&priv->wait);
schedule_work(&priv->work);
```

If the precise deadline should trigger sleepable work, queue a work item:

```c
static enum hrtimer_restart demo_deadline_fn(struct hrtimer *timer)
{
    struct demo_priv *priv =
        container_of(timer, struct demo_priv, deadline_timer);

    schedule_work(&priv->deadline_work);
    return HRTIMER_NORESTART;
}
```

The work function then performs the sleepable operation.

## Relative Versus Absolute Expiry

Relative expiry:

```c
hrtimer_start(&priv->timer, ms_to_ktime(20), HRTIMER_MODE_REL);
```

The timer expires 20 ms from now.

Absolute expiry:

```c
ktime_t expires = ktime_add_ms(ktime_get(), 20);

hrtimer_start(&priv->timer, expires, HRTIMER_MODE_ABS);
```

The timer expires at the specified clock time.

Relative timers are simpler for ordinary timeouts. Absolute timers are useful when aligning to a specific clock deadline.

## Clock Choice

Common clock choices:

| Clock | Meaning | Driver Use |
| --- | --- | --- |
| `CLOCK_MONOTONIC` | monotonic time, does not follow wall-clock changes | elapsed intervals and timeouts |
| `CLOCK_BOOTTIME` | includes time spent suspended | timeouts that should include suspend time |
| `CLOCK_REALTIME` | wall-clock time, can jump | rarely appropriate for driver timeouts |

Use `CLOCK_MONOTONIC` for most elapsed-time driver logic. Avoid `CLOCK_REALTIME` for timeouts because wall-clock changes can move it.

## Starting, Canceling, And Querying

Common APIs:

| API | Use |
| --- | --- |
| `hrtimer_init()` | initialize timer |
| `hrtimer_start()` | arm or rearm |
| `hrtimer_cancel()` | cancel and wait for running callback |
| `hrtimer_try_to_cancel()` | attempt cancellation without waiting if callback is running |
| `hrtimer_active()` | check whether timer is active |
| `hrtimer_forward_now()` | periodic restart based on current time |

Use `hrtimer_cancel()` before freeing the object containing the hrtimer.

Do not hold a lock needed by the hrtimer callback while calling `hrtimer_cancel()`.

## Hrtimer Versus Standard Timer

| Need | Prefer |
| --- | --- |
| ordinary timeout in milliseconds or seconds | standard timer |
| delayed sleepable work | delayed work |
| precise sub-jiffy deadline | hrtimer |
| precise deadline that queues sleepable recovery | hrtimer plus workqueue |
| periodic low-frequency polling | delayed work |

A precise timer that fires frequently can increase CPU wakeups and power usage. Consider hardware interrupts, batching, and subsystem-specific APIs before adding high-frequency hrtimers.

## Example: Precise Pulse Timeout

Suppose a device reports a pulse start in an IRQ and the driver must mark the pulse stale after 750 us.

IRQ:

```c
static irqreturn_t demo_pulse_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->pulse_active = true;
    spin_unlock_irqrestore(&priv->lock, flags);

    hrtimer_start(&priv->pulse_timer,
                  ns_to_ktime(750000),
                  HRTIMER_MODE_REL);

    return IRQ_HANDLED;
}
```

Hrtimer:

```c
static enum hrtimer_restart demo_pulse_timeout(struct hrtimer *timer)
{
    struct demo_priv *priv =
        container_of(timer, struct demo_priv, pulse_timer);
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->pulse_active = false;
    priv->pulse_timed_out = true;
    spin_unlock_irqrestore(&priv->lock, flags);

    wake_up_interruptible(&priv->wait);
    return HRTIMER_NORESTART;
}
```

Remove:

```c
hrtimer_cancel(&priv->pulse_timer);
```

## Example: Hrtimer Defers Reset To Workqueue

```c
static enum hrtimer_restart demo_reset_deadline(struct hrtimer *timer)
{
    struct demo_priv *priv =
        container_of(timer, struct demo_priv, reset_timer);

    queue_work(system_wq, &priv->reset_work);
    return HRTIMER_NORESTART;
}

static void demo_reset_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, reset_work);

    mutex_lock(&priv->lock);
    if (!priv->stopping)
        demo_reset_over_spi(priv);
    mutex_unlock(&priv->lock);
}
```

Teardown must stop both:

```c
WRITE_ONCE(priv->stopping, true);
hrtimer_cancel(&priv->reset_timer);
cancel_work_sync(&priv->reset_work);
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| sleep warning | hrtimer callback called sleepable API | callback body |
| use-after-free | hrtimer not canceled before freeing object | remove path |
| high CPU usage | high-frequency periodic timer | interval and restart logic |
| timing drift | restarted relative to callback end | use `hrtimer_forward_now()` |
| timeout affected by wall-clock changes | used realtime clock | clock choice |
| deadlock in cancel | lock held while cancel waits for callback | callback locks |

## Practice Exercises

### Exercise 1: One-Shot Precision Timeout

Implement a one-shot 750 us timeout. The callback should set a flag and wake a wait queue.

Explain why a normal jiffies timer may not be precise enough.

### Exercise 2: Periodic Hrtimer

Implement a 5 ms periodic hrtimer using `hrtimer_forward_now()`.

Add a stopping flag and explain the teardown sequence.

### Exercise 3: Sleepable Recovery

Modify an hrtimer callback that calls SPI directly. Move the SPI operation into workqueue context.

## Debugging Checklist

- Check callback context.
- Check time unit conversions.
- Cancel timers before freeing state.
- Avoid excessive high-frequency timers.
- Use monotonic or boottime clocks for driver timeouts.
- Use `hrtimer_forward_now()` for periodic timers.
- Do not hold callback locks while canceling.
- Queue work for sleepable follow-up.

## Related Topics

- [Timers](timers.md)
- [Timekeeping And Kernel Timers](timekeeping-and-kernel-timers.md)
- [Workqueues](workqueues.md)

## Official References

- [hrtimers](https://docs.kernel.org/timers/hrtimers.html)
- [ktime accessors](https://docs.kernel.org/core-api/timekeeping.html)
- [NO_HZ: Reducing Scheduling-Clock Ticks](https://docs.kernel.org/timers/no_hz.html)
