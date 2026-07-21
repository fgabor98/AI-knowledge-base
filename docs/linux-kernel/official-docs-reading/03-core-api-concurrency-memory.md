---
status: active
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# 3. Core APIs, Concurrency, Memory, And DMA

Official sections: [Core API](https://docs.kernel.org/core-api/index.html) and
[Locking](https://docs.kernel.org/locking/index.html)

## Core Utilities And Driver Execution

- [ ] **P0** [Driver basics](https://docs.kernel.org/driver-api/basics.html)
- [ ] **P0** [Workqueues](https://docs.kernel.org/core-api/workqueue.html)
- [ ] **P0** [Completions](https://docs.kernel.org/scheduler/completion.html)
- [ ] **P0** [Delay and sleep mechanisms](https://docs.kernel.org/timers/delay_sleep_functions.html)
- [ ] **P0** [Timer and delay APIs](https://docs.kernel.org/driver-api/basics.html#delaying-deferred-and-asynchronous-work)
- [ ] **P0** [Message logging with printk](https://docs.kernel.org/core-api/printk-basics.html)
- [ ] **P0** [Printk format specifiers](https://docs.kernel.org/core-api/printk-formats.html)
- [ ] **P1** [Real-time preemption](https://docs.kernel.org/core-api/real-time/index.html)
- [ ] **P1** [ktime accessors](https://docs.kernel.org/core-api/timekeeping.html)

## Objects, Lifetime, And Data Structures

- [ ] **P0** [Kobjects, ksets, and ktypes](https://docs.kernel.org/core-api/kobject.html)
- [ ] **P0** [Adding krefs to kernel objects](https://docs.kernel.org/core-api/kref.html)
- [ ] **P0** [Linked lists](https://docs.kernel.org/core-api/list.html)
- [ ] **P0** [Circular buffers](https://docs.kernel.org/core-api/circular-buffers.html)
- [ ] **P1** [XArray](https://docs.kernel.org/core-api/xarray.html)
- [ ] **P1** [ID allocation](https://docs.kernel.org/core-api/idr.html)
- [ ] **P1** [Red-black trees](https://docs.kernel.org/core-api/rbtree.html)
- [ ] **P1** [Bitfield packing and unpacking](https://docs.kernel.org/core-api/packing.html)
- [ ] **P1** [Scope-based cleanup helpers](https://docs.kernel.org/core-api/cleanup.html)

## Locking And Concurrency

- [ ] **P0** [Unreliable guide to kernel locking](https://docs.kernel.org/kernel-hacking/locking.html)
- [ ] **P0** [Locking lessons](https://docs.kernel.org/locking/locktypes.html)
- [ ] **P0** [Spinlock lesson](https://docs.kernel.org/locking/spinlocks.html)
- [ ] **P0** [Mutex design](https://docs.kernel.org/locking/mutex-design.html)
- [ ] **P0** [Atomic types](https://docs.kernel.org/core-api/wrappers/atomic_t.html)
- [ ] **P0** [`refcount_t` versus `atomic_t`](https://docs.kernel.org/core-api/refcount-vs-atomic.html)
- [ ] **P0** [Linux kernel memory barriers](https://docs.kernel.org/core-api/wrappers/memory-barriers.html)
- [ ] **P0** [Generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [ ] **P1** [RCU handbook](https://docs.kernel.org/RCU/index.html)
- [ ] **P1** [Lockdep design](https://docs.kernel.org/locking/lockdep-design.html)
- [ ] **P1** [Locking on PREEMPT_RT](https://docs.kernel.org/locking/locktypes.html#lock-categories)

For each primitive, record process/IRQ context legality, sleepability, ownership,
ordering guarantees, interrupt state, and teardown requirements.

## Memory Allocation And Userspace Access

- [ ] **P0** [Memory allocation guide](https://docs.kernel.org/core-api/memory-allocation.html)
- [ ] **P0** [Memory management APIs](https://docs.kernel.org/core-api/mm-api.html)
- [ ] **P0** [Unaligned memory accesses](https://docs.kernel.org/core-api/unaligned-memory-access.html)
- [ ] **P0** [User space memory access](https://docs.kernel.org/core-api/mm-api.html#user-space-memory-access)
- [ ] **P1** [`pin_user_pages()` and related calls](https://docs.kernel.org/core-api/pin_user_pages.html)
- [ ] **P1** [Kernel virtual memory layout for the target architecture](https://docs.kernel.org/arch/arm64/memory.html)

## MMIO, DMA Mapping, And DMAEngine

- [ ] **P0** [Bus-independent device accesses](https://docs.kernel.org/driver-api/device-io.html)
- [ ] **P0** [Ordering I/O writes to MMIO](https://docs.kernel.org/driver-api/io_ordering.html)
- [ ] **P0** [DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [ ] **P0** [DMA API](https://docs.kernel.org/core-api/dma-api.html)
- [ ] **P0** [DMA attributes](https://docs.kernel.org/core-api/dma-attributes.html)
- [ ] **P0** [DMAEngine client documentation](https://docs.kernel.org/driver-api/dmaengine/client.html)
- [ ] **P0** [DMAEngine provider documentation](https://docs.kernel.org/driver-api/dmaengine/provider.html)
- [ ] **P1** [DMA and swiotlb](https://docs.kernel.org/core-api/swiotlb.html)
- [ ] **P1** [Device I/O tracepoints](https://docs.kernel.org/trace/index.html)

## Applied Checks

- [ ] Classify every callback in one project driver by execution context.
- [ ] Draw the lifetime and teardown order of its private state, IRQ, work, timers, and userspace interfaces.
- [ ] Explain every allocation flag and lock choice in that driver.
- [ ] Trace one MMIO write from driver call to accessor and hardware register.
- [ ] Trace one DMA buffer through CPU address, DMA address, ownership changes, completion, and cleanup.
