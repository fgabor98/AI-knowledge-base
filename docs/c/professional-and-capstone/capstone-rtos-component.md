---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Capstone: RTOS Component

Build an RTOS component that owns a realistic peripheral or data-processing service.
The component must separate hardware access from policy, define an ISR/task boundary,
use bounded queues and synchronization, prove its timeout behavior, and recover from
device and task faults. Use the RTOS's static-allocation APIs where practical and make
every execution-context rule explicit.

## Project Brief

Choose a component such as:

- UART/SPI sensor service with DMA;
- bounded telemetry producer and transport worker;
- motor/actuator command service;
- flash-storage worker with asynchronous requests;
- network receive component with packet pools.

The component shall provide a synchronous or asynchronous application API while
internally handling interrupts, queues, timeouts, hardware errors, and recovery. It
must have a host-testable policy layer and a target adapter.

## Component Contract

Write down:

- states: uninitialized, stopped, starting, ready, busy, recovering, failed;
- commands/events and their ownership;
- caller context and whether each API can block;
- maximum request, response, queue, and buffer sizes;
- timeout units and clock source;
- priority, stack, CPU affinity, and scheduling policy;
- interrupt priority and which RTOS APIs are legal from the ISR;
- cancellation and shutdown behavior;
- reset, device removal, and recovery behavior;
- diagnostics and health metrics.

The public API should not expose an RTOS queue handle or a register pointer unless that
is deliberately the component's platform contract. Keep application code independent
of scheduler primitives where possible.

## Layered Architecture

```text
application policy
        |
component API/state machine
        |
request queue + worker task + completion/event interface
        |
ISR/DMA adapter and hardware abstraction
        |
RTOS port, BSP, peripheral registers
```

The hardware adapter translates register status and interrupts into typed events. The
policy/state-machine layer decides retries, deadlines, and user-visible errors. This
separation lets host tests inject events and lets target tests verify only the adapter,
queue, timing, and hardware behavior.

## ISR/Task Boundary

The ISR should usually:

1. read/capture the minimum status and timestamp;
2. acknowledge or mask the source according to the peripheral protocol;
3. move/mark DMA ownership safely;
4. enqueue a bounded event or notify a worker;
5. request a context switch only through the RTOS's ISR-safe API.

The worker should perform operations that can block, allocate, format, retry, or take
ordinary mutexes. Define what happens if the ISR event queue is full: drop with a
counter, overwrite a stale event, disable the source and enter recovery, or apply
backpressure. Never silently lose a safety-critical event.

Check every RTOS call against its context-specific contract. A function with an `FromISR`
suffix is not necessarily safe at every interrupt priority; some RTOS ports mask or
defer only below a configured priority ceiling.

## Queues, Buffers, And Ownership

Use fixed-size messages or a pool plus handles. A queue item should contain enough
information to identify the buffer and generation, not a pointer whose lifetime is
implicit. Define:

- who owns a buffer before enqueue;
- who owns it while queued;
- who owns it during DMA;
- who releases it on success, timeout, cancellation, reset, and queue failure;
- how stale completions are rejected after a reset.

For DMA, add cache maintenance and descriptor barriers where the target requires them.
For zero-copy buffers, define when the producer may reuse memory. A queue protects
ordering; it does not automatically protect the object referred to by the queue item.

## Synchronization Design

Choose primitives by context and purpose:

- binary/counting semaphore for event/resource signaling;
- mutex for exclusive ownership with priority inheritance where supported;
- event flags for independent bounded state bits;
- queue/message buffer for data transfer and backpressure;
- task notification for a lightweight one-task signal;
- atomic for small state/counters when the RTOS/architecture supports the required
  width and ordering;
- critical section only for a short, documented local protection interval.

Record lock order and maximum hold time. Do not hold a mutex across a hardware wait,
callback, blocking queue, or unknown application hook unless the design proves it safe.
Use a state machine to serialize start/stop/recovery rather than allowing every public
API to race the worker.

## Timeout And Clock Semantics

A timeout needs a clock definition:

- tick-based or monotonic high-resolution;
- relative or absolute deadline;
- behavior across tick wraparound;
- resolution and maximum representable interval;
- scheduler/tickless-sleep effect;
- whether the timeout includes queue wait, DMA, retries, and recovery.

Compute one absolute deadline and pass remaining time to each wait where possible. Do
not restart a full timeout after every retry. Define what happens at exactly the
deadline and whether a late completion is discarded, accepted for the next operation,
or reported as a timeout.

## Static Allocation And Resource Budgets

Record sizes and high-water marks for:

- worker and timer task stacks;
- queues, event pools, DMA descriptors, and packet buffers;
- RTOS control blocks and idle/timer tasks;
- mutex/semaphore/event objects;
- logging and recovery storage.

Use stack watermarking and overflow detection, but do not treat a watermark as proof
against every call path. Include interrupt stack usage, FPU context, trace hooks, and
compiler optimization. Put a static allocation failure on a tested startup path rather
than allowing a null handle to fail later.

## Priority And Scheduling Analysis

For each task, document period/deadline, priority, execution-time budget, blocking
resources, interrupt sources, and queue depth. Check:

- priority inversion and inheritance/ceiling;
- starvation from a high-rate producer;
- queue backpressure and burst size;
- interrupt storm behavior;
- worker recovery priority;
- watchdog service and health monitoring;
- CPU affinity/cache contention on SMP targets.

Measure response and jitter under worst supported interrupt, DMA, logging, and power
conditions. An average task runtime does not establish deadline compliance.

## Fault Recovery State Machine

A component recovery sequence might be:

```text
READY -> TIMEOUT -> QUIESCE -> RESET_DEVICE -> REINITIALIZE
                         |                         |
                         +------> FAILED <---------+
```

Specify how outstanding requests are completed, canceled, or failed; how buffers and
DMA descriptors are reclaimed; how interrupts are masked/cleared; how the device reset
is synchronized; and when the component becomes available again. Increment a generation
number so late ISR/DMA completions from the old instance cannot satisfy new requests.

Do not recover by deleting a task while it owns buffers or locks unless the RTOS and
component contract proves cleanup. Prefer cooperative shutdown and a dedicated recovery
worker for complex resources.

## Driver Abstraction

Keep a small hardware interface:

- configure/start/stop;
- submit/read/write;
- acknowledge/status;
- DMA ownership/cache operations;
- reset and power transitions;
- timestamp or time source;
- fault injection hooks for tests.

The fake implementation should simulate short transfer, timeout, interrupt ordering,
stale completion, reset, and resource failure. Do not make the fake unrealistically
helpful by completing every request synchronously if the real driver is asynchronous.

## Testing Strategy

### Host policy tests

- all state/event transitions, including invalid and duplicate events;
- timeout/deadline arithmetic and tick wraparound model;
- queue full/empty and buffer ownership transitions;
- retry/backoff and terminal failure;
- cancellation during every state;
- stale generation rejection;
- watchdog/health policy;
- fault and recovery event sequences.

### RTOS integration tests

- actual task priorities, stack watermark, queue blocking, and notification behavior;
- ISR-to-task handoff at permitted interrupt priorities;
- scheduler suspension and tickless behavior;
- allocation failure and object deletion policy;
- trace hooks and timing under load;
- SMP affinity and cache effects where applicable.

### Hardware-in-the-loop

- device reset/power loss and bus errors;
- DMA/cache and buffer alignment;
- interrupt storms, missing interrupts, and spurious status;
- clock changes and sleep/wake;
- watchdog and fault injection;
- long-run soak with full queues and repeated recovery;
- measured deadline/jitter/power results.

## Diagnostics

Include a component instance ID, generation, state, queue depth, request ID, last event,
deadline, retry count, reset cause, and hardware status in diagnostic records. Make
records bounded and safe from ISR/fault context. Rate-limit repeated failures but retain
counts and first/last timestamps.

Create a host decoder or trace view that can reconstruct a failed transaction. A log
that says “timeout” without request/state/generation/device context is rarely enough.

## Milestones

1. Requirements, state machine, ownership table, and timing budget.
2. Host policy state machine with fake hardware/events.
3. RTOS task/queue/timeout integration using static resources.
4. ISR/DMA adapter and target startup.
5. Recovery, watchdog/health, and diagnostic record.
6. Hardware fault/soak/timing tests.
7. Release review with stack/queue/map/timing evidence.

## Assessment Rubric

- **Context safety:** ISR/task/API rules are explicit and obeyed.
- **Concurrency:** queues, locks, atomics, ownership, and stale completions are safe.
- **Timing:** deadlines, jitter, blocking, and tick behavior are measured and bounded.
- **Recovery:** timeout, reset, cancellation, and partial progress have a complete state
  machine and tests.
- **Resources:** static allocation, stack, buffers, queue depth, and failure behavior
  are documented.
- **Portability/testability:** policy runs on host with realistic fakes.
- **Diagnostics:** an operator can explain a failed request from retained evidence.

## Common Mistakes

- Calling blocking or non-ISR-safe APIs from an interrupt.
- Treating a queue entry as ownership proof for the pointed-to buffer.
- Restarting a full timeout after each retry.
- Ignoring RTOS priority ceilings, tick wraparound, or tickless behavior.
- Deleting a task with live DMA, locks, callbacks, or buffers.
- Recovering the device without rejecting late completions from the old generation.
- Testing only a cooperative fake that never produces races, delays, or errors.
- Feeding the watchdog regardless of component health.

## Related Topics

- [Professional Practice And Capstones overview](./index.md)
- [RTOS Integration](../embedded-c-and-hardware/rtos-integration.md)
- [Interrupts, Exceptions, And Faults](../embedded-c-and-hardware/interrupts-exceptions-and-faults.md)
- [DMA, Cache, And Memory Barriers](../embedded-c-and-hardware/dma-cache-and-memory-barriers.md)
- [C Memory Model And Concurrency](../advanced-c/c-memory-model-and-concurrency.md)
- [Multicore And Heterogeneous Systems](../platform-specific-c/multicore-and-heterogeneous-systems.md)

## References

- [FreeRTOS kernel documentation](https://freertos.org/Documentation/02-Kernel/01-About-the-FreeRTOS-kernel)
- [FreeRTOS interrupt management](https://freertos.org/Documentation/02-Kernel/04-API-references/05-Software-timers/01-xTimerCreate)
- [Zephyr kernel services](https://docs.zephyrproject.org/latest/kernel/services/index.html)
- [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- The exact RTOS port guide, interrupt-priority rules, scheduler configuration, DMA
  manual, board power/reset behavior, and timing requirements
