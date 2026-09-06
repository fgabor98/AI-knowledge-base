---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Stage 4: Process Memory And Mapping

This stage explains how Linux gives each process a virtual address space and how
programs use mappings, protection, sharing, and allocation without confusing virtual
addresses with physical memory. The central rule is simple: a pointer is meaningful
only inside its process and lifetime; a mapping is a resource with permissions,
backing, ownership, and failure behavior.

## The address-space model

```text
high addresses
  shared libraries / dynamic loader / mmap regions
  thread stacks and guard pages
  main stack
  -----------------------------
  heap (brk and anonymous mappings)
  BSS / writable data
  read-only data / text / PIE
low addresses
```

The exact layout is architecture-, kernel-, loader-, ASLR-, and workload-dependent.
Do not encode assumptions about addresses. `/proc/<pid>/maps` is an observation of
one process, not an ABI for another.

## Learning materials

1. [Process Address Space](process-address-space.md)
2. [mmap, Files, And Shared Memory](mmap-files-and-shared-memory.md)
3. [Memory Protection And Process Hardening](memory-protection-and-hardening.md)
4. [Memory Pressure, OOM, And Real-Time Constraints](memory-pressure-oom-and-realtime.md)

## Memory is several contracts

| Question | Contract to identify |
| --- | --- |
| Is the address valid? | Mapping exists, range is within the object, and lifetime is active |
| May code read/write/execute? | Page protection and file permissions |
| Who backs it? | ELF file, heap allocator, anonymous pages, shared memory, or device UAPI |
| Who owns the bytes? | One thread/process, a shared protocol, kernel, DMA-capable subsystem |
| When is it resident? | Allocation, page fault, reclaim, locking, or pre-touch policy |
| What happens under pressure? | Allocation failure, reclaim, cgroup limit, or OOM kill |
| Does it survive a process? | Private mapping, shared backing object, or persistent file |

## Observation lab

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-memory-map.c -o /tmp/memory-map
/tmp/memory-map
cat /proc/$$/maps | sed -n '1,20p'
```

Inspect the probe while it runs with `/proc/<pid>/maps`, `smaps_rollup`, and
`pmap` if available. Compare a debug and release build; addresses and mappings may
change because of PIE, optimization, and loader behavior.

## Completion criteria

You can complete this stage when you can:

- describe mappings without assuming a fixed address layout;
- distinguish virtual size, resident memory, shared pages, and private dirty pages;
- use `mmap`/`munmap` with correct length, offset, alignment, and ownership rules;
- build a shared-memory protocol with explicit synchronization and lifetime;
- explain `mprotect`, guard pages, ASLR, PIE, NX, RELRO, and stack limits;
- diagnose allocation failure, page faults, memory pressure, and OOM policy;
- explain why userspace must not map physical addresses without a documented UAPI.

## Related topics

- [Stage 3: System Calls, Files, And File Descriptors](../system-calls-files-and-file-descriptors/index.md)
- [C Memory Safety And Lifetime](../../c/semantics-and-memory/memory-safety-and-lifetime.md)
- [C Memory Model And Concurrency](../../c/advanced-c/c-memory-model-and-concurrency.md)
- [Memory, DMA, And Cache Boundaries](../../c/embedded-c-and-hardware/dma-cache-and-memory-barriers.md)

## References

- [`mmap(2)`](https://man7.org/linux/man-pages/man2/mmap.2.html)
- [`proc_pid_maps(5)`](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html)
- [Linux kernel memory management documentation](https://www.kernel.org/doc/html/latest/admin-guide/mm/index.html)
