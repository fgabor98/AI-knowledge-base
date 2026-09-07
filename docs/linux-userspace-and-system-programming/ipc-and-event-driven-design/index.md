---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 7: IPC And Event-Driven Design

Interprocess communication is a product protocol, even when both endpoints are on
one board. The mechanism—pipe, Unix socket, shared memory, eventfd, or another Linux
facility—determines transport properties, but the application must still define
framing, ownership, authentication, bounds, versioning, overload, peer death, and
shutdown.

## Choose by contract, not fashion

```text
small stream      -> pipe / socketpair
local request     -> Unix-domain socket
large data        -> shared memory + control channel
counter/wakeup    -> eventfd
timer             -> timerfd
signal            -> signalfd
filesystem hint  -> inotify + state rescan
many FDs/events   -> poll / epoll event loop
```

The mechanism carries events or bytes. It does not define the meaning of a message,
whether a command is idempotent, or what happens after a restart.

## Learning materials

1. [IPC Selection And Failure Models](ipc-selection-and-failure-models.md)
2. [Pipes, socketpairs, And Unix Sockets](pipes-socketpairs-and-unix-sockets.md)
3. [Shared Memory And Zero-Copy IPC](shared-memory-and-zero-copy-ipc.md)
4. [eventfd, timerfd, signalfd, And inotify](eventfd-timerfd-signalfd-and-inotify.md)
5. [IPC Protocols And Versioning](ipc-protocols-and-versioning.md)
6. [Credentials, Authentication, And Peer Lifecycle](credentials-authentication-and-peer-lifecycle.md)
7. [Event Loops: select, poll, And epoll](event-loops-select-poll-and-epoll.md)

## IPC decision matrix

| Requirement | Good starting point | Main obligation |
| --- | --- | --- |
| Related processes, bounded byte stream | Pipe or `socketpair` | Close ends and define framing/EOF |
| Local bidirectional API | Unix stream socket | Authenticate peer and handle reconnect |
| Local datagrams/events | Unix datagram socket | Bound messages, queue depth, and loss policy |
| Large high-rate payload | Shared memory + socket/eventfd | Synchronize ownership and lifetime |
| Wakeup or count | `eventfd` | Drain counts and define overflow/coalescing |
| Timed event | `timerfd` | Consume expiration counts and missed-period policy |
| Signals in loop | `signalfd` | Block signals consistently before worker creation |
| File-change hint | `inotify` | Rescan authoritative state; events can coalesce or overflow |

## Protocol obligations

Every IPC endpoint needs:

- framing and maximum message size;
- encoding, byte order, alignment, and version policy;
- request IDs, replies, events, cancellation, and duplicate handling;
- peer identity and authorization;
- queue bounds and overload behavior;
- timeout, reconnect, and peer-death semantics;
- descriptor and buffer ownership;
- observability with operation IDs and state;
- shutdown and upgrade compatibility.

## Event-loop model

```text
wait for readiness
  -> read/drain bounded input
  -> validate and enqueue bounded work
  -> write bounded output
  -> process timers/signals/wakeups
  -> return to wait
```

Readiness is not completion. Each handler must be short enough to preserve fairness,
must handle `EAGAIN`, and must leave incomplete messages in owned buffers.

## Stage lab

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-ipc-event-loop.c -o /tmp/ipc-event-loop
/tmp/ipc-event-loop
```

The probe uses a local socket and an eventfd-style wakeup path. Extend it by adding a
slow consumer, malformed frame, peer close, and bounded output queue.

## Completion criteria

You can complete this stage when you can:

- choose an IPC mechanism from size, rate, ownership, identity, failure, and target
  constraints;
- construct framed Unix-socket and pipe protocols with bounded buffers;
- share large data without sharing unsafe pointers or ambiguous ownership;
- integrate eventfd/timerfd/signalfd/inotify into one loop;
- version, authenticate, cancel, retry, and reconnect IPC requests safely;
- implement level/edge-triggered readiness loops with fairness and shutdown.

## Related topics

- [Stage 6: Threads And Userspace Concurrency](../threads-and-userspace-concurrency/index.md)
- [Stage 8: Terminals, TTYs, And Serial Userspace](../terminals-ttys-and-serial-userspace/index.md)
- [Stage 9: Userspace Networking](../userspace-networking/index.md)
- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](../hardware-facing-userspace-and-kernel-uapi/index.md)

## References

- [`unix(7)`](https://man7.org/linux/man-pages/man7/unix.7.html)
- [`epoll(7)`](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [`eventfd(2)`](https://man7.org/linux/man-pages/man2/eventfd.2.html)
- [Linux kernel userspace API](https://www.kernel.org/doc/html/latest/userspace-api/index.html)
