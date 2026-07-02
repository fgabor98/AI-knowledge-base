---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Timekeeping And Kernel Timers

## What Problem Does This Solve?

Kernel drivers need safe ways to measure time, schedule timeouts, delay briefly, and avoid busy waiting.

Time bugs are common because driver code mixes several different concepts:

- "run a callback later"
- "sleep for a while"
- "busy wait for a few microseconds"
- "measure elapsed time"
- "compare deadlines safely across wraparound"
- "handle suspend and wall-clock changes correctly"

This page separates those concepts and maps them to the right kernel APIs.

## Core Concepts

- jiffies
- monotonic time
- real time
- `ktime_t`
- standard timers
- hrtimers
- sleeping delays
- busy-wait delays
- time comparison helpers

## Mental Model

Use the time API that matches the job: timers for future callbacks, sleeping delays for sleepable contexts, busy waits only for short hardware timing requirements, and monotonic time for elapsed intervals.

```text
future callback?
  timer, hrtimer, or delayed work

sleep current task?
  msleep(), usleep_range(), wait_event*(), schedule_timeout()

busy wait because hardware requires it?
  udelay(), ndelay(), mdelay() only for very short waits

measure elapsed time?
  ktime_get*() and monotonic/boottime clock choice

compare jiffies deadlines?
  time_after(), time_before(), time_is_*()
```

Do not choose an API only by unit. Choose by context, clock semantics, and whether you need a callback or a delay.

## Jiffies

`jiffies` is the kernel's tick counter. Standard timers use jiffies deadlines.

```c
unsigned long deadline;

deadline = jiffies + msecs_to_jiffies(100);
```

Jiffies wrap around. Code must use wraparound-safe comparison helpers.

Correct:

```c
if (time_after(jiffies, deadline))
    demo_timeout(priv);
```

Wrong:

```c
if (jiffies > deadline)
    demo_timeout(priv);
```

The wrong version eventually fails around wraparound.

## Jiffies Conversion Helpers

Use conversion helpers instead of open-coded `HZ` arithmetic.

```c
msecs_to_jiffies(250)
usecs_to_jiffies(500)
secs_to_jiffies(3)
jiffies_to_msecs(delta)
jiffies_to_usecs(delta)
```

Example:

```c
mod_timer(&priv->timer, jiffies + msecs_to_jiffies(250));
```

Avoid:

```c
mod_timer(&priv->timer, jiffies + HZ / 4);
```

The helper is clearer and handles edge cases better.

## Wraparound-Safe Helpers

Common helpers:

| Helper | Meaning |
| --- | --- |
| `time_after(a, b)` | `a` is after `b` |
| `time_before(a, b)` | `a` is before `b` |
| `time_after_eq(a, b)` | `a` is after or equal to `b` |
| `time_before_eq(a, b)` | `a` is before or equal to `b` |
| `time_is_before_jiffies(a)` | `a` is before current `jiffies` |
| `time_is_after_jiffies(a)` | `a` is after current `jiffies` |

Example:

```c
if (time_is_before_jiffies(priv->next_poll))
    schedule_delayed_work(&priv->poll_work, 0);
```

Use the `64` variants when dealing with `jiffies_64`.

## `ktime_t`

`ktime_t` represents high-resolution kernel time. Hrtimers and timekeeping APIs use it.

Create values:

```c
ktime_t delay;

delay = ms_to_ktime(20);
delay = ns_to_ktime(750000);
```

Add or subtract:

```c
ktime_t deadline = ktime_add_ms(ktime_get(), 50);
```

Convert when needed:

```c
s64 ns = ktime_to_ns(deadline);
```

Use `ktime_t` when jiffies granularity is not enough or when an API expects high-resolution time.

## Choosing A Clock

Common timekeeping accessors:

| API | Meaning | Use |
| --- | --- | --- |
| `ktime_get()` | monotonic time | elapsed intervals and most driver deadlines |
| `ktime_get_ns()` | monotonic time in ns | quick elapsed-time measurements |
| `ktime_get_boottime()` | includes suspend time | deadlines that should include suspend |
| `ktime_get_boottime_ns()` | boottime in ns | elapsed wall time including suspend |
| `ktime_get_real()` | wall-clock time | timestamps that intentionally follow real time |
| `ktime_get_real_ns()` | wall-clock time in ns | real-world timestamps |

Use monotonic time for most driver timeouts:

```c
u64 start = ktime_get_ns();

demo_do_operation(priv);

dev_dbg(priv->dev, "operation took %llu ns\n",
        ktime_get_ns() - start);
```

Use boottime if suspend time should count:

```c
u64 expires = ktime_get_boottime_ns() + 5ULL * NSEC_PER_SEC;
```

Avoid realtime for internal timeouts because wall-clock adjustments can jump forward or backward.

## Future Callback APIs

| Need | API |
| --- | --- |
| jiffies-based atomic callback | `struct timer_list` |
| high-resolution atomic callback | `struct hrtimer` |
| sleepable callback after delay | `struct delayed_work` |
| wait for condition with timeout | `wait_event*_timeout()` |
| sleep current task for duration | `msleep()`, `usleep_range()`, `schedule_timeout()` |

Examples:

Standard timer:

```c
mod_timer(&priv->timer, jiffies + msecs_to_jiffies(100));
```

Hrtimer:

```c
hrtimer_start(&priv->hrtimer, ms_to_ktime(5), HRTIMER_MODE_REL);
```

Delayed work:

```c
schedule_delayed_work(&priv->poll_work, msecs_to_jiffies(1000));
```

Wait with timeout:

```c
ret = wait_event_timeout(priv->wait, priv->ready,
                         msecs_to_jiffies(500));
if (!ret)
    return -ETIMEDOUT;
```

## Sleeping Delays

Sleeping delays give up the CPU and require sleepable context.

Use them in:

- probe/remove
- file operations
- sysfs methods
- workqueue callbacks
- threaded IRQ handlers

Do not use them in:

- hard IRQ handlers
- timer callbacks
- hrtimer callbacks
- spinlock-held sections
- RCU read-side critical sections that prohibit sleeping

Common APIs:

| API | Use |
| --- | --- |
| `msleep(ms)` | millisecond sleeps where precision is not critical |
| `usleep_range(min, max)` | short sleeps with tolerance |
| `schedule_timeout(timeout)` | sleep current task with task state already set |
| `wait_event_timeout()` | sleep until condition or timeout |

Example:

```c
ret = demo_write_reset(priv);
if (ret)
    return ret;

usleep_range(1000, 2000);

return demo_read_status(priv);
```

Use `usleep_range()` instead of a busy wait when the context can sleep and exact timing is not required. The range lets the scheduler coalesce wakeups.

## Busy-Wait Delays

Busy waits spin on the CPU. They are legal in atomic context, but they waste CPU time.

Common APIs:

| API | Use |
| --- | --- |
| `ndelay(n)` | nanosecond-scale hardware timing |
| `udelay(us)` | microsecond-scale hardware timing |
| `mdelay(ms)` | avoid except for very short, unavoidable hardware waits |

Example:

```c
demo_assert_reset(priv);
udelay(10);
demo_deassert_reset(priv);
```

Use busy waits only for short hardware-mandated delays where sleeping would be invalid or too imprecise. A long `mdelay()` in probe is usually a bug; use `msleep()` or `usleep_range()` if the context can sleep.

## Delay Selection Guide

| Situation | Prefer |
| --- | --- |
| wait 10 us in hard IRQ because hardware requires it | `udelay(10)` |
| wait 1 ms in workqueue | `usleep_range(1000, 2000)` |
| wait 100 ms in probe | `msleep(100)` |
| wait until device-ready bit or 500 ms | `wait_event_timeout()` or polling helper |
| call function after 250 ms and it can sleep | delayed work |
| call atomic timeout callback after 250 ms | timer |
| precise 500 us timeout callback | hrtimer |

Always consider whether the hardware offers an interrupt instead of polling or sleeping.

## Polling With Timeouts

Many drivers need to poll a register until a bit changes. Prefer subsystem helpers when available, such as regmap or I/O polling helpers.

Conceptual pattern:

```c
unsigned long timeout = jiffies + msecs_to_jiffies(100);

do {
    ret = demo_read_status(priv, &status);
    if (ret)
        return ret;

    if (status & DEMO_READY)
        return 0;

    usleep_range(1000, 2000);
} while (time_before(jiffies, timeout));

return -ETIMEDOUT;
```

This belongs only in sleepable context. In atomic context, use an interrupt, timer, hrtimer, or a short bounded busy wait.

## Timeouts And Wait Queues

A wait queue timeout combines condition waiting with a deadline.

```c
ret = wait_event_interruptible_timeout(priv->wait,
                                       priv->done,
                                       msecs_to_jiffies(500));
if (ret < 0)
    return ret;
if (ret == 0)
    return -ETIMEDOUT;

return 0;
```

The condition must be updated before the wakeup:

```c
spin_lock_irqsave(&priv->lock, flags);
priv->done = true;
spin_unlock_irqrestore(&priv->lock, flags);

wake_up_interruptible(&priv->wait);
```

The wait queue page covers this in more detail.

## Suspend And Time

Some clocks count suspend time and others do not.

Driver questions:

- Should a timeout expire while the system is suspended?
- Should a periodic check resume after suspend without "catching up"?
- Is the timeout tied to hardware state that loses power?
- Does runtime PM pause access to the device?

Use monotonic time for most in-kernel elapsed intervals. Use boottime when elapsed wall time including suspend matters.

For periodic polling, the suspend/resume callbacks may need to cancel and restart delayed work:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    cancel_delayed_work_sync(&priv->poll_work);
    return 0;
}

static int demo_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    schedule_delayed_work(&priv->poll_work, 0);
    return 0;
}
```

Subsystem PM rules may provide better patterns; follow them when available.

## Common Mistakes

- Comparing `jiffies` with plain relational operators.
- Using wall-clock time for internal timeouts.
- Calling `msleep()` in atomic context.
- Using `mdelay()` for long waits in sleepable context.
- Using a timer when delayed work is needed.
- Forgetting to cancel timers before freeing state.
- Forgetting that hrtimer callbacks still cannot sleep.
- Measuring elapsed time with realtime clocks that can jump.
- Open-coding `HZ` conversions.
- Polling when the hardware provides an interrupt.

## Practice Exercises

### Exercise 1: Jiffies Audit

Find open-coded comparisons like:

```c
if (jiffies > deadline)
```

Replace them with `time_after()` or related helpers.

### Exercise 2: Delay Audit

For each delay call in a driver, record:

```text
API
duration
context
may sleep?
could this be an interrupt or wait queue?
```

Replace long busy waits in sleepable context.

### Exercise 3: Clock Choice

For each timeout, decide whether suspend time should count. Choose monotonic or boottime and explain why.

## Debugging Checklist

- Check wraparound-safe comparisons.
- Check whether the current context may sleep.
- Check units and conversion helpers.
- Avoid long busy waits.
- Use delayed work for delayed sleepable callbacks.
- Use hrtimers only when standard timer granularity is insufficient.
- Avoid realtime clocks for internal deadlines.
- Check suspend/resume behavior for periodic work.

## Related Topics

- [Timers](timers.md)
- [Hrtimers](hrtimers.md)
- [Sleepable Vs Atomic Code](sleepable-vs-atomic-code.md)
- [Wait Queues And Completions](wait-queues-and-completions.md)

## Official References

- [ktime accessors](https://docs.kernel.org/core-api/timekeeping.html)
- [hrtimers](https://docs.kernel.org/timers/hrtimers.html)
- [NO_HZ: Reducing Scheduling-Clock Ticks](https://docs.kernel.org/timers/no_hz.html)
