---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Interrupts, Exceptions, And Faults

Interrupts and exceptions introduce asynchronous control flow into a C system. An interrupt can arrive between any two instructions, change shared state, preempt a critical section, or expose an initialization race. A fault handler runs in an already-invalid context and must preserve evidence while avoiding a second failure.

## Learning Objectives

- design bounded ISR entry, acknowledgement, capture, and deferred-work paths;
- understand interrupt priorities, nesting, masking, and latency;
- protect shared state with appropriate atomicity and memory ordering;
- distinguish interrupt-safe, task-safe, and fault-safe APIs;
- collect useful fault and reset evidence;
- reason about watchdogs, storms, starvation, and recovery.

## ISR Responsibilities

A good ISR normally:

1. identifies the source;
2. acknowledges or clears it according to the peripheral contract;
3. captures minimal status/data into a bounded buffer;
4. records an event or notifies deferred work;
5. requests a context switch if the RTOS requires it;
6. returns quickly.

Avoid parsing protocols, formatting logs, allocating memory, taking ordinary mutexes, or performing unbounded loops in an ISR. If the peripheral requires a long service sequence, use a hardware FIFO/DMA or defer the work.

## Shared ISR State

`volatile` can be appropriate for a flag observed by an ISR and foreground code, but it does not guarantee a compound operation or memory ordering:

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static volatile bool sample_ready;
static volatile uint16_t sample_value;
static volatile uint16_t sample_reg;
static volatile uint32_t sample_int_clear;

#define SAMPLE_REG sample_reg
#define SAMPLE_INT_CLEAR sample_int_clear
#define SAMPLE_DONE 1u

void SAMPLE_IRQHandler(void)
{
    sample_value = SAMPLE_REG;
    sample_ready = true;
    SAMPLE_INT_CLEAR = SAMPLE_DONE;
}

bool sample_take(uint16_t *out)
{
    if (out == NULL || !sample_ready) {
        return false;
    }
    *out = sample_value;
    sample_ready = false;
    return true;
}
~~~

This example is only safe under a specific hardware and concurrency contract. The foreground can race with the ISR, events can be overwritten, and the order of data/flag accesses may require barriers. For multiple events, use an atomic operation, a critical section, a lock-free single-producer structure, or an RTOS ISR primitive designed for the target.

## Interrupt Priority And Nesting

Document:

- numerical priority meaning on the architecture;
- which interrupts can preempt which;
- which priorities may call RTOS APIs;
- maximum nesting depth and stack use;
- critical-section masking scope;
- latency budget for higher-priority sources;
- shared peripheral and DMA ownership.

On Cortex-M systems, priority numbering and implemented priority bits can surprise engineers. Configure and verify grouping, preemption, and subpriority fields using the device/RTOS port contract rather than assuming a generic value.

## Critical Sections

A critical section should protect a specific invariant and last no longer than necessary. Consider whether it must:

- disable all maskable interrupts;
- mask only a priority range;
- use a spinlock on a multicore system;
- use an atomic operation;
- stop a DMA engine or coordinate with a peripheral;
- preserve interrupt state rather than blindly re-enable interrupts.

Never call a blocking API or wait for an event while holding interrupts disabled. Measure maximum duration, including compiler-generated code and cache effects.

## Deferred Work

Use an ISR-to-task handoff for work that can wait or take longer:

- ring buffer plus task notification;
- queue of small values or descriptors;
- binary/counting semaphore for event count;
- event flags for independent conditions;
- DMA completion plus ownership transfer;
- scheduler-specific deferred interrupt work.

The handoff must define overflow behavior. Dropping an event may be safe for a level-triggered status bit but unsafe for a pulse counter or data stream. Test full queues, repeated interrupts, and a consumer that is delayed by higher-priority work.

## Exception And Fault Classes

Architecture-specific faults can indicate:

- invalid instruction or execution state;
- unaligned or inaccessible memory;
- bus/peripheral error;
- MPU/TrustZone permission violation;
- divide-by-zero or illegal operation;
- stack overflow or corrupted return state;
- debug event or breakpoint;
- watchdog reset without a synchronous fault.

Capture the architecture’s exception frame, status registers, fault address, active exception, stack pointer, and reset cause. Interpret them with the exact core revision and ABI.

## Fault Handler Design

A fault handler should:

1. select the correct stacked frame if the architecture provides more than one;
2. preserve registers and status before clearing evidence;
3. write a bounded, checksum-protected retention record;
4. disable or contain unsafe peripherals and outputs;
5. notify a supervisor or reset according to product policy;
6. avoid allocation, locks, complex formatting, and unverified MMIO.

Example record shape:

~~~c
#include <stdint.h>

struct fault_record {
    uint32_t magic;
    uint32_t reason;
    uintptr_t program_counter;
    uintptr_t stack_pointer;
    uint32_t status;
    uint32_t checksum;
};
~~~

The storage must be reserved from normal initialization if it must survive reset. Validate records on the next boot and clear them only after successful export or explicit acknowledgment.

## Watchdogs And Reset-Cause Analysis

A watchdog is not a substitute for finding the cause. Define:

- which task or supervisor is allowed to refresh it;
- the deadline and refresh window;
- what evidence is stored before reset;
- whether a stuck high-priority task can prevent refresh;
- how repeated resets enter recovery mode;
- how an intentional long operation extends or services the deadline safely.

Do not sprinkle watchdog refreshes through arbitrary loops. That can convert a deadlocked system into an apparently healthy one. A supervisor should refresh only after required tasks, communication, and safety checks report progress.

## Interrupt Storms And Starvation

An interrupt can remain pending because the source was not cleared, a level condition remains true, or a status bit was read incorrectly. Detect storms with counters, rate limits, and telemetry. Protect lower-priority work from starvation with hardware masking, coalescing, DMA, backpressure, and bounded deferred processing.

## Exercises

1. Measure ISR entry-to-exit latency with a GPIO or trace marker.
2. Design a single-producer event ring and test overflow and wraparound.
3. Compare a flag, queue, semaphore, and task notification for one event source.
4. Trigger each available fault class and decode the exact exception frame.
5. Build a retention fault record and verify corruption detection after reset.
6. Create an interrupt storm and test containment without hiding the root cause.
7. Review every ISR API call in a driver and classify it as interrupt-safe or invalid.

## Common Mistakes

- doing substantial application work in an ISR;
- failing to clear or acknowledge the exact interrupt source;
- using `volatile` as a complete synchronization design;
- ignoring event loss and queue-full behavior;
- using the wrong interrupt priority or RTOS API level;
- disabling interrupts for longer than the deadline budget;
- assuming a fault handler can use ordinary logging and allocation;
- refreshing the watchdog from a loop that is already stuck;
- erasing reset evidence during early initialization;
- testing handlers only with a debugger attached.

## Related Topics

- [Memory-Mapped I/O](./memory-mapped-io.md)
- [DMA, Cache, And Memory Barriers](./dma-cache-and-memory-barriers.md)
- [RTOS Integration](./rtos-integration.md)
- [Real-Time Constraints](./real-time-constraints.md)
- [Debugging With GDB](../correctness-quality-and-security/debugging-with-gdb.md)

## References

- [CMSIS NVIC and interrupt documentation](https://arm-software.github.io/CMSIS_5/Core/html/group__NVIC__gr.html)
- [CMSIS vector table guidance](https://arm-software.github.io/CMSIS_5/5.7.0/Core/html/group__NVIC__gr.html)
- [FreeRTOS ISR API guidance](https://www.freertos.org/media/2018/161204_Mastering_the_FreeRTOS_Real_Time_Kernel-A_Hands-On_Tutorial_Guide.pdf)
- [FreeRTOS task notifications from ISR](https://freertos.org/xTaskNotifyFromISR.html)
