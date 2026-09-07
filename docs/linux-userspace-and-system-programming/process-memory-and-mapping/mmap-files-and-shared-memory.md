---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# mmap, Files, And Shared Memory

## What problem does this solve?

`mmap` can expose file bytes or anonymous pages as memory and can share storage
between processes. It also introduces page faults, offset/alignment constraints,
coherence questions, SIGBUS hazards, and explicit unmapping/lifetime obligations.

## Mapping a file

```c
int fd = open(path, O_RDONLY | O_CLOEXEC);
struct stat st;
if (fd == -1 || fstat(fd, &st) == -1 || st.st_size == 0) {
    /* handle error and close fd */
}

void *view = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
if (view == MAP_FAILED) {
    /* inspect errno */
}
/* use only the mapped range; munmap(view, length) when finished */
```

The offset must meet the page-alignment requirement. The mapping length can be
rounded internally, but the application may access only the requested valid range.
Do not close the FD too early in a design that needs the FD for identity or lifetime
bookkeeping; on Linux the mapping can remain after close, but its backing and
ownership policy still need to be clear.

`MAP_PRIVATE` writes use copy-on-write and do not update the file. `MAP_SHARED` writes
can update the underlying file and require synchronization/durability policy. Mapping
a file larger than its current size and then truncating it can cause SIGBUS on access
to pages beyond the new end.

## Anonymous and shared memory

For related processes, an anonymous `MAP_SHARED` mapping inherited across `fork` can
hold shared state. For unrelated processes, use a named shared-memory object (`shm_open`),
an ordinary file, or a Linux `memfd_create` object, then map it into both processes.

The mapping supplies bytes, not synchronization. Use process-shared pthread objects,
POSIX semaphores, atomics with a proven protocol, eventfds, or another explicit
mechanism. Define initialization, version, size, producer/consumer ownership,
shutdown, and peer-death behavior.

## Lifetime and resizing

`munmap` invalidates every pointer into the unmapped range. A concurrent user must stop
before unmapping; reference counting alone is insufficient if a thread can acquire a
new pointer without synchronization. Resizing a shared mapping requires a generation
or replacement protocol, not an in-place `ftruncate` while readers access it.

For a shared-memory header, include:

```text
magic and version
mapping size / layout size
generation or sequence
initialization state
alignment and endian policy
producer/consumer ownership
```

Validate all values before deriving pointers. Never place raw process-private pointers
in shared memory; address-space layouts differ between processes.

## Coherence and persistence

`MAP_SHARED` provides a sharing mechanism, not an automatic message protocol or power-
loss commit. Use synchronization for visibility and `msync`/file synchronization only
according to the filesystem durability requirement. A mapped device region has
additional cache and ordering rules defined by its driver; ordinary atomics do not
automatically make DMA coherent.

## Anonymous allocation alternatives

`malloc` is usually the right choice for ordinary private dynamic storage. `mmap`
is useful for large allocations, explicit protection, shared backing, file windows,
guard pages, or lifetime isolation. `shm_open` and `memfd_create` are useful for
cross-process or descriptor-transfer designs. Choose based on ownership and protocol,
not because “zero-copy” sounds faster.

## Common mistakes

- Using `MAP_PRIVATE` while expecting file updates.
- Accessing beyond the file’s current size and receiving SIGBUS.
- Sharing pointers instead of offsets in shared memory.
- Unmapping while another thread still uses a pointer.
- Assuming `mmap` removes synchronization or copy costs everywhere.
- Using `msync` as a substitute for a complete durable transaction.
- Resizing a live shared object without a generation protocol.

## Debugging checklist

- Record FD, offset, length, protection, flags, and mapping owner.
- Inspect `/proc/<pid>/maps` and `smaps` for the actual region.
- Test zero-length, truncated, permission-denied, and unmap paths.
- Test peer crash, stale initialization, version mismatch, and full shared storage.
- Check synchronization, cache/DMA boundary, and durability assumptions separately.
- Use sanitizers and guard pages to catch lifetime and bounds errors.

## Related topics

- [Stage 4: Process Memory And Mapping](index.md)
- [Process Address Space](process-address-space.md)
- [Memory Protection And Process Hardening](memory-protection-and-hardening.md)
- [Shared Memory And Zero-Copy IPC](../ipc-and-event-driven-design/shared-memory-and-zero-copy-ipc.md)

## References

- [`mmap(2)`](https://man7.org/linux/man-pages/man2/mmap.2.html)
- [`munmap(2)`](https://man7.org/linux/man-pages/man2/munmap.2.html)
- [`shm_open(3)`](https://man7.org/linux/man-pages/man3/shm_open.3.html)
- [`memfd_create(2)`](https://man7.org/linux/man-pages/man2/memfd_create.2.html)
- [`msync(2)`](https://man7.org/linux/man-pages/man2/msync.2.html)
