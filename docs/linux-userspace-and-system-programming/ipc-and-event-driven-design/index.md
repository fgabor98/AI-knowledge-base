---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 7: IPC And Event-Driven Design

Choose and implement bounded local communication and readiness-driven control flow.

This stage is a collection of focused draft pages. Read the overview first, then study the leaf pages in order while extending one small C utility or service.

## Learning Materials

1. [IPC Selection And Failure Models](ipc-selection-and-failure-models.md)
2. [Pipes, socketpairs, And Unix Sockets](pipes-socketpairs-and-unix-sockets.md)
3. [Shared Memory And Zero-Copy IPC](shared-memory-and-zero-copy-ipc.md)
4. [eventfd, timerfd, signalfd, And inotify](eventfd-timerfd-signalfd-and-inotify.md)
5. [IPC Protocols And Versioning](ipc-protocols-and-versioning.md)
6. [Credentials, Authentication, And Peer Lifecycle](credentials-authentication-and-peer-lifecycle.md)
7. [Event Loops: select, poll, And epoll](event-loops-select-poll-and-epoll.md)

## Study Pattern

For each page:

1. Read the contract and identify the libc, POSIX, Linux, kernel UAPI, or init-system layer.
2. Implement the smallest host-side example.
3. Add error, timeout, ownership, and cleanup paths.
4. Observe the result with the relevant Linux tools.
5. Repeat on the target and record differences.
6. Integrate the mechanism into the running capstone service.

## Stage Outcomes

By the end of this stage, you should be able to:

- explain and demonstrate ipc selection and failure models;
- explain and demonstrate pipes, socketpairs, and unix sockets;
- explain and demonstrate shared memory and zero-copy ipc;
- explain and demonstrate eventfd, timerfd, signalfd, and inotify;
- explain and demonstrate ipc protocols and versioning;
- connect the mechanism to an embedded Linux failure, test, or service-design decision;
- produce evidence that distinguishes application, kernel, deployment, and hardware causes.

## Completion Criteria

- The examples compile with warnings and debug information.
- Normal, interrupted, missing-resource, and teardown paths are tested.
- Resource ownership and target assumptions are documented.
- At least one failure has been diagnosed using observable evidence.
- The work is linked to the next stage or an existing capstone.

## Related Topics

- [Linux Userspace And System Programming](../index.md)
- [C Programming](../../c/index.md)
- [Linux Kernel Programming](../../linux-kernel/index.md)
- [Embedded Linux](../../embedded-linux/index.md)
