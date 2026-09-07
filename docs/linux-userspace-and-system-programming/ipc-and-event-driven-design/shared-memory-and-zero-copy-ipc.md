---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Shared Memory And Zero-Copy IPC

## What problem does this solve?

Large or high-rate payloads can make repeated copies expensive. Shared memory can
reduce copying, but it replaces transport copying with explicit synchronization,
buffer ownership, versioning, peer-death recovery, and cache/coherence obligations.

## Separate data and control

Use shared memory for bulk bytes and a socket, eventfd, or semaphore for control:

```text
producer writes slot -> release/publication -> event notification
consumer waits       -> acquire/ownership -> reads slot
consumer returns slot or advances sequence
```

The shared region does not wake a process or authenticate a peer by itself. Define
slot count, states, sequence numbers, maximum payload, and behavior when the consumer
falls behind.

## Representation

Use offsets, lengths, fixed-width integers, and explicit padding—not raw pointers.
Include magic, version, total layout size, endianness, alignment, and generation.
Validate every offset/length against the mapped region before deriving a pointer.

```text
header: magic/version/size/generation
slots:  state | sequence | length | payload | checksum
```

The producer must not overwrite a slot until the consumer has returned ownership.
Single-producer/single-consumer rings can be efficient, but only when the index
ownership and memory-order proof is explicit. Multi-producer or multi-consumer
designs need a stronger algorithm or a lock.

## Backing choices

`shm_open` creates a POSIX shared-memory object; an ordinary file can supply a mapped
region; `memfd_create` creates an anonymous Linux file descriptor suitable for mapping
and FD passing. Set size with `ftruncate` before mapping, and define cleanup of names
or descriptors after peer death.

## Lifetime and recovery

A mapping can outlive a name or FD, and a process can die while holding a slot. Use
generation/lease state, robust synchronization, or restart-time initialization rules
to detect abandoned ownership. Never wait forever for a dead peer’s slot.

If the backing file is truncated while mapped, access beyond the new size can SIGBUS.
Resize by creating a new generation and switching ownership, not by shrinking a live
region under readers.

## Zero-copy is conditional

The data may still be copied by the kernel, device, network stack, or cache path. A
zero-copy design can increase memory footprint, cache misses, synchronization cost,
and latency variance. Measure end-to-end behavior and retain a bounded-copy fallback.

Device/DMA sharing has additional cache, ownership, and memory-barrier rules. Ordinary
userspace atomics do not prove visibility to hardware.

## Common mistakes

- Sharing raw process pointers.
- Publishing a slot before all payload fields are written.
- Reusing a slot while a consumer still reads it.
- Treating shared memory as a durable database.
- Ignoring stale generation and peer-death recovery.
- Resizing/truncating a mapped object in place.
- Assuming zero-copy removes all copies or synchronization.

## Debugging checklist

- Record backing FD/name, mapping address/length, version, generation, and owner.
- Inspect maps, FD lifetime, queue indices, states, and sequence gaps.
- Test peer crash at each ownership transition and restart with stale data.
- Test malformed headers, oversized lengths, truncated backing, and full rings.
- Use race detection and an event/control trace alongside payload checksums.
- Test hardware/DMA cache behavior separately from process-to-process visibility.

## Related topics

- [Stage 7: IPC And Event-Driven Design](index.md)
- [IPC Selection And Failure Models](ipc-selection-and-failure-models.md)
- [mmap, Files, And Shared Memory](../process-memory-and-mapping/mmap-files-and-shared-memory.md)
- [Atomics, Memory Ordering, And Reentrancy](../threads-and-userspace-concurrency/atomics-memory-ordering-and-reentrancy.md)

## References

- [`shm_open(3)`](https://man7.org/linux/man-pages/man3/shm_open.3.html)
- [`memfd_create(2)`](https://man7.org/linux/man-pages/man2/memfd_create.2.html)
- [`mmap(2)`](https://man7.org/linux/man-pages/man2/mmap.2.html)
- [`futex(2)`](https://man7.org/linux/man-pages/man2/futex.2.html)
