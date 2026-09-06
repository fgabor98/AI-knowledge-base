---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Memory Pressure, OOM, And Real-Time Constraints

## What problem does this solve?

An application can fail because allocation is unavailable, because the kernel reclaims
memory, because a cgroup limit is reached, or because the OOM killer selects it. A
latency-sensitive service can also miss a deadline because a page fault, allocator
lock, filesystem operation, or reclaim path blocks. Correctness and timing require a
budget, not optimism about free memory.

## Memory measurements

| Measure | Meaning |
| --- | --- |
| Virtual size (`VmSize`) | Address space reserved or mapped, not necessarily RAM |
| RSS (`VmRSS`) | Resident pages currently associated with the process |
| PSS | RSS weighted across sharing, useful for proportional cost |
| Anonymous memory | Heap, stacks, and anonymous mappings |
| File-backed memory | Executable, libraries, and mapped files |
| Dirty pages | Modified pages needing writeback |
| Commit/overcommit | Kernel policy about promised virtual memory |
| cgroup memory | Accounted usage and limits for a resource-control group |

```sh
cat /proc/meminfo
cat /proc/$pid/status
cat /proc/$pid/smaps_rollup 2>/dev/null
cat /sys/fs/cgroup/memory.current 2>/dev/null
cat /sys/fs/cgroup/memory.events 2>/dev/null
```

Values change while reading. Record timestamps, workload, cgroup, and kernel policy.

## Allocation failure and fragmentation

`malloc` can return `NULL`, but an allocation can also succeed and fault later on
first touch. Large contiguous allocations can fail despite aggregate free memory.
Fragmentation, limits, address-space availability, overcommit, and allocator arenas
all matter. Check size arithmetic before allocation and set upper bounds for queues,
frames, logs, mappings, and worker counts.

Never continue with a null pointer. Define whether a service rejects a request, drops
optional cache, enters degraded mode, or terminates when a required allocation fails.

## OOM killer and cgroups

When reclaim and policy cannot satisfy memory demand, the kernel may invoke the OOM
killer. A cgroup can reach its own limit even while the host has free memory. The
selected process may be a worker rather than the service leader, leaving a partially
alive service. Capture kernel/cgroup OOM events and design a supervisor response.

Avoid relying on `oom_score_adj` as a substitute for a complete resource policy. Set
limits and priorities only with an explicit product decision; overly protected
processes can starve the rest of the system.

## Real-time memory concerns

Latency-sensitive code should avoid unbounded allocation, page faults, blocking
filesystem operations, and unexpected library work in its critical path. Possible
tools include:

- pre-allocation and bounded pools;
- prefaulting and `mlockall` where allowed;
- explicit thread stacks and guard pages;
- controlled allocator behavior;
- cgroups and resource limits;
- measuring worst-case, not just average, latency.

`mlock`/`mlockall` consume a limited locked-memory budget and can fail with `EPERM`
or `ENOMEM`. Locking memory does not make code nonblocking: mutexes, drivers,
interrupts, CPU contention, and I/O can still delay it.

## Memory pressure tests

Use a disposable cgroup or controlled stress workload, not an unbounded command on a
production device. Test allocation failure, reclaim, cgroup limit, worker death,
recovery, and logging under pressure. Keep enough memory and storage for the failure
diagnostic path.

## Common mistakes

- Treating free memory as a guaranteed allocation budget.
- Confusing virtual size with resident cost.
- Ignoring cgroup limits and OOM events.
- Allocating in a real-time callback without a bound.
- Calling `mlockall` without checking limits and privilege.
- Assuming page faults are impossible after `malloc`.
- Letting memory pressure consume all logging and recovery capacity.

## Debugging checklist

- Record process/cgroup memory, limits, workload, and kernel version.
- Inspect `/proc/meminfo`, status, smaps, cgroup events, and kernel logs.
- Check allocation sizes, overflow, fragmentation, and lifetime leaks.
- Correlate latency spikes with page faults, reclaim, writeback, and scheduler events.
- Test required versus optional allocation policy.
- Verify supervisor behavior when a worker or service is OOM-killed.

## Related topics

- [Stage 4: Process Memory And Mapping](index.md)
- [Process Address Space](process-address-space.md)
- [Memory Protection And Process Hardening](memory-protection-and-hardening.md)
- [Memory Pressure And Realtime Scheduling](../threads-and-userspace-concurrency/cancellation-priority-and-realtime-scheduling.md)

## References

- [`malloc(3)`](https://man7.org/linux/man-pages/man3/malloc.3.html)
- [`proc(5)`](https://man7.org/linux/man-pages/man5/proc.5.html)
- [`proc_pid_status(5)`](https://man7.org/linux/man-pages/man5/proc_pid_status.5.html)
- [`mlock(2)`](https://man7.org/linux/man-pages/man2/mlock.2.html)
- [Linux kernel cgroup v2 memory controller](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)
