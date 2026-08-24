---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# RTOS Integration

An RTOS supplies scheduling, synchronization, timers, task isolation options, and often memory and interrupt integration. It does not automatically make C concurrent code safe or real-time. The application must still define ownership, priorities, blocking, stack, timing, ISR boundaries, and failure behavior.

## Learning Objectives

- choose tasks, queues, notifications, semaphores, mutexes, event flags, and timers;
- define task priorities, periods, budgets, stack sizes, and cancellation behavior;
- use ISR-safe APIs and request deferred context switches correctly;
- prevent races, deadlocks, priority inversion, and unbounded blocking;
- integrate static RTOS objects, memory protection, and libc reentrancy;
- test scheduler behavior, overload, reset, and resource exhaustion.

## Task Design

Each task should have a clear responsibility and contract:

- input events and queue ownership;
- priority and why it has that priority;
- maximum execution time and period/deadline;
- blocking APIs and maximum wait;
- stack budget and measured high-water mark;
- memory/privilege domain;
- shutdown, reset, and watchdog behavior;
- diagnostic identity and health signal.

Avoid a “god task” that owns every peripheral and state machine. Also avoid one task per trivial operation when context switches and synchronization obscure the timing model.

## Scheduling And Priorities

Choose a scheduling policy and document it. A higher priority should reflect deadline and interference needs, not organizational importance. Analyze:

- CPU utilization and burst load;
- interrupt time;
- blocking and mutex ownership;
- priority inversion;
- timer service task delays;
- idle/low-power requirements;
- startup and shutdown transitions.

A task that never blocks can starve the idle task and prevent watchdog, memory reclamation, or low-power work. Every loop needs an intentional wait, bounded work unit, or scheduler-yield policy.

## Queues And Ownership

Queues copy values or transfer references according to the RTOS contract. Decide whether to send:

- a small value/event;
- a fixed-size data record;
- a pointer to an owned buffer;
- a descriptor whose lifetime is managed by a pool.

For pointer queues, define who owns the object before send, after receive, on queue-full, on timeout, and on task deletion. A queue can synchronize a pointer while leaving the pointed-to data racy or prematurely freed.

## Mutexes, Semaphores, And Notifications

Use the primitive matching the concept:

| Primitive | Use |
| --- | --- |
| mutex | mutual exclusion with ownership and often priority inheritance |
| binary semaphore | event or resource token without ownership semantics |
| counting semaphore | count of available resources/events |
| queue | data plus synchronization |
| event flags | independent condition bits |
| direct task notification | compact task-specific event/value |
| spinlock/critical section | very short nonblocking protection, often ISR/multicore |

Do not use a binary semaphore as a mutex when ownership and priority inheritance matter. Do not use a mutex from an ISR. Do not use an event flag when each event occurrence must be counted unless the design explicitly coalesces events.

## ISR-to-Task Handoff

RTOSes commonly provide special ISR APIs. In FreeRTOS, APIs ending in `FromISR` are intended for interrupt use, and a higher-priority task wakeup may require a context-switch request before ISR exit:

~~~c
#include <stdint.h>

/* Names are FreeRTOS-style; use the exact port API in a real project. */
typedef int BaseType_t;
typedef void *TaskHandle_t;
enum { pdFALSE = 0, eSetBits = 0 };
#define portYIELD_FROM_ISR(value) ((void)(value))

static TaskHandle_t worker_task;
uint32_t peripheral_read_and_clear_status(void);
BaseType_t xTaskNotifyFromISR(TaskHandle_t task,
                              uint32_t value,
                              int action,
                              BaseType_t *higher_priority_task_woken);

void peripheral_isr(void)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    uint32_t event = peripheral_read_and_clear_status();

    xTaskNotifyFromISR(worker_task,
                       event,
                       eSetBits,
                       &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}
~~~

The snippet is a platform example, not ISO C. Verify interrupt priority limits, nesting support, queue-copy behavior, and port macros. Never call a task-only blocking API from an ISR.

## Timeouts And Clocks

Choose a monotonic RTOS tick or hardware time base and define:

- tick frequency and wrap behavior;
- timeout rounding;
- maximum block duration;
- behavior when the scheduler is suspended;
- tickless idle effects;
- clock changes and timer recalibration.

Use absolute deadlines for periodic work when drift matters. A loop that sleeps for “period” after variable work accumulates phase drift; a loop that schedules the next absolute release can report missed deadlines more accurately.

## Priority Inversion And Deadlock

For every lock, record order and ownership. Avoid cycles such as task A holding lock 1 while waiting for lock 2 and task B doing the reverse. Bound critical sections and use priority inheritance/ceiling where appropriate. Be careful: priority inheritance can increase memory and scheduling complexity; it does not repair lock-order cycles.

## Static Objects And Memory Protection

Static RTOS objects can make startup, memory budgeting, and failure behavior more deterministic. If dynamic objects are used, measure allocation and failure. For MPU/privileged RTOS modes, define:

- task memory regions and stacks;
- privileged driver entry points;
- shared buffers and access rights;
- ISR privilege and secure/non-secure state;
- fault response for invalid access;
- DMA access that may bypass task protection.

An RTOS memory boundary is not automatically a DMA or peripheral security boundary.

## libc, C Runtime, And RTOS

Integrate `errno`, stdio, malloc, locale, TLS, and reentrancy explicitly. Decide whether:

- libc state is per task;
- allocator locks are RTOS-aware;
- formatted I/O is forbidden in timing paths;
- callbacks can run in ISR or task context;
- `main` creates tasks or becomes a task;
- termination is reset, idle, or scheduler-specific.

Test C library calls from multiple tasks and under allocation failure. Do not assume a libc’s hosted thread-safety policy maps cleanly onto an RTOS port.

## Error And Recovery Tasks

Every driver/task should have a failure policy:

- retry with bounded backoff;
- reset a peripheral and reinitialize it;
- discard or quarantine a buffer;
- notify a supervisor;
- enter degraded/safe mode;
- persist a fault record;
- stop accepting work.

Recovery must synchronize with in-flight DMA, interrupts, queues, and callbacks. Deleting a task does not automatically cancel hardware or release every object it owns.

## Observability

Measure and expose:

- task execution and blocked time;
- priority and context-switch counts;
- stack high-water marks;
- queue occupancy and drops;
- mutex wait and hold times;
- ISR-to-task latency;
- timer lateness;
- heap allocation/failure/fragmentation;
- watchdog health and reset cause.

Use trace hooks or counters that remain valid without a debugger. Avoid logging from high-priority or ISR contexts when the logger can block.

## Exercises

1. Design a task and queue architecture for a DMA-backed UART.
2. Compare a queue, notification, semaphore, and event flag for four event types.
3. Create a priority inversion and measure it with and without inheritance.
4. Test queue-full, task-timeout, task-delete, and peripheral-reset races.
5. Size task stacks from measurements plus a reviewed margin.
6. Integrate a libc allocator and test concurrent and failed allocations.
7. Build a supervisor that refreshes the watchdog only after health checks pass.

## Common Mistakes

- using the wrong RTOS API in an ISR;
- assigning priorities without deadline/interference analysis;
- sending borrowed pointers without lifetime ownership;
- using event flags where event counts must not be lost;
- taking mutexes in interrupt context;
- allowing a high-priority task to run without blocking;
- ignoring timer tick wrap and tickless behavior;
- deleting tasks while hardware and callbacks remain active;
- assuming RTOS protection covers DMA and peripherals;
- failing to measure stack, queue, lock, and ISR behavior.

## Related Topics

- [Interrupts, Exceptions, And Faults](./interrupts-exceptions-and-faults.md)
- [Real-Time Constraints](./real-time-constraints.md)
- [DMA, Cache, And Memory Barriers](./dma-cache-and-memory-barriers.md)
- [Embedded libc Implementations](../standard-library-and-ecosystem/embedded-libc.md)
- [Atomics, Threads, And Signals](../standard-library-and-ecosystem/atomics-threads-and-signals.md)

## References

- [FreeRTOS queues and ISR API](https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/01-Queues)
- [FreeRTOS task notifications from ISR](https://freertos.org/xTaskNotifyFromISR.html)
- [FreeRTOS customization and interrupt priorities](https://www.freertos.org/Documentation/02-Kernel/03-Supported-devices/02-Customization)
- [CMSIS-RTOS2 theory of operation](https://arm-software.github.io/CMSIS_5/RTOS2/html/theory_of_operation.html)
