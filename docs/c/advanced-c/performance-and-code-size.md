---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Performance And Code Size

Performance engineering is the process of meeting a measured budget under a defined
workload and environment. In embedded systems the budget may include execution time,
interrupt latency, jitter, SRAM, flash, energy per operation, thermal headroom, and
worst-case completion time. Optimizing a function in isolation can make the system
worse by increasing code size, cache pressure, power, or verification complexity.

## Learning Objectives

- Define performance, size, energy, latency, and worst-case budgets before optimizing.
- Measure representative workloads without introducing benchmark artifacts.
- Relate C layout and control flow to caches, branches, pipelines, DMA, and memory buses.
- Use compiler optimization, LTO, PGO, section garbage collection, and linker reports
  intentionally.
- Distinguish average throughput from tail latency and worst-case execution time.
- Build regression tests that protect both correctness and resource budgets.

## Define The Budget

Write the requirement as a measurable statement:

```text
At 160 MHz, processing one sensor frame of 128 samples shall complete in <= 200 us
for every valid input, use <= 4 KiB of temporary SRAM, and add <= 3 mJ per frame.
The interrupt-disabled interval shall remain below 8 us at the 99.999th percentile.
```

Clarify the environment: compiler and flags, cache state, clock/power mode, DMA
traffic, interrupt load, input distribution, thermal state, and whether the requirement
is average, percentile, maximum observed, or analytically bounded. A benchmark number
without these conditions is not a reusable engineering result.

Track at least:

- throughput and per-operation latency;
- p50, p95, p99, and maximum latency where distributions matter;
- stack high-water mark and peak dynamic allocation;
- flash/ROM, RAM, relocation, TLS, and retained sections;
- energy per operation and wake/sleep overhead;
- interrupt response and scheduling jitter;
- code complexity and proof/maintenance cost.

## Measurement Before Modification

Use a hierarchy of evidence:

1. **System measurement:** end-to-end latency, power, dropped work, and user-visible
   behavior.
2. **Subsystem profiling:** time by task, thread, ISR, driver, and allocator.
3. **Function profiling:** hot functions and call paths.
4. **Hardware evidence:** cycles, cache misses, branch misses, stalls, bus utilization,
   DMA contention, and energy counters.
5. **Generated-code inspection:** instruction count, loads/stores, branches, calls,
   vectorization, and section placement.

Start with a profile or trace, not a favorite optimization. A function that consumes
20% of CPU but runs once per second may matter less than a 1% function on a 1 kHz path.
Measure a baseline after every meaningful change and preserve the input corpus.

## Benchmarking C Code

A microbenchmark must prevent the compiler from deleting or simplifying the work. Use
opaque inputs, consume outputs, repeat enough iterations to overcome timer resolution,
separate setup from the measured region, and randomize or warm up state when cache
effects are part of the target behavior.

```c
#include <stddef.h>
#include <stdint.h>

static volatile uint32_t benchmark_sink;

static uint32_t mix_bytes(const unsigned char *data, size_t length)
{
    uint32_t result = 2166136261u;
    for (size_t index = 0u; index < length; ++index) {
        result ^= data[index];
        result *= 16777619u;
    }
    return result;
}

static void run_measurement(const unsigned char *data, size_t length,
                            size_t iterations)
{
    uint32_t result = 0u;
    for (size_t iteration = 0u; iteration < iterations; ++iteration) {
        result ^= mix_bytes(data, length);
    }
    benchmark_sink = result;
}
```

The volatile sink prevents a whole-program optimizer from discarding the result, but
it is not a timing clock and can itself affect the final iteration. Use a target timer,
cycle counter, trace tool, or OS profiler around the appropriate region. A benchmark
that runs only from cache may be useful for steady-state throughput and useless for
first-use latency.

## Cache-Aware Layout

Cache behavior is driven by locality and working-set size:

- **Temporal locality:** reuse data while it is likely to remain in a cache.
- **Spatial locality:** place data accessed together near each other.
- **Instruction locality:** keep hot paths and frequently called functions compact.

Array-of-structures is convenient when each iteration uses all fields of one object;
structure-of-arrays can be better when a loop uses one field across many objects. Split
hot and cold fields so rarely used metadata does not consume cache lines. Align DMA and
shared data to the target requirements, but avoid padding without measuring the memory
cost and cache-line behavior.

On non-coherent DMA systems, cache-aware layout also includes clean/invalidate cost and
ownership. A layout that is fast for the CPU can be expensive for a peripheral or can
be incorrect if the driver does not synchronize it.

## Branches And Control Flow

Branches cost more when mispredicted or when they prevent vectorization. Do not remove
a branch merely to make the code look branchless; a branchless expression can execute
more expensive operations, leak timing, or overflow intermediate values.

Useful techniques include:

- separate common and exceptional paths;
- sort or batch inputs when it improves predictability;
- use lookup tables only when their memory footprint and cache behavior fit;
- express invariants so the compiler can see bounds and alignment;
- use target intrinsics only behind a measured, portable interface;
- keep error paths cold without hiding required diagnostics.

Security-sensitive code may require constant-time behavior, which is a different
constraint from average performance. Review branches, memory accesses, compiler
transformations, and target instructions under the relevant threat model.

## Code-Size Optimization

Flash size affects cost, update capacity, instruction-cache behavior, and sometimes
power. RAM size affects concurrency, buffering, and stack safety. Techniques include:

- remove unused sections with `-ffunction-sections -fdata-sections` plus linker garbage
  collection when the startup/linker contract supports it;
- use LTO to expose cross-file inlining and dead-code opportunities;
- choose `-Os`/`-Oz` or target-specific size options after measuring speed and latency;
- reduce duplicate format strings, tables, wrappers, and error paths;
- select a smaller libc feature set or avoid floating formatting when not needed;
- use packed encodings only when decode cost and alignment risk are acceptable;
- inspect map files for large symbols, alignment gaps, retention, and unexpected
  library pulls.

Do not optimize size by removing bounds checks, diagnostics, or initialization without
proving the safety and product impact. A few kilobytes saved in a boot image can be
consumed by a larger stack, extra retries, or a recovery feature later.

## Compiler Optimization Strategy

Treat optimization flags as part of the program contract. Important choices include:

- C dialect and strictness flags;
- `-O` level and target CPU/ISA options;
- inlining, vectorization, floating-point contraction, and fast-math policy;
- LTO/whole-program assumptions and visibility;
- section garbage collection and linker relaxation;
- debug, unwind, profiling, sanitizer, and hardening settings;
- PGO training corpus and profile validity.

Build and link all objects with compatible ABI-changing options. Keep a reproducible
release command line and compare map/disassembly output when compiler versions change.
An optimization that is valid for a hosted process may be invalid for MMIO, a signal,
an interrupt entry, or a foreign callback unless the interface is annotated correctly.

## Profiling And Performance Counters

Sampling profilers are low overhead and show where execution spends time; tracing gives
sequence and latency detail at a higher cost. Hardware counters can expose cache,
branch, issue, stall, bus, and energy behavior, but their event meanings vary by core.
Pin down the counter configuration, multiplexing, privilege mode, and interrupt impact.

Instrumenting every function can perturb cache and timing. Use statistically meaningful
sampling, selective tracepoints, GPIO markers, ETM/trace hardware, or a low-overhead
cycle counter depending on the requirement. Correlate a measurement with source,
disassembly, and system state rather than attributing a counter to one function without
checking call/interrupt context.

## Energy-Aware Optimization

Energy is roughly the integral of power over time. A faster algorithm can save energy
by finishing sooner, or consume more by using wider SIMD, higher frequency, or more
memory traffic. Measure active, idle, wake-up, DMA, flash, and radio/peripheral costs.

For MCUs, consider clock/power states, sleep entry latency, batching, interrupt rate,
flash wait states, and memory placement. For Linux-class systems, consider CPU
frequency governors, cache coherency, page faults, scheduler wakeups, and device power
domains. Optimize the complete duty cycle, not only the active loop.

## Worst-Case Execution Time

Average profiling cannot prove a deadline. WCET analysis combines measurement, control-
flow analysis, hardware timing models, and conservative bounds. C features that make
WCET harder include unbounded loops, recursion, dynamic allocation, data-dependent
cache misses, interrupts, preemption, shared buses, and opaque library calls.

For a bounded path:

- define loop bounds and assert or reject invalid inputs;
- use fixed-capacity data structures;
- bound retries, polling, and error recovery;
- account for preemption and higher-priority interrupts;
- control cache/flash/TCM placement where required;
- measure on worst supported clock, temperature, and contention conditions;
- keep a margin and document assumptions.

Do not call a path “real-time” because its average is low. A timeout loop that depends
on a peripheral changing state needs a maximum wait and a failure action.

## Latency Budgets And Jitter

Break end-to-end latency into acquisition, interrupt entry, scheduling, queueing,
processing, output, and observation. The sum of nominal values is not enough: queue
depth, interrupt masking, cache misses, lock contention, DMA ownership, and retries can
create tail latency.

Record the longest interrupt-disabled interval and the longest lock hold. Use priority
inheritance/ceiling or a different ownership model when priority inversion matters.
Define whether a late result is dropped, applied to the next cycle, or triggers a fault.

## Binary And Memory Budgets

Inspect the final image, not only compiler object sizes. Track:

- load and virtual addresses, section sizes, alignment gaps, and padding;
- initialized data copied to RAM and zero-initialized data;
- stacks, heaps, pools, retained regions, DMA buffers, and guard bands;
- dynamic dependencies, relocations, TLS, unwind, and debug sections;
- bootloader, signing, compression, and update-slot overhead.

Make budget failures automatic in CI. A map-file parser, `size`/`readelf` report,
linker assertions, stack watermark, and runtime high-water telemetry complement one
another. A link that succeeds is not proof that runtime stacks and buffers fit.

## Optimization Workflow

1. State the user/system budget and baseline.
2. Profile representative and worst-case workloads.
3. Form one hypothesis tied to evidence.
4. Make the smallest change that preserves the contract.
5. Run correctness, race, sanitizer, and boundary tests.
6. Re-measure speed, size, memory, energy, and latency distributions.
7. Inspect generated code and map changes.
8. Keep the change only if the overall budget improves or an explicit trade-off is
   accepted and documented.

## Exercises And Diagnostics

1. Build a benchmark with cold-cache, warm-cache, and interrupt-loaded runs; report
   distributions instead of one average.
2. Compare array-of-structures and structure-of-arrays layouts for a real access pattern
   and measure memory footprint, cache misses, and total processing time.
3. Enable LTO and section garbage collection, then explain every size change using the
   map file and symbols.
4. Create a bounded WCET worksheet for a packet-processing path, including loops,
   allocation, cache, interrupts, retries, and error paths.
5. Measure energy for a polling implementation and a batched interrupt/sleep design;
   document active time, wakeups, and accuracy trade-offs.

## Common Mistakes

- Optimizing a microbenchmark that does not represent the system workload.
- Reporting a mean while the requirement is a tail or worst-case latency.
- Letting the compiler eliminate benchmark work or measuring setup/initialization by
  accident.
- Ignoring cache, DMA, bus contention, interrupts, thermal state, and power mode.
- Using `volatile` as a benchmark sink and treating the resulting code as production
  performance.
- Enabling fast-math, LTO, or target instructions without changing the correctness and
  deployment contract.
- Saving flash while increasing stack, energy, jitter, or verification burden.
- Claiming WCET bounds for paths with unbounded allocation, retries, or polling.

## Related Topics

- [Advanced C overview](./index.md)
- [Compiler Optimization And Undefined Behavior](./compiler-optimization-and-undefined-behavior.md)
- [Numerical And Fixed-Point C](./numerical-and-fixed-point-c.md)
- [Advanced Data Structures](./advanced-data-structures.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Real-Time Constraints](../embedded-c-and-hardware/real-time-constraints.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)

## References

- [GCC optimization options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [GCC instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
- [LLVM optimization remarks](https://llvm.org/docs/Remarks.html)
- [Linux perf documentation](https://docs.kernel.org/userspace-api/perf_event_open.html)
- [Arm Performance Monitor extension](https://developer.arm.com/documentation/100616/latest)
- The target core performance-monitoring guide, linker/map tooling, power measurement
  setup, real-time requirements, and release budget definition
