---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Cancellation, Priority, And Real-Time Scheduling

## What problem does this solve?

Stopping a thread at an arbitrary instruction can leave locks held, invariants
broken, buffers owned twice, or device commands in flight. Scheduling priority can
also create inversion and starvation. This page treats cancellation and real-time
policy as explicit lifecycle and system-resource decisions.

## Cooperative cancellation

Prefer a stop flag/queue command plus a bounded wakeup. The worker reaches a safe
point, stops accepting work, finishes or rolls back its current operation, releases
resources, and returns. This makes ownership visible and allows a device protocol to
cancel or invalidate late completion.

## POSIX cancellation

Deferred cancellation acts at cancellation points such as selected blocking calls or
an explicit `pthread_testcancel`. Cleanup handlers execute during cancellation:

```text
push cleanup for mutex/resource
enter cancellation point
cleanup unlocks/releases in reverse order
thread terminates as PTHREAD_CANCELED
```

Disable cancellation around short invariant updates and resource-transfer windows.
Avoid asynchronous cancellation; it can strike while any instruction runs and makes
ordinary C cleanup reasoning nearly impossible. Ensure cleanup handlers match every
resource acquired before the cancellation point.

## Priority inversion

Priority inversion occurs when a high-priority thread waits for a lock held by a low-
priority thread while medium-priority work prevents the low-priority owner from
running. Priority inheritance or priority-protection mutex protocols can help where
supported, but they require limits, permissions, and correct lock design.

Do not solve inversion by globally raising priority. Shorten critical sections,
avoid blocking I/O under shared locks, and give control paths bounded resources.

## Scheduling policies

Linux policies include normal fair scheduling and real-time `SCHED_FIFO`/`SCHED_RR`.
Real-time threads can starve ordinary tasks, and a misconfigured FIFO thread can
make a target appear dead. Affinity can improve isolation but can also overload one
CPU or prevent balancing. `chrt`, `taskset`, and cgroup policy are operational
controls, not substitutes for measurement.

```sh
chrt -p "$pid"
taskset -pc "$pid"
cat /proc/$pid/sched
```

Privileges and `RLIMIT_RTPRIO`/`RLIMIT_RTTIME` may limit changes. Record the exact
policy, priority, affinity, cgroup, kernel configuration, and target CPU topology.

## Real-time proof

A real-time claim needs a bounded path: allocation, page faults, locks, system calls,
interrupt interference, scheduler latency, device response, and logging. Measure
worst-case release-to-completion under representative load and power/thermal modes.
“Uses a real-time priority” is not a proof.

## Common mistakes

- Cancelling while holding a mutex or owning a partially updated resource.
- Using asynchronous cancellation in ordinary application code.
- Assuming every blocking function is a cancellation point.
- Raising priority without a starvation and privilege analysis.
- Ignoring priority inheritance and lock ordering.
- Pinning all workers to one CPU and causing overload.
- Calling malloc, filesystem, or logging code in an unbounded real-time path.

## Debugging checklist

- Record cancellation state/type, cleanup stack, owner, and cancellation point.
- Test cancellation during every blocking call and resource-transfer window.
- Inspect `chrt`, affinity, cgroups, limits, and `/proc/<pid>/sched`.
- Measure lock hold/wait times and priority inversion under load.
- Test starvation, CPU isolation, thermal throttling, and worker failure.
- Verify shutdown joins all threads and no cancelled thread retains shared state.

## Related topics

- [Stage 6: Threads And Userspace Concurrency](index.md)
- [pthread Lifecycle And Thread Attributes](pthread-lifecycle-and-thread-attributes.md)
- [Worker Pools, Bounded Queues, And Backpressure](worker-pools-bounded-queues-and-backpressure.md)
- [Timers And Periodic Work](../time-clocks-and-signals/timers-and-periodic-work.md)

## References

- [`pthread_cancel(3)`](https://man7.org/linux/man-pages/man3/pthread_cancel.3.html)
- [`pthreads(7)`](https://man7.org/linux/man-pages/man7/pthreads.7.html)
- [`sched(7)`](https://man7.org/linux/man-pages/man7/sched.7.html)
- [`chrt(1)`](https://man7.org/linux/man-pages/man1/chrt.1.html)
- [`sched_setaffinity(2)`](https://man7.org/linux/man-pages/man2/sched_setaffinity.2.html)
