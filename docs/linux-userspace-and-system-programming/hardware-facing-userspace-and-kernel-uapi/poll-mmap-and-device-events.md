---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# poll, mmap, And Device Events

## What problem does this solve?

Many devices produce data asynchronously. `poll`/`epoll` can wait for readiness and
`mmap` can expose buffers, but neither defines event meaning, ownership, completion,
or cache behavior. Those must come from the subsystem UAPI.

## Readiness

An event may mean data available, space available, state change, error, hangup, or
completion. Attempt nonblocking I/O after readiness and handle short data, `EAGAIN`,
`EIO`, `ENODEV`, and HUP. Drain according to the UAPI and apply a bounded work budget.

## Mapped buffers

Record address, length, offset, protection, queue state, and generation. Validate
metadata before deriving payload pointers. The driver may require a queue/dequeue
protocol, cache synchronization, or an explicit buffer release. Do not unmap while
another thread or in-flight device operation uses the range.

```text
allocate/prepare -> queue -> device fills -> event -> dequeue/validate -> requeue
```

Hardware DMA adds cache coherency and memory-order rules. Userspace cannot infer them
from `mmap` success. Follow V4L2/ALSA/UIO/VFIO or device-specific documentation.

## Event loss and reset

Events can coalesce, overflow, or become stale after reset. Use sequence numbers or
state queries. On disconnect/reset, stop queues, invalidate buffers, and reject old
generation completions.

## Common mistakes

- Treating readiness as data completion.
- Reading mapped buffers before ownership transfer.
- Unmapping while DMA or another thread is active.
- Ignoring event overflow, HUP, reset, and stale generations.
- Assuming userspace atomics solve device cache visibility.

## Debugging checklist

- Record event mask, sequence, buffer state, generation, and timestamps.
- Correlate poll events with actual transfer results and driver logs.
- Test no data, short data, overflow, timeout, reset, unplug, and close races.
- Verify cache/DMA rules and buffer ownership from subsystem documentation.
- Inspect `/proc/<pid>/maps` and FD flags for actual mappings.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)
- [mmap, Files, And Shared Memory](../process-memory-and-mapping/mmap-files-and-shared-memory.md)
- [Event Loops: select, poll, And epoll](../ipc-and-event-driven-design/event-loops-select-poll-and-epoll.md)

## References

- [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html)
- [`epoll(7)`](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [`mmap(2)`](https://man7.org/linux/man-pages/man2/mmap.2.html)
- [Linux kernel DMA API](https://www.kernel.org/doc/html/latest/core-api/dma-api.html)
