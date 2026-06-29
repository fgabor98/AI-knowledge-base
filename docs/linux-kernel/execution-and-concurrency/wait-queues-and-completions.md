---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Wait Queues And Completions

## What Problem Does This Solve?

Wait queues and completions let sleepable code wait for a condition or one-time event without busy waiting.

They are the normal answer when a driver needs to block a process until:

- an interrupt reports data
- a hardware command completes
- a buffer becomes available
- a device state changes
- teardown asks sleeping users to exit
- a one-time initialization event finishes

They are not locks. They coordinate sleeping and wakeup. The actual state must still be protected by a lock or by carefully designed lockless access.

## Core Concepts

- wait queue
- condition predicate
- wakeup
- completion
- timeout
- interruptible sleep
- poll integration
- lost wakeups

## Mental Model

The condition is the truth. Wakeups are only notifications that the condition may have changed.

```text
waiter:
  check condition
  sleep if false
  wake later
  check condition again

waker:
  update condition
  wake waiters
```

If the wakeup happens but the condition is false, the waiter should keep waiting. If the condition is true before the wakeup, the waiter should not sleep.

This is why wait queues are always built around a condition expression.

## Wait Queue Basics

Embed a wait queue head in driver state:

```c
struct demo_priv {
    wait_queue_head_t read_wq;
    spinlock_t lock;
    bool data_ready;
    bool stopping;
};
```

Initialize:

```c
init_waitqueue_head(&priv->read_wq);
```

For state protected by a lock, put the condition check behind a helper:

```c
static bool demo_data_ready(struct demo_priv *priv)
{
    unsigned long flags;
    bool ready;

    spin_lock_irqsave(&priv->lock, flags);
    ready = priv->data_ready;
    spin_unlock_irqrestore(&priv->lock, flags);

    return ready;
}
```

Wait:

```c
ret = wait_event_interruptible(priv->read_wq,
                               demo_data_ready(priv) ||
                               READ_ONCE(priv->stopping));
if (ret)
    return ret;
```

Wake:

```c
WRITE_ONCE(priv->data_ready, true);
wake_up_interruptible(&priv->read_wq);
```

For a single lockless flag, `READ_ONCE()` can be enough if the rest of the design is built for it. For compound conditions or related data, use a lock.

## Producer And Consumer Example

IRQ producer:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    unsigned long flags;

    spin_lock_irqsave(&priv->lock, flags);
    priv->status = demo_read_irq_status(priv);
    priv->data_ready = true;
    spin_unlock_irqrestore(&priv->lock, flags);

    wake_up_interruptible(&priv->read_wq);
    return IRQ_HANDLED;
}
```

Read consumer:

```c
static ssize_t demo_read(struct file *filp, char __user *buf,
                         size_t count, loff_t *ppos)
{
    struct demo_priv *priv = filp->private_data;
    unsigned long flags;
    u32 status;
    int ret;

    ret = wait_event_interruptible(priv->read_wq,
                                   demo_data_ready(priv) ||
                                   READ_ONCE(priv->stopping));
    if (ret)
        return ret;

    if (READ_ONCE(priv->stopping))
        return -ENODEV;

    spin_lock_irqsave(&priv->lock, flags);
    status = priv->status;
    priv->data_ready = false;
    spin_unlock_irqrestore(&priv->lock, flags);

    if (copy_to_user(buf, &status, sizeof(status)))
        return -EFAULT;

    return sizeof(status);
}
```

This example keeps the spinlock around the shared status fields only. `copy_to_user()` happens after unlocking because it may sleep.

For real drivers, consider multiple readers, FIFO semantics, and whether clearing `data_ready` after one read is correct.

## Lost Wakeups

A lost wakeup usually means the wakeup and the condition update were not designed together.

Wrong:

```c
wake_up_interruptible(&priv->read_wq);
priv->data_ready = true;
```

The waiter can wake, see `data_ready == false`, go back to sleep, and miss the later condition update.

Better:

```c
WRITE_ONCE(priv->data_ready, true);
wake_up_interruptible(&priv->read_wq);
```

For locked state:

```c
spin_lock_irqsave(&priv->lock, flags);
priv->data_ready = true;
spin_unlock_irqrestore(&priv->lock, flags);

wake_up_interruptible(&priv->read_wq);
```

Update condition first, then wake.

## Wait Variants

| API | Use |
| --- | --- |
| `wait_event()` | uninterruptible wait until condition is true |
| `wait_event_interruptible()` | wait can be interrupted by signals |
| `wait_event_killable()` | wait can be interrupted by fatal signals |
| `wait_event_timeout()` | uninterruptible wait with timeout |
| `wait_event_interruptible_timeout()` | signal-aware wait with timeout |

For user-facing file operations, interruptible waits are usually better:

```c
ret = wait_event_interruptible(priv->read_wq, demo_can_read(priv));
if (ret)
    return ret;
```

Uninterruptible waits can make processes hard to kill and should be justified.

## Timeout Return Values

Timeout wait APIs need careful return handling.

```c
ret = wait_event_interruptible_timeout(priv->wait,
                                       READ_ONCE(priv->done),
                                       msecs_to_jiffies(500));
if (ret < 0)
    return ret;
if (ret == 0)
    return -ETIMEDOUT;

return 0;
```

Positive return means the condition became true before the timeout expired, with the returned value representing remaining jiffies for these APIs.

Do not collapse all non-positive values into timeout or you will mishandle signals.

## Blocking Reads

A character device read commonly waits for data unless opened with `O_NONBLOCK`.

```c
static ssize_t demo_read(struct file *filp, char __user *buf,
                         size_t count, loff_t *ppos)
{
    struct demo_priv *priv = filp->private_data;
    int ret;

    if (filp->f_flags & O_NONBLOCK) {
        if (!demo_data_available(priv))
            return -EAGAIN;
    } else {
        ret = wait_event_interruptible(priv->read_wq,
                                       demo_data_available(priv) ||
                                       READ_ONCE(priv->stopping));
        if (ret)
            return ret;
    }

    if (READ_ONCE(priv->stopping))
        return -ENODEV;

    return demo_copy_data_to_user(priv, buf, count);
}
```

`demo_data_available()` must be safe to call from the wait condition. If it inspects compound state, protect that state.

## Poll Integration

`poll()` and `select()` need to register the wait queue and return readiness bits.

```c
static __poll_t demo_poll(struct file *filp, poll_table *wait)
{
    struct demo_priv *priv = filp->private_data;
    __poll_t mask = 0;

    poll_wait(filp, &priv->read_wq, wait);

    if (demo_data_available(priv))
        mask |= EPOLLIN | EPOLLRDNORM;
    if (demo_space_available(priv))
        mask |= EPOLLOUT | EPOLLWRNORM;
    if (READ_ONCE(priv->stopping))
        mask |= EPOLLHUP;
    if (READ_ONCE(priv->error))
        mask |= EPOLLERR;

    return mask;
}
```

The sequence is intentional:

1. register the wait queue with `poll_wait()`
2. check current readiness
3. return readiness mask

If readiness changes later, the producer calls `wake_up_interruptible()` or a related wakeup function.

## Waking During Teardown

Remove paths must wake sleepers that are waiting on conditions that will never become true.

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    WRITE_ONCE(priv->stopping, true);
    wake_up_interruptible_all(&priv->read_wq);

    disable_irq(priv->irq);
    cancel_work_sync(&priv->work);
}
```

Wait condition:

```c
ret = wait_event_interruptible(priv->read_wq,
                               demo_data_available(priv) ||
                               READ_ONCE(priv->stopping));
```

Without the stopping condition and wakeup, a process blocked in `read()` may sleep forever during device removal.

## Completions

Completions are for waiting until a one-time event happens.

Common uses:

- command finished
- firmware load step finished
- hardware reset completed
- probe sequencing between two contexts
- async callback finished initialization

They are simpler than wait queues when the event is one-shot and does not require a custom predicate.

```c
struct demo_priv {
    struct completion cmd_done;
    int cmd_status;
};
```

Initialize:

```c
init_completion(&priv->cmd_done);
```

Wait:

```c
ret = wait_for_completion_timeout(&priv->cmd_done,
                                  msecs_to_jiffies(500));
if (!ret)
    return -ETIMEDOUT;
```

Complete:

```c
priv->cmd_status = status;
complete(&priv->cmd_done);
```

Use `complete_all()` when all waiters must be released.

## Completion Command Example

Before starting a new command:

```c
reinit_completion(&priv->cmd_done);
priv->cmd_status = 0;

ret = demo_start_command(priv);
if (ret)
    return ret;
```

IRQ or threaded IRQ completion:

```c
static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_priv *priv = data;

    priv->cmd_status = demo_read_command_status(priv);
    complete(&priv->cmd_done);

    return IRQ_HANDLED;
}
```

Waiter:

```c
ret = wait_for_completion_interruptible_timeout(&priv->cmd_done,
                                                msecs_to_jiffies(500));
if (ret < 0)
    return ret;
if (ret == 0)
    return -ETIMEDOUT;

return priv->cmd_status;
```

Protect `cmd_status` with a lock if multiple commands or concurrent users can touch it. A completion only signals the event; it does not automatically serialize all related state.

## `reinit_completion()` Caution

Only reinitialize a completion when you know no waiter still depends on the previous event.

Wrong:

```c
reinit_completion(&priv->cmd_done); /* while another thread may wait */
```

Better:

```c
mutex_lock(&priv->cmd_lock);
reinit_completion(&priv->cmd_done);
ret = demo_start_command_locked(priv);
mutex_unlock(&priv->cmd_lock);
```

Use a command mutex or state machine to ensure only one command owns the completion at a time.

## Wait Queue Versus Completion

| Need | Prefer |
| --- | --- |
| wait until arbitrary condition is true | wait queue |
| wait for data availability repeatedly | wait queue |
| support `poll()`/`select()` | wait queue |
| wait for one command completion | completion |
| wait for one initialization step | completion |
| wake all users during teardown | wait queue or `complete_all()` depending on design |

If you need a predicate, use a wait queue. If you need a one-shot done signal, use a completion.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| process sleeps forever | teardown did not set condition and wake | remove path |
| missed event | wakeup before condition update | producer ordering |
| busy loop | poll returns ready without consumable data | readiness condition |
| signal ignored | used uninterruptible wait | wait variant |
| timeout mishandled | wrong return-value handling | timeout logic |
| stale command status | completion reused without serialization | command lock |
| data race in condition | predicate reads unlocked compound state | condition protection |

## Practice Exercises

### Exercise 1: Blocking Read

Implement blocking read behavior with:

```text
wait queue
data-ready condition
O_NONBLOCK handling
interruptible wait
teardown wakeup
```

Explain which state the condition reads and how it is protected.

### Exercise 2: Poll Support

Add `poll()` to the same device. Return readable readiness only when a read would not block.

### Exercise 3: Completion Command

Implement a command that starts in process context and completes in an IRQ thread. Use a completion, timeout handling, and a command mutex.

## Debugging Checklist

- Check the condition under the right lock.
- Handle signals and timeouts.
- Avoid sleeping while holding a spinlock.
- Check for teardown paths that must wake waiters.
- Update the condition before waking.
- Include stopping/error conditions in waits.
- Return `-EAGAIN` for nonblocking operations that would block.
- Use completions for one-shot events, not general predicates.

## Related Topics

- [Character Device Basics](../fundamentals/character-device-basics.md)
- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Locking And Atomics](locking-and-atomics.md)
- [Timers](timers.md)

## Official References

- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [Unreliable Guide To Locking](https://docs.kernel.org/kernel-hacking/locking.html)
