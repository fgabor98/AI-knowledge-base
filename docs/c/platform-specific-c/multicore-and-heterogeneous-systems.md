---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Multicore And Heterogeneous Systems

Adding a second execution agent changes the design even when both agents run C. The
agents may be symmetric CPU cores, different cores in one SoC, a DSP, a GPU, a DMA
engine, or an external controller. They may share caches and an address space, share
only physical memory, or communicate through mailboxes and interrupts. Correctness
depends on ownership, visibility, ordering, lifetime, and failure recovery—not merely
on using a `volatile` qualifier.

## Learning Objectives

- Distinguish SMP, AMP, heterogeneous multiprocessing, and peripheral/DMA concurrency.
- Design shared-memory protocols with explicit ownership and C11 memory ordering.
- Account for cache coherence, non-coherent buffers, false sharing, and barriers.
- Choose between locks, atomics, IPIs, mailboxes, shared rings, RPMsg-like IPC, and
  remote-processor services.
- Define startup, reset, crash, and shared-peripheral ownership for multiple agents.

## System Models

| Model | Description | Main C concern |
| --- | --- | --- |
| SMP | Cores run one OS image and share a coherent address space | Locks, atomics, affinity, cache contention, preemption |
| AMP | Cores run separate images or operating systems | Shared-memory protocol, ownership, reset, address mapping |
| Heterogeneous | Different cores/accelerators have different ISAs or memory views | Serialization, translation, capability differences, timing |
| DMA/peripheral concurrency | A bus master reads/writes memory without executing C | Buffer ownership, cache maintenance, descriptor protocol |
| Distributed board | Agents communicate over a link or separate memory | Framing, transport failure, versioning, timeouts |

Start with an ownership diagram. For each buffer and register, identify the current
owner, the operation that transfers ownership, the visibility mechanism, and the
behavior when the owner crashes or resets.

## Shared Memory Is A Protocol

An address that two agents can read is not automatically a useful shared-memory
interface. Define:

- object layout, alignment, endianness, and integer widths;
- cacheability and coherency of the region;
- producer/consumer ownership and valid states;
- publication and observation ordering;
- interrupt or polling notification;
- timeout, retry, generation, and reset behavior;
- compatibility/versioning for independently deployed images.

Avoid sharing pointers across different address spaces or processors unless the
platform explicitly defines a shared virtual-address contract. Use offsets, handles,
physical/I/O addresses, or descriptors with fixed-width fields instead. Validate every
offset against the shared region before forming a local pointer.

## C11 Atomics For Ordinary Shared Memory

For ordinary memory visible to C threads, C11 atomics express the synchronization
relationship directly:

```c
#include <stdatomic.h>
#include <stdbool.h>

struct mailbox {
    int payload;
    atomic_bool ready;
};

static void publish(struct mailbox *box, int value)
{
    box->payload = value;
    atomic_store_explicit(&box->ready, true, memory_order_release);
}

static bool try_consume(struct mailbox *box, int *value)
{
    if (!atomic_load_explicit(&box->ready, memory_order_acquire)) {
        return false;
    }
    *value = box->payload;
    return true;
}
```

The release store prevents the payload write from being published after `ready`; the
acquire load prevents the consumer from reading the payload before observing the
publication. This is only a one-shot example: a reusable mailbox needs an acknowledge
or sequence state, and multiple producers/consumers need a complete queue algorithm.
If `box` is shared with a DMA engine or a non-C processor, use the platform's cache,
barrier, and layout contract in addition to—or instead of—C atomics.

Use the weakest ordering that expresses the protocol and no weaker. Relaxed atomics
can count events or protect a value when another synchronization mechanism carries
the payload visibility; they are not a general replacement for acquire/release.

## Cache Coherence And False Sharing

Coherence means that compatible caches maintain a consistent view of a cache line under
the architecture's rules. It does not guarantee low latency, fair progress, or
coherence with every DMA master. Non-coherent systems require explicit clean/invalidate
operations and ownership transitions.

False sharing occurs when independent hot variables occupy one cache line and different
cores repeatedly write them. The program can be logically correct but unexpectedly
slow. Use per-core data, padding/alignment where measured, batching, and read-mostly
layouts. Do not add arbitrary padding to an externally defined shared structure without
also defining its ABI and cache-line assumptions.

For DMA, distinguish:

- CPU writes followed by device reads: clean/publish before handing off;
- device writes followed by CPU reads: invalidate/acquire before consuming;
- bidirectional ownership: perform the required transition on both directions;
- descriptor rings: synchronize descriptors and payloads in the documented order.

## Locks, Atomics, And Progress

Choose synchronization by critical-section size, context, contention, and progress
requirements:

- mutexes permit blocking and are suitable for process/task contexts;
- spinlocks avoid sleeping but burn CPU and require strict context rules;
- atomics suit small state transitions and lock-free structures when their algorithm
  has been proven;
- seqlocks favor readers but require retry logic and suitable writer constraints;
- RCU-like techniques separate publication from reclamation and need lifetime rules;
- interrupt masking only protects against specified local preemption and is not an SMP
  lock unless the platform explicitly says so.

Document lock order to prevent deadlock. Keep lock scope small, never call unknown
callbacks while holding a low-level lock, and define whether an error path can release
resources in a different order. For lock-free algorithms, prove ABA handling, memory
reclamation, wraparound, and progress under contention—not just the atomic operations.

## IPIs, Mailboxes, And Doorbells

An inter-processor interrupt (IPI), mailbox, or doorbell is a notification, not usually
the payload itself. A safe sequence is:

1. Write the message or descriptors.
2. Publish them with the required release/barrier/cache operation.
3. Ring the doorbell or raise the IPI.
4. Receiver acknowledges the notification, acquires/invalidates, and validates the
   message before consuming.
5. Receiver returns completion or ownership, then the sender reuses the storage only
   after observing that transition.

Interrupt coalescing means multiple messages may correspond to one notification.
Handlers should drain or schedule work based on queue state rather than assuming one
interrupt equals one item. A lost notification must be recoverable by polling state;
an interrupt alone is not a durable queue.

## SMP And AMP Startup

For SMP, the boot core brings up secondary cores, initializes per-CPU data, installs
interrupt routing, and releases the scheduler according to the OS contract. For AMP,
each image needs a distinct stack, vector/trap entry, memory ownership, and reset
protocol. Shared regions must be excluded from destructive initialization by every
image.

Define what happens when one core resets:

- Does the other core observe a generation change and invalidate outstanding handles?
- Who resets shared peripherals and mailboxes?
- Can stale cache lines or descriptors survive the reset?
- How are in-flight DMA transactions stopped or quarantined?
- Is the restarted image allowed to reclaim the shared region immediately?

Use generation counters and explicit state machines instead of a boolean “ready” flag
that cannot distinguish an old owner from a restarted one.

## Remote Processors And RPMsg-Style IPC

Remote-processor frameworks commonly combine a firmware loader, reserved memory,
virtio-like rings, mailbox notifications, and a message protocol. The names vary by
platform, but the design questions are stable:

- who allocates and maps each buffer;
- whether addresses are local, physical, IOVA, or offsets;
- how cache maintenance is performed;
- how endpoints are created, versioned, and destroyed;
- how backpressure and malformed messages are handled;
- how crash/restart is signaled and stale messages are rejected.

Treat the IPC format as an ABI. Use fixed-width fields, explicit alignment, bounded
lengths, and a version/capability handshake. Never trust a remote processor merely
because it runs firmware from the same vendor; its memory writes are an untrusted
input at the C boundary.

## Shared Peripheral Ownership

A peripheral should have one authoritative owner unless the hardware explicitly
supports partitioning. If two agents configure the same UART, clock, DMA channel, or
reset line, each can invalidate the other's assumptions. Ownership can be:

- static, assigned by boot configuration;
- delegated through a broker/firmware service;
- arbitrated by a lock and a versioned register protocol;
- partitioned into independent channels with documented resource boundaries.

The owner must define initialization, runtime access, power transitions, and recovery.
Sharing a data register without sharing the peripheral's control state is rarely safe.

## Exercises And Diagnostics

1. Design a bounded single-producer/single-consumer ring with sequence numbers and
   write down its memory-ordering proof.
2. Run a false-sharing benchmark with per-core counters, then separate the counters and
   measure the change under realistic affinity and frequency settings.
3. Build a shared-memory message format that survives one endpoint reset; test stale
   generation, truncated message, and queue-full cases.
4. Trace a DMA descriptor from allocation through CPU preparation, device ownership,
   completion interrupt, cache maintenance, and reuse.
5. Draw a shared-peripheral ownership state machine for normal boot, suspend, crash,
   and recovery.

## Common Mistakes

- Treating a shared address, `volatile`, or a cache-coherent CPU as the whole protocol.
- Publishing a ready flag before payload and descriptors are visible.
- Reusing a DMA buffer before the device has returned ownership.
- Sharing native pointers, enums, or `long` fields across different ABIs.
- Assuming one interrupt corresponds exactly to one message.
- Using local interrupt masking as an SMP synchronization mechanism.
- Omitting reset generations, stale-message rejection, or crash recovery.
- Letting multiple agents configure a shared peripheral without an owner.

## Related Topics

- [Platform-Specific C overview](./index.md)
- [ARM Cortex-A And AArch64](./arm-cortex-a-and-aarch64.md)
- [RISC-V](./risc-v.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)

## References

- [C atomics overview](../standard-library-and-ecosystem/atomics-threads-and-signals.md)
- [Linux memory barriers](https://docs.kernel.org/core-api/wrappers/memory-barriers.html)
- [Linux remote processor framework](https://docs.kernel.org/staging/remoteproc.html)
- [Linux RPMsg framework](https://docs.kernel.org/staging/rpmsg.html)
- [Arm architecture and memory model documentation](https://developer.arm.com/Architectures)
- The exact SoC coherency, interconnect, DMA, interrupt, remote-processor, and power
  management manuals
