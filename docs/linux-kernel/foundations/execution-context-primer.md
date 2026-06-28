---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Execution Context Primer

## What Problem Does This Solve?

Many driver bugs come from calling the right API from the wrong context. Kernel code does not always run as a normal process. Some callbacks can sleep; others must not. Some can take mutexes; others require spinlocks or deferral. Some can call I2C/SPI helpers; others cannot.

The first context question is:

> Can this code sleep?

If the answer is "no", the allowed APIs and locking choices are much narrower.

## Core Concepts

- process context
- interrupt context
- hard IRQ
- threaded IRQ
- softirq
- tasklet
- timer callback
- workqueue context
- sleepable context
- atomic context
- RCU preview
- per-CPU variable preview
- blocking APIs
- allocation flags
- mutex
- spinlock
- preemption

## Sleep Means Scheduling

In kernel discussions, "sleep" usually means the current execution path may block and let the scheduler run something else.

Examples of operations that may sleep:

- `mutex_lock()` when the mutex is contended
- `kmalloc(..., GFP_KERNEL)` under memory pressure
- I2C and SPI transfers
- `msleep()`
- waiting on a completion
- regulator, clock, or firmware calls that may block
- copying to or from userspace in paths where page faults may occur

If code runs in atomic context, these operations are not allowed.

## Common Contexts

| Context | May sleep? | Typical driver entry points |
|---|---:|---|
| process context | yes | file operations, probe, sysfs callbacks |
| workqueue context | yes | deferred work callbacks |
| threaded IRQ context | yes | `request_threaded_irq` thread function |
| hard IRQ context | no | primary IRQ handler |
| softirq/tasklet context | no | network softirq, tasklets, legacy bottom halves |
| timer callback | no | `timer_list` callback, hrtimer callback |
| spinlock-held region | no | any code while holding spinlock |

## Process Context

Process context means code runs on behalf of a task. It can usually sleep unless locks or other constraints prohibit it.

Examples:

- `probe`
- `remove`
- character device `open`, `read`, `write`, `ioctl`
- sysfs `show` and `store`
- workqueue callbacks

Example file operation:

```c
static ssize_t demo_read(struct file *file, char __user *buf,
                         size_t len, loff_t *ppos)
{
        struct demo_dev *demo = file->private_data;
        int ret;

        mutex_lock(&demo->lock);
        ret = demo_read_slow_status(demo);  /* may sleep */
        mutex_unlock(&demo->lock);

        if (ret < 0)
                return ret;

        return simple_read_from_buffer(buf, len, ppos,
                                       demo->text, strlen(demo->text));
}
```

This is allowed only if `demo_read_slow_status()` is called from sleepable context.

## Hard IRQ Context

Hard IRQ handlers run in interrupt context. They must be short and must not sleep.

Allowed style:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
        struct demo_dev *demo = data;
        u32 status;

        status = readl(demo->base + DEMO_STATUS);
        if (!(status & DEMO_STATUS_IRQ))
                return IRQ_NONE;

        writel(DEMO_STATUS_IRQ, demo->base + DEMO_STATUS);
        demo->irq_count++;

        return IRQ_WAKE_THREAD;
}
```

Do not do this in a hard IRQ handler:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
        struct demo_dev *demo = data;

        mutex_lock(&demo->lock);       /* wrong */
        i2c_smbus_read_byte(demo->client); /* wrong */
        msleep(20);                    /* wrong */
        mutex_unlock(&demo->lock);

        return IRQ_HANDLED;
}
```

The fix is often a threaded IRQ or workqueue.

## Threaded IRQ Context

Threaded IRQs split interrupt handling:

- primary hard handler: identify/acknowledge event quickly
- thread handler: perform sleepable follow-up work

Example:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
        struct demo_dev *demo = data;

        if (!demo_irq_pending(demo))
                return IRQ_NONE;

        demo_ack_irq(demo);
        return IRQ_WAKE_THREAD;
}

static irqreturn_t demo_irq_thread(int irq, void *data)
{
        struct demo_dev *demo = data;
        int ret;

        mutex_lock(&demo->lock);
        ret = demo_read_status_over_i2c(demo);
        mutex_unlock(&demo->lock);

        return ret < 0 ? IRQ_NONE : IRQ_HANDLED;
}
```

Registration:

```c
ret = devm_request_threaded_irq(dev, irq,
                                demo_irq,
                                demo_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(dev),
                                demo);
```

Use `IRQF_ONESHOT` when the interrupt line must remain masked while the threaded handler runs.

## Workqueue Context

Workqueues run callbacks in process context through kernel worker threads.

Example:

```c
static void demo_work_fn(struct work_struct *work)
{
        struct demo_dev *demo =
                container_of(work, struct demo_dev, work);

        mutex_lock(&demo->lock);
        demo_read_status_over_spi(demo);
        mutex_unlock(&demo->lock);
}
```

Schedule from IRQ:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
        struct demo_dev *demo = data;

        schedule_work(&demo->work);
        return IRQ_HANDLED;
}
```

Teardown:

```c
cancel_work_sync(&demo->work);
```

The `_sync` part matters: it waits for a running callback to finish before state is freed.

## Timer Context

Kernel timer callbacks do not run in ordinary sleepable process context.

Example:

```c
static void demo_timer_fn(struct timer_list *t)
{
        struct demo_dev *demo = from_timer(demo, t, timer);

        demo->expired = true;
        schedule_work(&demo->work);
}
```

Do not call sleepable APIs directly from timer callbacks. Defer to workqueue when needed.

Teardown:

```c
del_timer_sync(&demo->timer);
cancel_work_sync(&demo->work);
```

## Atomic Context

Atomic context means code must not sleep. Examples:

- hard IRQ handler
- softirq
- tasklet
- timer callback
- code holding a spinlock
- code with preemption or interrupts disabled

Debug kernels may report:

```text
BUG: sleeping function called from invalid context
```

This usually means a sleepable function was called while the kernel could not schedule.

## Locks And Context

| Lock | May sleep while acquiring? | Use in hard IRQ? | Typical use |
|---|---:|---:|---|
| mutex | yes | no | protect state in process context |
| spinlock | no | yes, with right variant | protect short critical sections in atomic context |
| rwsem | yes | no | read-mostly process-context state |
| atomic_t/refcount_t | no | yes | simple counters and references |

Example mutex:

```c
mutex_lock(&demo->lock);
demo->enabled = true;
mutex_unlock(&demo->lock);
```

Example spinlock:

```c
unsigned long flags;

spin_lock_irqsave(&demo->irq_lock, flags);
demo->irq_count++;
spin_unlock_irqrestore(&demo->irq_lock, flags);
```

Do not protect long I2C/SPI transactions with a spinlock.

## Allocation Flags

Memory allocation flags express whether the allocator may sleep.

| Flag | Meaning | Typical context |
|---|---|---|
| `GFP_KERNEL` | normal allocation, may sleep | probe, file ops, workqueue |
| `GFP_ATOMIC` | emergency-style allocation, must not sleep | hard IRQ or spinlock-held path |
| `GFP_DMA` | allocation suitable for DMA constraints | device-specific needs |

Example:

```c
data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL); /* probe: OK */
```

Avoid allocating in IRQ paths if possible. Pre-allocate buffers during probe.

## Bus Transfers And Context

I2C and SPI transfers usually sleep.

Bad:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
        struct demo_dev *demo = data;

        i2c_smbus_read_byte_data(demo->client, DEMO_STATUS); /* wrong */
        return IRQ_HANDLED;
}
```

Better:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
        return IRQ_WAKE_THREAD;
}

static irqreturn_t demo_irq_thread(int irq, void *data)
{
        struct demo_dev *demo = data;

        i2c_smbus_read_byte_data(demo->client, DEMO_STATUS);
        return IRQ_HANDLED;
}
```

## Common API Context Rules

| API or operation | Sleepable context required? | Notes |
|---|---:|---|
| `mutex_lock` | yes | can block |
| `spin_lock` | no | keep section short |
| `kmalloc(..., GFP_KERNEL)` | yes | normal allocation |
| `kmalloc(..., GFP_ATOMIC)` | no | avoid unless needed |
| I2C/SPI transfers | yes | use threaded IRQ/workqueue |
| `readl`/`writel` | no | MMIO accessors are atomic-context compatible |
| `copy_to_user` | yes in normal usage | can fault; use in file ops |
| `msleep` | yes | process context only |
| `udelay` | no | short busy wait only |
| `wait_for_completion` | yes | blocking wait |
| `complete` | no | can be called from IRQ |
| `wake_up` | no | often called from IRQ |

## Example: Fixing A Context Bug

Buggy design:

```text
GPIO interrupt
-> hard IRQ handler
-> reads I2C status register
-> reports input event
```

Problem:

I2C transfer may sleep.

Better design:

```text
GPIO interrupt
-> hard IRQ handler returns IRQ_WAKE_THREAD
-> threaded IRQ reads I2C status
-> reports input event
```

Alternative:

```text
GPIO interrupt
-> hard IRQ handler schedules work
-> workqueue reads I2C status
-> reports input event
```

Choose threaded IRQ when the work is directly tied to that interrupt and should be serialized with interrupt masking. Choose workqueue when the work is broader deferred processing.

## Advanced Concurrency Preview: RCU And Per-CPU Data

Some kernel concurrency tools are important, but they are not beginner tools to reach for first.

### RCU

RCU means read-copy update. It is used for read-mostly data where readers must be very cheap and updates are more carefully staged.

You may see patterns like:

```c
rcu_read_lock();
p = rcu_dereference(global_ptr);
if (p)
        do_something_read_only(p);
rcu_read_unlock();
```

Update-side code uses different rules and must wait for readers before freeing replaced data.

Beginner rule:

- recognize RCU when reading kernel internals
- do not introduce RCU into a simple driver until mutexes, spinlocks, and lifetime rules are clearly insufficient
- never free RCU-protected data with ordinary `kfree` immediately after publishing a replacement

### Per-CPU Variables

Per-CPU data gives each CPU its own instance of a variable. It is useful for counters or hot-path data where sharing one global variable would cause contention.

You may see declarations like:

```c
static DEFINE_PER_CPU(unsigned long, demo_events);
```

Conceptual use:

```c
this_cpu_inc(demo_events);
```

Beginner rule:

- per-CPU data is for hot paths and scalability
- normal driver state should usually start with simple per-device fields plus appropriate locks
- reading a global total from per-CPU counters requires summing all CPUs carefully

These mechanisms are worth recognizing early, but they belong in advanced concurrency material once the basic context and locking model is stable.

## Common Mistakes

- Calling I2C/SPI helpers in hard IRQ context.
- Taking a mutex while holding a spinlock.
- Using `GFP_KERNEL` inside a spinlock.
- Forgetting timer callbacks cannot sleep.
- Freeing driver state before workqueue callbacks finish.
- Assuming `probe` is always atomic because it is kernel code.
- Using spinlocks for long operations that should use a mutex.
- Introducing RCU or per-CPU state before the simpler locking model is understood.

## Debugging Checklist

- What callback is running?
- Who calls this callback?
- Can this callback sleep?
- Is a spinlock held?
- Are interrupts disabled?
- Does this path allocate memory?
- Does this path call I2C/SPI/regulator/firmware APIs?
- Does teardown wait for this callback to finish?

## Related Topics

- [Context Rules](../execution-and-concurrency/context-rules.md)
- [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Workqueues](../execution-and-concurrency/workqueues.md)

## References

- Driver basics: <https://docs.kernel.org/driver-api/basics.html>
- Workqueue documentation: <https://docs.kernel.org/core-api/workqueue.html>
- Generic IRQ documentation: <https://docs.kernel.org/core-api/genericirq.html>
