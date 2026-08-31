---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 5: Time, Clocks, And Signals

Use time and asynchronous events correctly in systems where deadlines, suspend, shutdown, and recovery matter.

This stage is a collection of focused draft pages. Read the overview first, then study the leaf pages in order while extending one small C utility or service.

## Learning Materials

1. [Clocks, Time Bases, And Deadlines](clocks-time-bases-and-deadlines.md)
2. [Timers And Periodic Work](timers-and-periodic-work.md)
3. [Signal Model And sigaction](signal-model-and-sigaction.md)
4. [Signal-Safe Shutdown And Event Integration](signal-safe-shutdown-and-event-integration.md)

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

- explain and demonstrate clocks, time bases, and deadlines;
- explain and demonstrate timers and periodic work;
- explain and demonstrate signal model and sigaction;
- explain and demonstrate signal-safe shutdown and event integration;
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
