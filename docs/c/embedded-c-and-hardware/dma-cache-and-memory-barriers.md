---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# DMA, Cache, And Memory Barriers

Direct memory access (DMA) lets a peripheral or bus master read and write memory without the CPU copying every byte. The performance benefit comes with an ownership problem: the CPU, cache hierarchy, DMA engine, and possibly another core may hold different views of the same bytes. Correctness requires an explicit state machine for buffer ownership, addressability, cache maintenance, ordering, and lifetime.

## Learning Objectives

- distinguish compiler barriers, CPU memory barriers, cache maintenance, and device completion;
- choose coherent, non-cacheable, streaming, or explicitly synchronized memory;
- design aligned descriptors and buffers with ownership transitions;
- handle transmit, receive, and descriptor-ring lifecycles;
- reason about DMA address width, scatter/gather, and memory protection;
- test stale-cache, premature reuse, partial transfer, and error cases.

## Four Different Problems

| Problem | What it controls | Typical mechanism |
| --- | --- | --- |
| compiler reordering | optimizer movement of ordinary C accesses | compiler barrier, atomic operation, volatile where appropriate |
| CPU memory ordering | visibility/order among cores or agents | architecture barrier such as DMB/DSB, C atomic |
| cache visibility | whether RAM contents reach/come from cache | clean, invalidate, flush, coherent/non-cacheable mapping |
| peripheral completion | whether a device accepted a register/descriptor | status bit, readback, device barrier, interrupt |

None is a universal replacement for another. A cache clean does not necessarily order descriptor fields; a compiler barrier does not flush a cache; and a volatile store does not mean a DMA engine has observed it.

## Ownership State Machine

Define ownership for each buffer:

```text
CPU_WRITABLE -> CPU_CLEANED -> DMA_OWNED -> DMA_COMPLETE
       ^                                      |
       +----------- CPU_INVALIDATED <---------+
```

For transmit:

1. CPU fills payload and descriptor.
2. CPU cleans cache lines if needed.
3. CPU performs the required memory barrier.
4. CPU marks the descriptor valid or gives ownership to DMA.
5. DMA reads the buffer.
6. Completion is observed and ownership returns to CPU.

For receive:

1. CPU invalidates or prepares the buffer according to the platform API.
2. CPU gives ownership to DMA.
3. DMA writes the buffer and reports completion.
4. CPU observes completion with required ordering.
5. CPU invalidates cache lines before reading payload.
6. CPU processes and eventually returns the buffer.

Never reuse or modify a DMA-owned buffer. Put ownership transitions in one driver module rather than scattering cache calls through application code.

## Alignment And Cache Lines

Cache maintenance normally operates on whole lines. If a DMA buffer shares a line with unrelated CPU data, cleaning or invalidating the buffer can write back or discard the neighbor’s data. Use:

- cache-line alignment;
- cache-line-sized padding where necessary;
- a dedicated DMA section or allocator;
- exact range/length requirements from the platform;
- descriptors separated from unrelated mutable state.

Alignment for the CPU is not automatically alignment for the bus. Verify DMA address alignment, burst boundaries, maximum transfer size, boundary crossing, and address width.

## Coherent And Streaming Memory

Some systems provide coherent DMA mappings where CPU and device observe memory without explicit cache flushing. Coherent does not remove the need for ordering: descriptor fields can still be observed in the wrong order without a barrier. Other systems use streaming mappings that require explicit map/unmap or synchronize operations for each transfer.

The names vary by OS and SoC. On Linux, use the DMA mapping API rather than converting a CPU virtual address to a device address. On a microcontroller, use the vendor/HAL cache APIs or allocate from a configured non-cacheable region. Document whether a function returns a CPU address, DMA/bus address, or both.

## Descriptor Ordering

A common descriptor protocol is:

~~~c
#include <stddef.h>
#include <stdint.h>

#define DMA_STATUS_VALID 1u

void dma_clean(void *address, size_t length);
void dma_memory_barrier(void);
void dma_ring_doorbell(void);

struct dma_descriptor {
    uint32_t address;
    uint32_t length;
    uint32_t status;
};

static void submit_descriptor(struct dma_descriptor *descriptor,
                              uint32_t address,
                              uint32_t length)
{
    descriptor->address = address;
    descriptor->length = length;
    dma_clean(descriptor, sizeof(*descriptor));
    dma_memory_barrier();
    descriptor->status = DMA_STATUS_VALID;
    dma_clean(&descriptor->status, sizeof(descriptor->status));
    dma_ring_doorbell();
}
~~~

The exact placement of cache operations and barriers depends on whether the descriptor is coherent and how the hardware defines the ownership/valid bit. The example is a protocol sketch; use the platform’s documented primitives and inspect generated code when necessary.

## Address Translation And Protection

The CPU pointer and DMA address may differ because of an MMU, IOMMU, bus window, security state, or address remapping. Check:

- device address width and high-address support;
- physical/bus versus virtual address;
- secure/non-secure ownership;
- MPU/MMU permissions;
- memory type and shareability;
- bounce-buffer requirements;
- DMA isolation and malicious-device threat model.

Never cast a pointer to a 32-bit integer just because the peripheral register is 32 bits. Use a target-defined DMA address type and a checked mapping function.

## Descriptor Rings

Rings need invariants:

- producer and consumer indices remain within the ring;
- each slot has one owner at a time;
- wraparound arithmetic is defined;
- a descriptor is not reused until completion;
- errors return ownership and release resources;
- memory barriers match the producer/consumer direction.

For multicore or lock-free rings, use the architecture’s atomic and memory-order model. For a single-core peripheral ring, a critical section may protect software indices, but it does not replace device ordering or cache maintenance.

## Partial Transfers And Errors

DMA completion may mean fewer bytes, an error flag, a timeout, a bus fault, or a descriptor-chain termination. Always read status before returning a buffer to the pool. Invalidate only the bytes or cache lines that the hardware may have written, according to the platform API. Do not parse a receive buffer before validating the transferred length.

## Lifetime And Cancellation

Before stopping or resetting a peripheral:

1. prevent new submissions;
2. disable or mask completion interrupts;
3. stop the DMA engine and wait for quiescence;
4. resolve active descriptors and ownership;
5. synchronize/invalidate buffers as required;
6. release or reuse memory only after the device cannot access it;
7. clear stale status before restart.

Resetting a DMA controller while it still owns a buffer creates a use-after-free at the hardware boundary.

## Testing Coherency

Test with:

- cache enabled and disabled;
- aligned and deliberately misaligned requests where the API should reject them;
- buffers adjacent to unrelated data;
- back-to-back transfers and ring wrap;
- delayed and out-of-order completion;
- reset and cancellation while active;
- maximum address, length, and boundary values;
- memory protection and invalid descriptor faults.

Use hardware trace, cache counters, logic analyzers, and data-pattern checks. A host fake cannot prove coherency.

## Exercises

1. Draw CPU/DMA ownership transitions for transmit and receive buffers.
2. Add cache-line padding and demonstrate the adjacent-data corruption hazard in a model.
3. Implement descriptor-ring invariants and test wrap, full, empty, and error states.
4. Compare coherent and streaming mappings on a supported OS or SoC.
5. Measure transfer completion and cache-maintenance cost.
6. Cancel a transfer at each state and prove no buffer is reused early.
7. Inject stale status, invalid address, short transfer, and DMA error conditions.

## Common Mistakes

- treating `volatile` or a compiler barrier as cache coherency;
- assuming coherent memory eliminates memory barriers;
- using CPU pointers as DMA addresses;
- sharing cache lines with unrelated mutable data;
- modifying a buffer while DMA owns it;
- invalidating a line and discarding neighboring CPU data;
- forgetting descriptor ownership on error or reset;
- assuming completion means the requested length was transferred;
- restarting hardware before old DMA transactions are quiescent;
- validating only on a cache-disabled development build.

## Related Topics

- [Memory-Mapped I/O](./memory-mapped-io.md)
- [Interrupts, Exceptions, And Faults](./interrupts-exceptions-and-faults.md)
- [Peripheral Drivers](./peripheral-drivers.md)
- [Object Representation, Alignment, And Padding](../semantics-and-memory/object-representation-alignment-and-padding.md)
- [Linux Userspace And System Programming](../../linux-userspace-and-system-programming/index.md)

## References

- [Linux DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Arm cache coherency and barriers](https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/CacheCoherencyWhitepaper_6June2011.pdf)
- [CMSIS compiler control](https://arm-software.github.io/CMSIS_5/develop/Core/html/group__compiler__conntrol__gr.html)
- [FreeRTOS documentation](https://freertos.org/Documentation/)
