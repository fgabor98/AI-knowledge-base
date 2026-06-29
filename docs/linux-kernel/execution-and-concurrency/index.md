---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Execution And Concurrency

This track covers where kernel code runs, what each context is allowed to do, and how drivers protect state that is shared between callbacks, interrupts, timers, workqueues, and userspace-facing operations.

It assumes you already know:

- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Common Driver Interfaces](../driver-interfaces/index.md)
- [Execution Context Primer](../foundations/execution-context-primer.md)

## What Problem Does This Solve?

Most serious driver bugs are not syntax problems. They are context, race, ordering, or lifetime problems.

Examples:

- sleeping in a hard IRQ handler
- taking a mutex while holding a spinlock
- freeing driver state while work is still queued
- returning from `remove()` while a timer can still fire
- reporting userspace data without locking shared state
- using atomics where a real lock is needed
- waiting forever because a wakeup raced with the condition update
- touching a device after runtime suspend powered it down

This chapter teaches the execution rules and synchronization primitives that keep driver callbacks coherent.

## Learning Materials

1. [Context Rules](context-rules.md)
2. [Sleepable Vs Atomic Code](sleepable-vs-atomic-code.md)
3. [Bottom Halves, Softirqs, And Tasklets](bottom-halves-softirqs-and-tasklets.md)
4. [Locking And Atomics](locking-and-atomics.md)
5. [Workqueues](workqueues.md)
6. [Concurrency Managed Workqueues](concurrency-managed-workqueues.md)
7. [Timers](timers.md)
8. [Hrtimers](hrtimers.md)
9. [Timekeeping And Kernel Timers](timekeeping-and-kernel-timers.md)
10. [Wait Queues And Completions](wait-queues-and-completions.md)
11. [Reference Counting And Lifetime](reference-counting-and-lifetime.md)

## Mental Model

Every callback has four questions:

```text
Where am I running?
May I sleep?
Who else can touch this state?
What keeps the object alive?
```

Answer those before choosing an API.

```text
hard IRQ
  cannot sleep
  use minimal work, spinlocks, atomics, wakeups, or IRQ thread

threaded IRQ
  can sleep
  useful for I2C/SPI/regmap-over-bus, mutexes, input/IIO reporting

workqueue
  can sleep
  useful for deferred work and longer processing

timer callback
  cannot sleep
  use to mark timeout and defer sleepable work

file/sysfs/probe/remove
  process context
  can usually sleep, unless holding atomic locks or called under constraints
```

## Context And Primitive Map

| Need | Common Primitive | Context Notes |
| --- | --- | --- |
| Protect state in process context | `struct mutex` | Sleepable only. |
| Protect state shared with hard IRQ | `spinlock_t` plus IRQ-safe variants | Do not sleep while held. |
| Count simple events | `atomic_t`, `atomic64_t` | Not a substitute for protecting compound state. |
| Wait for condition | wait queue | Sleepable waiter, wake from IRQ/work/process. |
| Wait for one-time completion | `struct completion` | Good for command-done or probe sequencing. |
| Defer sleepable work | workqueue or threaded IRQ | Must cancel/flush during teardown. |
| Run after future timeout | timer or delayed work | Timer callback cannot sleep; delayed work can. |
| Precise timeout | hrtimer | Callback still cannot sleep. |
| Object lifetime | refcount, `kref`, device refs | Required across async boundaries. |

## Typical Driver Concurrency Surfaces

A real driver may have all of these touching the same state:

```text
probe()
remove()
runtime_suspend()
runtime_resume()
IRQ handler
threaded IRQ handler
workqueue callback
timer callback
sysfs show/store
character device read/write/ioctl
poll()
userspace close()
```

The driver must define:

- which state each callback may access
- which lock protects that state
- which callbacks may sleep
- which callbacks may run after remove begins
- how new operations are blocked during teardown
- how queued or running async work is stopped

## Teardown Rule

Before freeing driver state, stop every asynchronous path that can use it:

```text
block new users
-> stop hardware events
-> disable/free IRQ or make handler harmless
-> delete timers
-> cancel delayed work
-> cancel normal work
-> wake sleeping waiters
-> wait for active users/references
-> unregister userspace/subsystem interfaces
-> free state
```

The exact order depends on the driver, but the invariant is stable:

```text
no callback may use memory after it is freed
```

## Debugging Tools To Enable Early

Development kernels should use debug options where practical:

- lockdep
- `CONFIG_DEBUG_ATOMIC_SLEEP`
- KASAN
- KCSAN
- DEBUG_OBJECTS timers/work where available
- dynamic debug
- ftrace
- panic/oops symbolization with matching `vmlinux`

These catch bugs while they are still close to the code change.

## Completion Criteria

You are ready to move on when you can:

- classify a callback as sleepable or atomic
- explain why hard IRQ handlers cannot call I2C/SPI or take mutexes
- move sleepable work to a threaded IRQ or workqueue
- choose mutex versus spinlock for a piece of state
- use wait queues without losing wakeups
- use completions for one-shot command completion
- schedule, cancel, and flush work safely
- use timers and hrtimers without sleeping in callbacks
- write wraparound-safe jiffies comparisons
- explain why atomics do not protect compound invariants
- stop timers/work/IRQs before freeing driver state
- explain who owns every pointer crossing an async boundary

## Common Mistakes

- Starting from a lock choice instead of first listing all access paths.
- Sleeping while holding a spinlock.
- Using `GFP_KERNEL` in atomic context.
- Calling I2C/SPI/regulator/clock APIs from hard IRQ or timer callbacks.
- Assuming `cancel_work_sync()` is optional during remove.
- Using a timer when delayed work would be simpler.
- Using a workqueue when a threaded IRQ directly matches the hardware event.
- Relying on atomics to protect multi-field state.
- Waking waiters without updating the condition first.
- Freeing state while file descriptors, work, timers, or IRQ handlers can still use it.

## Related Topics

- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)
- [Kernel Memory And I/O](../memory-and-io/index.md)
- [Kernel Debugging Basics](../debugging/index.md)
- [Power Management](../power-management/index.md)

## Official References

- [Workqueue](https://docs.kernel.org/core-api/workqueue.html)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Unreliable Guide To Locking](https://docs.kernel.org/kernel-hacking/locking.html)
- [ktime accessors](https://docs.kernel.org/core-api/timekeeping.html)
- [hrtimers](https://docs.kernel.org/timers/hrtimers.html)
- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
