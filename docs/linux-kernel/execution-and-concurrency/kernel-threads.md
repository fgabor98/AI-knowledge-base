---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Threads

## What Problem Does This Solve?

A kernel thread (kthread) is a schedulable task that runs only kernel code. It suits work that owns a long-running loop: polling hardware that has no interrupt, feeding a FIFO, or running a protocol state machine with its own scheduling priority.

Most deferred driver work does not need one. Use a threaded IRQ when the work follows an interrupt, and a workqueue when it is a sequence of independent items.

## Core Concepts

- `kthread_run()`, `kthread_create()`, `wake_up_process()`
- `kthread_should_stop()`, `kthread_stop()`
- `struct task_struct`
- wait queue with a stop condition
- `kthread_worker`
- scheduling policy and CPU affinity
- freezer

## Mental Model

A kthread is a function with a loop, a reason to sleep, and a way out:

```text
create
-> loop:
     sleep until there is work OR someone asked me to stop
     if stop requested: leave
     do one bounded unit of work
-> return value is handed to kthread_stop()
```

The thread does not share the caller's lifetime. Whoever creates it must stop it before the data or the module code it uses disappears.

## Basic Pattern

```c
static int demo_thread(void *data)
{
    struct demo_priv *priv = data;

    while (!kthread_should_stop()) {
        wait_event_interruptible(priv->wq,
                                 demo_has_work(priv) ||
                                 kthread_should_stop());

        if (demo_has_work(priv))
            demo_process_one(priv);
    }

    return 0;
}
```

Start and check for an encoded error:

```c
priv->task = kthread_run(demo_thread, priv, "demo/%s", dev_name(dev));
if (IS_ERR(priv->task))
    return PTR_ERR(priv->task);
```

Stop:

```c
kthread_stop(priv->task);
```

`kthread_stop()` sets the stop flag, wakes the thread, and waits until the function returns. Three rules follow:

- **The stop condition must be part of the wait.** If the wait expression ignores `kthread_should_stop()`, the thread goes back to sleep and `kthread_stop()` never returns.
- **Do not return early.** If the thread function returns on its own and its task structure is released, a later `kthread_stop()` uses freed memory. Either loop until stopped, or hold a reference with `get_task_struct()` before it can exit and drop it after `kthread_stop()`.
- **Do not hold a lock that the thread needs** while calling `kthread_stop()`.

A kthread has no signal handling unless it opts in, so `wait_event_interruptible()` here is not about signals. It keeps the sleep from counting toward the load average and from triggering hung-task warnings. `wait_event_idle()` states that intent directly.

## Create Without Starting

Use `kthread_create()` to configure the task before it runs:

```c
task = kthread_create(demo_thread, priv, "demo");
if (IS_ERR(task))
    return PTR_ERR(task);

kthread_bind(task, 1);          /* only before the first wakeup */
wake_up_process(task);
```

For a real-time priority use `sched_set_fifo()` or `sched_set_fifo_low()`. Drivers do not choose a numeric priority; the administrator can change it with `chrt`.

## kthread_worker

When a private thread is needed only for ordered items at a controlled priority, `kthread_worker` provides a queue on top of a dedicated thread:

```c
priv->worker = kthread_run_worker(0, "demo_worker");
if (IS_ERR(priv->worker))
    return PTR_ERR(priv->worker);

kthread_init_work(&priv->work, demo_kwork_fn);
kthread_queue_work(priv->worker, &priv->work);

/* teardown */
kthread_destroy_worker(priv->worker);
```

Before 6.14 the creating call was `kthread_create_worker(0, "demo_worker")`. SPI controller drivers use this to give message pumping a real-time priority.

## Choosing A Mechanism

| Need | Prefer |
| --- | --- |
| follow-up to one interrupt | threaded IRQ |
| independent items, occasional | workqueue |
| periodic sleepable polling | delayed work |
| ordered items at a specific priority | `kthread_worker` |
| continuous loop with its own state | kthread |
| atomic, very short | BH work, timer |

## Suspend And The Freezer

Kernel threads are not frozen during system suspend unless they call `set_freezable()` and `try_to_freeze()`. Prefer stopping the activity explicitly in the driver's suspend callback; that also covers runtime PM.

## Sharing Data With The Thread

A kthread runs in process context and may sleep, but it is concurrent with every other path into the driver. Protect shared state as for a workqueue: a mutex against process context, an IRQ-safe spinlock against an interrupt handler.

`current` is the kthread itself. It has no userspace address space, so `copy_to_user()` and `copy_from_user()` are meaningless there.

## Inspecting

```sh
ps -e -o pid,cls,rtprio,psr,stat,comm | grep demo
sudo cat /proc/$(pgrep -x demo)/stack
```

Kernel threads are shown in brackets by `ps` and have no `cmdline`. The `stack` file shows where a thread is blocked.

## Runnable Example

`examples/linux-kernel/small-labs/13-deferred-contexts` compares a kthread, normal work, and BH work triggered by one timer. Lab 14 uses kthreads as concurrent readers.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `rmmod` hangs | stop condition missing from the wait | `/proc/PID/stack` of the thread and of `rmmod` |
| crash on unload | thread not stopped before its data or code was freed | exit and remove order |
| use-after-free in `kthread_stop()` | thread returned early | function exit paths |
| one CPU at 100 % | loop without a sleep or with an always-true condition | wait expression |
| soft lockup or RCU stall | long loop without `cond_resched()` | stack in the report |
| high load average while idle | uninterruptible sleep | `wait_event()` used for an idle wait |

## Practice Exercises

1. In a VM, remove `kthread_should_stop()` from the wait in Lab 13, unload, and find the hang with `/proc/PID/stack`.
2. Replace the kthread in Lab 13 with a `kthread_worker` and compare the code.
3. Take a driver that creates a kthread for periodic polling and decide whether delayed work would be simpler. State what would be lost.

## Debugging Checklist

- Does the wait expression include `kthread_should_stop()`?
- Is every wakeup preceded by a change to the waited-for condition?
- Is the result of `kthread_run()` tested with `IS_ERR()`?
- Is `kthread_stop()` called exactly once, before freeing shared state?
- Does the loop sleep or call `cond_resched()`?

## Related Topics

- [Workqueues](workqueues.md)
- [Wait Queues And Completions](wait-queues-and-completions.md)
- [Bottom Halves, Softirqs, And Tasklets](bottom-halves-softirqs-and-tasklets.md)
- [RCU](rcu.md)

## Official References

- [Driver Basics: kthread API](https://docs.kernel.org/driver-api/basics.html)
- [Freezing of tasks](https://docs.kernel.org/power/freezing-of-tasks.html)
- [Workqueue](https://docs.kernel.org/core-api/workqueue.html)
