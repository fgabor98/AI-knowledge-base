---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Timers

## What Problem Does This Solve?

Kernel timers schedule callbacks for future execution, commonly for timeouts, periodic polling, and debounce logic.

They answer the question:

```text
run this callback after a future jiffies-based deadline
```

Timers are useful when the callback can do a small amount of atomic-context work, such as marking a timeout, waking waiters, updating state protected by a spinlock, or queueing work.

Timers are not a general way to run sleepable code later. If the follow-up operation needs to sleep, the timer should schedule work or you should use delayed work directly.

## Core Concepts

- timer callback
- jiffies
- high-resolution timers
- delayed work
- cancellation
- timer lifetime
- process context alternatives

## Mental Model

Timers are asynchronous callbacks. They are useful for deadlines and wakeups, but any sleepable follow-up work must be moved elsewhere.

```text
process/IRQ/work context arms timer
time passes
timer callback runs in atomic context
callback cannot sleep
callback may update state, wake waiters, or queue work
remove path must stop timer before freeing state
```

The timer callback is another concurrent access path into your driver. Treat it like an IRQ-like callback for sleepability and lifetime.

## Basic Timer Structure

Embed a `struct timer_list` in the object it belongs to:

```c
struct demo_priv {
    struct device *dev;
    spinlock_t lock;
    struct timer_list timeout;
    struct work_struct timeout_work;
    bool timed_out;
    bool stopping;
};
```

Initialize it before it can be armed:

```c
timer_setup(&priv->timeout, demo_timeout_fn, 0);
```

Arm or rearm it:

```c
mod_timer(&priv->timeout, jiffies + msecs_to_jiffies(250));
```

Cancel it during teardown:

```c
del_timer_sync(&priv->timeout);
```

On modern kernels, prefer `timer_shutdown_sync()` for final teardown when the timer must not be rearmed after shutdown:

```c
timer_shutdown_sync(&priv->timeout);
```

Use the API available in your target kernel, but preserve the invariant: after teardown synchronization returns, no timer callback may run or be rearmed against freed state.

## Timer Callback Example

```c
static void demo_timeout_fn(struct timer_list *t)
{
    struct demo_priv *priv = from_timer(priv, t, timeout);
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->timed_out = true;
    spin_unlock_irqrestore(&priv->lock, flags);

    schedule_work(&priv->timeout_work);
}
```

This callback does not sleep. It records state and queues work for sleepable recovery.

The work function can sleep:

```c
static void demo_timeout_work_fn(struct work_struct *work)
{
    struct demo_priv *priv =
        container_of(work, struct demo_priv, timeout_work);

    if (READ_ONCE(priv->stopping))
        return;

    mutex_lock(&priv->io_lock);
    if (!priv->stopping)
        demo_reset_device(priv);
    mutex_unlock(&priv->io_lock);
}
```

Teardown must cancel both:

```c
WRITE_ONCE(priv->stopping, true);
timer_shutdown_sync(&priv->timeout);
cancel_work_sync(&priv->timeout_work);
```

If using older timer APIs, use `del_timer_sync()` plus a separate rearm-prevention flag.

## Why Timer Callbacks Cannot Sleep

Timer callbacks run in atomic context. They must not call APIs that can block.

Do not call:

```c
mutex_lock(&priv->lock);
i2c_transfer(...);
spi_sync(...);
regulator_enable(...);
msleep(20);
copy_to_user(...);
request_firmware(...);
```

Use:

```c
spin_lock_irqsave(&priv->lock, flags);
atomic_inc(&priv->timeouts);
wake_up_interruptible(&priv->wait);
schedule_work(&priv->work);
```

Atomic context is not made sleepable just because the timer was armed from process context.

## One-Shot Timeout

Example: a command must complete within 500 ms.

```c
static int demo_start_command(struct demo_priv *priv)
{
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->cmd_done = false;
    priv->cmd_timed_out = false;
    spin_unlock_irqrestore(&priv->lock, flags);

    mod_timer(&priv->cmd_timer, jiffies + msecs_to_jiffies(500));

    return demo_kick_hardware(priv);
}
```

IRQ completion cancels the timer:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    unsigned long flags;

    if (!demo_command_complete(priv))
        return IRQ_NONE;

    spin_lock_irqsave(&priv->lock, flags);
    priv->cmd_done = true;
    spin_unlock_irqrestore(&priv->lock, flags);

    del_timer(&priv->cmd_timer);
    wake_up_interruptible(&priv->wait);

    return IRQ_HANDLED;
}
```

Timeout callback:

```c
static void demo_cmd_timeout_fn(struct timer_list *t)
{
    struct demo_priv *priv = from_timer(priv, t, cmd_timer);
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->cmd_timed_out = true;
    spin_unlock_irqrestore(&priv->lock, flags);

    wake_up_interruptible(&priv->wait);
}
```

Waiter:

```c
ret = wait_event_interruptible(priv->wait,
                               priv->cmd_done || priv->cmd_timed_out);
if (ret)
    return ret;

if (priv->cmd_timed_out)
    return -ETIMEDOUT;
```

For production code, access `cmd_done` and `cmd_timed_out` under the same lock or use helpers that make the condition race-free.

## Periodic Timers

A periodic timer rearms itself from the callback.

```c
static void demo_periodic_fn(struct timer_list *t)
{
    struct demo_priv *priv = from_timer(priv, t, periodic);

    if (READ_ONCE(priv->stopping))
        return;

    atomic_inc(&priv->ticks);
    mod_timer(&priv->periodic, jiffies + msecs_to_jiffies(1000));
}
```

Teardown:

```c
WRITE_ONCE(priv->stopping, true);
timer_shutdown_sync(&priv->periodic);
```

Periodic timers are easy to abuse. Prefer hardware interrupts when the hardware can notify you. Prefer delayed work when the periodic operation needs sleepable bus access.

## Debounce Example

For GPIO debounce, a timer may be enough if reading the final state is atomic-safe.

```c
static irqreturn_t demo_gpio_irq(int irq, void *data)
{
    struct demo_priv *priv = data;

    mod_timer(&priv->debounce_timer, jiffies + msecs_to_jiffies(20));
    return IRQ_HANDLED;
}

static void demo_debounce_fn(struct timer_list *t)
{
    struct demo_priv *priv = from_timer(priv, t, debounce_timer);
    int value;

    value = gpiod_get_value(priv->gpiod);
    input_report_switch(priv->input, SW_LID, value);
    input_sync(priv->input);
}
```

But if the GPIO provider can sleep, use delayed work:

```c
mod_delayed_work(system_wq, &priv->debounce_work, msecs_to_jiffies(20));
```

Then call `gpiod_get_value_cansleep()` from the work function.

## Timer Versus Delayed Work

| Need | Prefer |
| --- | --- |
| update atomic state after timeout | timer |
| wake waiters after timeout | timer |
| queue sleepable recovery after timeout | timer plus work |
| run sleepable callback after delay | delayed work |
| periodic sleepable polling | delayed work |
| precise sub-jiffy timing | hrtimer |

If the timer callback immediately queues work and does nothing else, delayed work may be simpler.

## Cancellation APIs

| API | Use |
| --- | --- |
| `del_timer()` | cancel pending timer, does not wait for running callback |
| `del_timer_sync()` | cancel and wait for running callback |
| `timer_shutdown_sync()` | final shutdown; prevents future rearm on modern kernels |
| `timer_pending()` | test whether a timer is pending |

For teardown, use a synchronous API. A non-synchronous delete is rarely enough before freeing state.

Do not hold a lock needed by the timer callback while calling the synchronous delete:

```c
spin_lock_irqsave(&priv->lock, flags);
del_timer_sync(&priv->timer); /* wrong if callback takes lock */
spin_unlock_irqrestore(&priv->lock, flags);
```

Better:

```c
WRITE_ONCE(priv->stopping, true);
del_timer_sync(&priv->timer);

spin_lock_irqsave(&priv->lock, flags);
demo_finish_locked(priv);
spin_unlock_irqrestore(&priv->lock, flags);
```

## Rearm Races

The common timer teardown bug is:

```text
CPU0 remove: del_timer_sync() returns
CPU1 work: mod_timer() arms timer again
CPU0 remove: frees driver data
timer fires and uses freed data
```

Fix this with ordering:

```text
set stopping flag
stop external producers
cancel work that can rearm timer
shutdown/delete timer
free data only after all async users are stopped
```

If the timer and work rearm each other, shut down both sides:

```c
WRITE_ONCE(priv->stopping, true);
cancel_work_sync(&priv->rearm_work);
timer_shutdown_sync(&priv->timer);
```

Adjust order to match which callback can rearm which mechanism.

## Time Units

Standard timers use `jiffies` deadlines.

Convert from human units:

```c
msecs_to_jiffies(100)
usecs_to_jiffies(500)
secs_to_jiffies(5)
```

Set deadlines relative to current time:

```c
mod_timer(&priv->timer, jiffies + msecs_to_jiffies(100));
```

Compare jiffies with wraparound-safe helpers:

```c
if (time_after(jiffies, priv->deadline))
    demo_handle_expiry(priv);
```

Do not write:

```c
if (jiffies > priv->deadline)
    demo_handle_expiry(priv);
```

That fails around wraparound.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| sleep warning | timer callback called sleepable API | callback body |
| use-after-free after unload | timer not synchronously stopped | remove path |
| timer fires after shutdown | another callback rearms it | rearm sites |
| deadlock in remove | lock held across `del_timer_sync()` | callback locks |
| timeout too short/long | wrong units or jiffies conversion | conversion helpers |
| high CPU usage | periodic timer interval too small | rearm rate |
| missed timeout | command completion and timeout state race | shared lock/condition |

## Practice Exercises

### Exercise 1: One-Shot Command Timeout

Implement a command timeout with:

```text
timer_setup()
mod_timer()
timeout flag
wait queue wakeup
del_timer_sync() or timer_shutdown_sync() in teardown
```

Explain which fields are protected by a spinlock.

### Exercise 2: Timer To Delayed Work

Take a timer callback that calls a sleepable function and convert it to delayed work.

Show the before and after teardown sequence.

### Exercise 3: Rearm Audit

Search a driver for every `mod_timer()` call. For each one, answer:

```text
Can this run after remove begins?
What flag blocks it?
What synchronous teardown call stops the timer?
Can a work item rearm it?
```

## Debugging Checklist

- Check callback context before calling APIs.
- Confirm timers are canceled before freeing state.
- Avoid periodic timers when hardware interrupts are available.
- Check time unit conversions.
- Use wraparound-safe jiffies helpers.
- Check every timer rearm site.
- Do not hold callback locks while synchronously deleting timers.
- Prefer delayed work when the delayed callback needs to sleep.

## Related Topics

- [Workqueues](workqueues.md)
- [Context Rules](context-rules.md)
- [Reference Counting And Lifetime](reference-counting-and-lifetime.md)
- [Timekeeping And Kernel Timers](timekeeping-and-kernel-timers.md)
- [Hrtimers](hrtimers.md)

## Official References

- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [ktime accessors](https://docs.kernel.org/core-api/timekeeping.html)
