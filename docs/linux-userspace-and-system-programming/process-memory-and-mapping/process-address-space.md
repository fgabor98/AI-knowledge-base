---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Process Address Space

## What problem does this solve?

Virtual memory makes each process appear to have a private, contiguous address space,
but the visible layout is assembled from mappings with different backing, protection,
sharing, and lifetime. Confusing virtual size with physical use, or assuming a pointer
is globally meaningful, causes crashes and misleading memory diagnoses.

## Mapping categories

| Region | Contents | Typical properties |
| --- | --- | --- |
| Text | Instructions, including PLT stubs where used | Read/execute, often file-backed and shareable |
| Read-only data | Constants, strings, relocation data | Read-only, sometimes non-executable |
| Writable data/BSS | Globals and zero-initialized state | Private initially, copy-on-write after `fork` |
| Heap | Allocator-managed dynamic storage | Writable, allocator metadata and fragmentation |
| Main stack | Automatic objects and call frames | Writable, bounded by `RLIMIT_STACK`, often guard-protected |
| Thread stacks/TLS | Per-thread stack and thread-local variables | Per-thread lifetime and limits |
| Shared libraries | libc and other DSOs | File-backed, shared clean pages and private relocations |
| Anonymous mappings | Allocator arenas, `mmap`, stacks | No ordinary file backing; can be private or shared |
| File mappings | Mapped file contents | Shared/private write semantics and page faults |
| VDSO/VVAR | Kernel-provided user mappings | Architecture/kernel implementation detail |

The GOT contains address data rather than executable instructions; relocations may
write it during loading, and RELRO can protect applicable regions afterward.

ASLR, PIE, loader choices, compiler options, and environment alter addresses. Code
must use symbols and valid pointers, never address literals inferred from one run.

## Pages and page faults

The kernel manages memory in pages. A virtual page can be unmapped, mapped but not yet
resident, read-only, copy-on-write, swapped, or backed by a file. Accessing a valid
but nonresident page can trigger a page fault and block while storage or reclaim work
completes. Accessing an unmapped or forbidden page delivers `SIGSEGV`/`SIGBUS`.

The first touch of an allocation is therefore not necessarily free or deterministic.
Real-time or latency-sensitive code may need bounded allocation, pre-faulting, locked
memory, and a tested memory budget—but each adds resource and privilege requirements.

## `fork` and copy-on-write

After `fork`, private pages are shared read-only until either process writes. The
writer receives a private copy. Large resident heaps can therefore make a fork cheap
at first and expensive at an unpredictable later point. A child that calls `exec`
soon can benefit; a child that modifies the whole heap may duplicate it.

Mappings shared with `MAP_SHARED` have different semantics and require an explicit
cross-process synchronization protocol. Copy-on-write is not synchronization.

## Inspecting memory

```sh
pid=1234
cat /proc/$pid/maps
cat /proc/$pid/smaps_rollup 2>/dev/null || cat /proc/$pid/smaps
pmap -x "$pid" 2>/dev/null || true
cat /proc/$pid/limits | grep -E 'Max stack|Max address|Max locked'
```

`VmSize` is virtual address space; `VmRSS` is resident memory; proportional set size
(PSS) accounts shared pages more fairly. The numbers are snapshots and can change
during collection.

## Stack lifetime and safety

Automatic objects cease to exist when their scope ends; returning a pointer to a local
object is a lifetime error. A thread’s stack ceases to be valid after the thread
terminates and is joined/detached according to its contract. Deep recursion, large
automatic arrays, and unbounded call paths can exhaust a stack even when heap memory
is available.

Use explicit size bounds, guard pages, and stack attributes for worker threads. Do not
assume the main-thread stack size applies to every pthread.

## Physical addresses are not pointers

Userspace virtual addresses are translated by the MMU. A physical register or DMA
buffer address cannot be dereferenced as an ordinary pointer. Mapping device memory
requires a documented driver UAPI that establishes permissions, cache behavior,
lifetimes, and synchronization. `/dev/mem` is not a general substitute for a driver.

## Common mistakes

- Treating a virtual address as a physical address or stable interprocess handle.
- Assuming `malloc` commits all physical memory immediately.
- Treating `VmSize` as actual RAM consumption.
- Assuming a page fault is always a fatal bug.
- Returning pointers to stack, unmapped, or freed storage.
- Assuming `fork` produces independent file-backed or shared mappings.
- Mapping device memory without a driver-defined UAPI and cache protocol.

## Debugging checklist

- Capture `/proc/<pid>/maps`, `status`, `limits`, and thread count.
- Compare virtual, RSS, PSS, dirty, anonymous, and file-backed memory.
- Check stack limits, guard pages, recursion, and large automatic objects.
- Correlate faults with `SIGSEGV`/`SIGBUS`, core dumps, and fault addresses.
- Identify mapping ownership and whether another thread can unmap it.
- Check `fork`/COW and shared-memory behavior separately.

## Related topics

- [Stage 4: Process Memory And Mapping](index.md)
- [mmap, Files, And Shared Memory](mmap-files-and-shared-memory.md)
- [Memory Pressure, OOM, And Real-Time Constraints](memory-pressure-oom-and-realtime.md)
- [C Memory Safety And Lifetime](../../c/semantics-and-memory/memory-safety-and-lifetime.md)

## References

- [`proc_pid_maps(5)`](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html)
- [`proc_pid_status(5)`](https://man7.org/linux/man-pages/man5/proc_pid_status.5.html)
- [`getrlimit(2)`](https://man7.org/linux/man-pages/man2/getrlimit.2.html)
- [`fork(2)`](https://man7.org/linux/man-pages/man2/fork.2.html)
