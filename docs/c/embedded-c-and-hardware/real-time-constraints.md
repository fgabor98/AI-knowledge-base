---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Real-Time Constraints

Real-time correctness means producing the required result before its deadline with bounded enough latency and resource use. A fast average is not sufficient. An embedded system can meet average timing while failing at the exact interrupt nesting, cache state, queue occupancy, flash wait state, or power transition that matters.

## Learning Objectives

- distinguish hard, firm, and soft real-time requirements;
- derive budgets for deadlines, latency, jitter, CPU, stack, queue, and energy;
- reason about blocking, priorities, interrupt nesting, and priority inversion;
- measure representative and worst-case behavior on the target;
- choose deterministic allocation, buffering, and timeout strategies;
- design watchdog and overload responses.

## Timing Vocabulary

| Term | Meaning |
| --- | --- |
| deadline | latest acceptable completion time |
| response time | arrival-to-completion duration |
| latency | delay before service starts or an event is observed |
| execution time | CPU time spent executing a unit of work |
| jitter | variation from a reference timing |
| period | interval between recurring releases |
| WCET | justified worst-case execution-time bound |
| utilization | fraction of processor capacity consumed |
| slack | time remaining before a deadline |

Classify each requirement as hard, firm, or soft. “The sample must be processed every 1 ms” is incomplete without a maximum latency, allowed misses, recovery, and measurement method.

## Deadline Budget

For a sensor pipeline, a budget might be:

```text
sensor event -> ISR capture       5 us
             -> DMA completion   20 us
             -> task wake        10 us
             -> decode           80 us
             -> control output   30 us
             -> safety margin    55 us
             total              200 us
```

Allocate margin for interrupt preemption, cache, bus contention, flash stalls, RTOS scheduling, clock tolerance, and measurement error. A sum of nominal times is not a WCET argument.

## Response-Time Analysis

For a periodic task, consider:

- its execution time and period;
- higher-priority task interference;
- ISR execution and arrival rate;
- blocking on mutexes, queues, or drivers;
- release jitter and timer granularity;
- context-switch and cache costs;
- non-preemptible sections;
- DMA and peripheral service time.

Priority assignment should follow the system’s deadlines and blocking analysis, not only software ownership. If a high-priority task waits for a lower-priority task holding a resource, use priority inheritance/ceiling or redesign ownership.

## Interrupt Latency

Interrupt latency includes masking, higher-priority service, entry stacking, synchronization, bus and flash wait, and any RTOS port behavior. Measure:

1. assert an input or trigger a timer;
2. capture a GPIO or trace marker at the first handler instruction;
3. repeat under maximum interrupt and DMA load;
4. include cache and power-state transitions;
5. record min, max, distribution, and outliers.

Do not measure only with a debugger or an idle system. A logic analyzer or cycle counter can provide less intrusive evidence.

## Blocking And Critical Sections

Every blocking operation needs:

- maximum wait or timeout;
- priority interaction;
- ownership and cancellation behavior;
- interrupt-context restriction;
- resource-full/empty behavior;
- watchdog and recovery policy.

Keep critical sections short and bounded. A critical section that disables interrupts around a loop with input-dependent length converts a data-size problem into a system-wide latency problem.

## Deterministic Resource Use

For time-sensitive paths, consider:

- static buffers and object pools;
- fixed-capacity queues with explicit overflow policy;
- arenas reset at known phase boundaries;
- bounded parsing and retry counts;
- precomputed tables;
- nonblocking state machines;
- DMA for large transfers;
- avoiding lazy initialization and page faults;
- fixed stack budgets and high-water monitoring.

Dynamic allocation is not automatically impossible, but the project must bound allocation time, fragmentation, locking, failure, and lifetime. General-purpose formatted I/O, filesystem access, and cryptography can have hidden cost.

## Clock And Tick Correctness

Use a monotonic time base for deadlines. Handle counter wrap using a documented unsigned-difference technique and avoid comparing absolute times with ordinary `<` when wrap is possible:

~~~c
#include <stdbool.h>
#include <stdint.h>

static bool deadline_reached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}
~~~

This assumes the deadline interval is less than half the counter range and that the conversion is valid for the target integer model. Define tick resolution and interrupt latency; a 1 ms tick does not guarantee 1 ms response.

## Overload And Degradation

Define what happens when work exceeds capacity:

- drop oldest/newest data;
- coalesce status events;
- reduce sampling or quality;
- shed diagnostics;
- enter a safe state;
- reset a failed subsystem;
- preserve a fault record;
- reject new commands with backpressure.

Do not let overload silently create unbounded queues, priority inversion, or watchdog starvation. Test overload as a normal operating mode, not only as a failure in the lab.

## Watchdog Design

A robust watchdog is refreshed by a supervisor after progress checks, not by every task independently. Track task heartbeats, deadlines, queue health, critical peripheral progress, and reset history. Set the watchdog timeout longer than the justified worst recovery path but short enough to meet product recovery requirements.

## Timing Evidence

Preserve:

- compiler/linker options and image identity;
- clock and power configuration;
- interrupt priorities and RTOS configuration;
- input workload and hardware revision;
- instrumentation method and overhead;
- min/max/percentile and outlier records;
- stack/heap/queue high-water data;
- temperature, voltage, and frequency conditions.

Timing evidence without configuration is not reproducible.

## Exercises

1. Derive a deadline budget for a sensor-to-actuator pipeline.
2. Measure interrupt latency under nested, DMA-heavy, and cache-changing workloads.
3. Add a blocking call to a high-priority task and analyze the resulting inversion.
4. Replace unbounded allocation or parsing with a bounded pool/state machine.
5. Test timer wraparound and deadline behavior near the counter boundary.
6. Fill every queue and verify overload policy and watchdog behavior.
7. Compare timing at debug, release, LTO, and power-saving configurations.

## Common Mistakes

- using average execution time as a deadline guarantee;
- omitting ISR, cache, bus, flash, and RTOS interference;
- disabling interrupts around unbounded work;
- assuming a tick period equals response latency;
- ignoring timer wraparound;
- allowing unbounded queues, retries, or parser loops;
- refreshing the watchdog without checking progress;
- measuring only idle hardware or debugger-attached runs;
- keeping timing claims without image/configuration provenance.

## Related Topics

- [Interrupts, Exceptions, And Faults](./interrupts-exceptions-and-faults.md)
- [DMA, Cache, And Memory Barriers](./dma-cache-and-memory-barriers.md)
- [RTOS Integration](./rtos-integration.md)
- [Testing Strategy](../correctness-quality-and-security/testing-strategy.md)
- [Advanced Performance And Code Size](../advanced-c/performance-and-code-size.md)

## References

- [FreeRTOS documentation](https://freertos.org/Documentation/)
- [FreeRTOS ISR API guidance](https://www.freertos.org/media/2018/161204_Mastering_the_FreeRTOS_Real_Time_Kernel-A_Hands-On_Tutorial_Guide.pdf)
- [C11 public draft N1570, integer types](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
