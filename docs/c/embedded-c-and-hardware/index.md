---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Embedded C And Hardware

Embedded C is where the C abstract machine meets a processor, memory system, peripherals, interrupts, DMA engines, boot ROM, power states, and a product’s timing and recovery requirements. The language remains important, but it is no longer the whole execution model.

This chapter teaches how to make that boundary explicit. It covers freestanding runtime support, reset and startup, memory-mapped I/O, exceptions, DMA and cache coherency, real-time constraints, drivers, RTOS integration, and firmware images.

## The Embedded Execution Model

```text
reset / boot ROM
        |
        v
vector table -> reset handler -> clocks/memory -> C runtime
        |                                      |
        v                                      v
interrupts / faults <-> drivers <-> RTOS/tasks/application
                              |
                              v
                 DMA/cache/peripheral hardware
```

Every arrow is a contract. For example:

- startup must initialize memory before ordinary C objects are used;
- an ISR must clear or acknowledge the source and hand off bounded work;
- a driver must define register ordering, ownership, timeout, and reset behavior;
- a DMA buffer must obey address, alignment, cache, and lifetime rules;
- an RTOS API must be called from the permitted context and priority range;
- a bootloader must validate and hand off an image with a known machine state.

## Recommended Progression

1. [Freestanding C](./freestanding-c.md) — define the runtime and library boundary.
2. [Startup, Reset, And Vector Tables](./startup-reset-and-vector-tables.md) — follow reset into C.
3. [Memory-Mapped I/O](./memory-mapped-io.md) — access registers without confusing them with RAM.
4. [Interrupts, Exceptions, And Faults](./interrupts-exceptions-and-faults.md) — handle asynchronous execution and failure.
5. [DMA, Cache, And Memory Barriers](./dma-cache-and-memory-barriers.md) — coordinate CPU and peripheral memory views.
6. [Real-Time Constraints](./real-time-constraints.md) — reason about deadlines, jitter, and bounded resources.
7. [Peripheral Drivers](./peripheral-drivers.md) — build layered, testable hardware interfaces.
8. [RTOS Integration](./rtos-integration.md) — integrate tasks, synchronization, and interrupt handoff.
9. [Bootloaders And Firmware Images](./bootloaders-and-firmware-images.md) — create updateable and recoverable images.

## Context Matrix

| Context | Allowed work | Typical restrictions |
| --- | --- | --- |
| reset/early startup | stack, clocks, memory, minimal diagnostics | libc, heap, peripherals, and interrupts may not be ready |
| thread/task | blocking and ordinary driver APIs according to policy | priority, stack, deadline, and ownership constraints |
| ISR | acknowledge source, capture bounded state, notify/defer | no blocking; usually no allocation or ordinary RTOS calls |
| fault handler | capture CPU and fault state, enter safe response | logging and memory access may themselves fault |
| DMA completion | validate descriptor/status, transfer ownership | cache maintenance and ordering must be correct |
| bootloader | validate, select, recover, and hand off images | isolation, rollback, power loss, and debug policy |

Put the matrix in API documentation and code review checklists. A function that is safe in a task may be invalid in an ISR even if the C prototype is identical.

## Hardware Facts Versus C Facts

| Topic | C can describe | Hardware/platform must define |
| --- | --- | --- |
| `volatile` object | observable accesses in the C implementation | register side effects and ordering |
| pointer value | address representation in the implementation | valid bus region and privilege |
| atomic operation | language-level atomicity | instruction support and peripheral visibility |
| memory barrier | compiler/CPU ordering primitive | which bus masters and caches it orders |
| interrupt function | callable function shape | vector encoding, priority, stacking |
| DMA buffer | object storage and alignment request | DMA address, cache, ownership, coherency |
| timeout | integer/time calculation | clock source, tick rate, wakeup latency |

Do not hide a hardware assumption behind a cast. Name it, isolate it, and test it with target evidence.

## Layered Hardware Architecture

A maintainable system commonly separates:

1. **register/BSP layer** — addresses, bit definitions, clocks, pin mux, reset;
2. **primitive driver layer** — bounded register transactions and interrupt/DMA control;
3. **device service layer** — buffering, state machine, timeout, and error recovery;
4. **portable policy layer** — protocol, control, data processing, and application behavior;
5. **integration layer** — RTOS, bootloader, power, security, and product configuration.

Keep the policy layer free of device header dependencies where possible. Test it on the host; test the primitive and integration layers on hardware.

## Bring-Up Order

A disciplined bring-up sequence is:

1. verify image format, vector table, entry point, and memory placement;
2. establish a minimal reset/fault indicator;
3. initialize stack, data, bss, clocks, and memory protection;
4. verify one GPIO or timer with a scope/logic analyzer;
5. establish a reliable nonblocking diagnostic channel;
6. initialize one peripheral and its interrupt path;
7. add DMA and cache ownership checks;
8. start the RTOS and measure scheduler/interrupt behavior;
9. exercise watchdog, reset, power, update, and recovery paths;
10. remove bring-up assumptions and temporary debug paths from production.

## Chapter Outcomes

After completing this chapter, you should be able to:

- explain the complete reset-to-application execution path;
- write freestanding code with a deliberate runtime and library boundary;
- access MMIO registers with correct width, masks, side effects, and ordering;
- design ISR and fault paths that are bounded, observable, and context-safe;
- maintain DMA/cache coherency and CPU/peripheral ownership;
- derive real-time budgets and measure the worst relevant behavior;
- layer drivers behind testable interfaces and integrate them with an RTOS;
- design firmware images with authentication, rollback, power-loss recovery, and safe handoff.

## Running Project

Implement a UART-backed sensor service:

1. start from a host-testable frame parser;
2. add a bare-metal register driver;
3. capture bytes in an ISR into a bounded ring;
4. use DMA for larger transfers with explicit cache/ownership operations;
5. defer parsing to an RTOS task;
6. measure ISR latency, task deadline, stack, queue, and power budgets;
7. record faults in retention memory;
8. package the image behind a bootloader with rollback and recovery tests.

## Related Topics

- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Standard Library And Ecosystem](../standard-library-and-ecosystem/index.md)
- [Platform-Specific C](../platform-specific-c/index.md)
- [Embedded Productization](../../embedded-productization/index.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- [CMSIS-Core documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [CMSIS startup file](https://arm-software.github.io/CMSIS_5/5.8.0/Core/html/startup_c_pg.html)
- [CMSIS NVIC and vector tables](https://arm-software.github.io/CMSIS_5/5.7.0/Core/html/group__NVIC__gr.html)
- [FreeRTOS documentation](https://freertos.org/Documentation/)
- [Linux DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Trusted Firmware-M rollback protection](https://tf-m.docs.trustedfirmware.org/en/latest/design_docs/booting/secure_boot_rollback_protection.html)
