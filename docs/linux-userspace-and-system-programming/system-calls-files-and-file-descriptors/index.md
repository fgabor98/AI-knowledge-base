---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 3: System Calls, Files, And File Descriptors

Build the reliable I/O foundation needed by hardware clients, services, diagnostic tools, and event loops.

This stage is a collection of focused draft pages. Read the overview first, then study the leaf pages in order while extending one small C utility or service.

## Learning Materials

1. [System-Call Contracts And Errors](system-call-contracts-and-errors.md)
2. [File Descriptors And Open-File Descriptions](file-descriptors-and-open-file-descriptions.md)
3. [Descriptor Inheritance And Redirection](descriptor-inheritance-and-redirection.md)
4. [Regular-File I/O And Metadata](regular-file-io-and-metadata.md)
5. [Durability, Locking, And Power Loss](durability-locking-and-power-loss.md)
6. [Pipes, FIFOs, And Backpressure](pipes-fifos-and-backpressure.md)
7. [Blocking, Nonblocking, And Partial I/O](blocking-nonblocking-and-partial-io.md)

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

- explain and demonstrate system-call contracts and errors;
- explain and demonstrate file descriptors and open-file descriptions;
- explain and demonstrate descriptor inheritance and redirection;
- explain and demonstrate regular-file i/o and metadata;
- explain and demonstrate durability, locking, and power loss;
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
