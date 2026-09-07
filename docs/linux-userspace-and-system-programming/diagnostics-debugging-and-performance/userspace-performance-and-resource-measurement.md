---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Userspace Performance And Resource Measurement

## Measure the right quantity

Separate wall time, CPU time, scheduler wait, I/O wait, allocation time, and device latency. Report throughput and distributions, especially p95/p99 or deadline misses; an average can hide a single unacceptable stall. Define workload, input size, warm-up, cache state, CPU frequency policy, and concurrency before comparing versions.

Useful first measurements include `time`, `getrusage()`, `/proc` counters, cgroup memory/CPU events, `strace -T -c`, and `perf stat`. Sampling profilers such as `perf record` can identify hot code with less distortion than extensive tracing.

## Resource boundaries

Track resident memory, peak allocation, file descriptors, threads, processes, dirty pages, I/O bytes, context switches, and pressure stall information. A memory leak is not the only memory failure: fragmentation, page cache growth, a cgroup limit, or a single oversized input can look similar.

## Experiment quality

Repeat enough runs to show variance, pin down environmental differences, and avoid optimizing instrumentation overhead. Change one variable, preserve a baseline, and add a benchmark or production metric only when it represents a real requirement. Performance fixes must retain correctness, cancellation, backpressure, and bounded resource use.

## Turn a latency claim into a measurement

Define boundaries such as receive complete request, admit work, begin device
operation, finish operation, and send complete response. Timestamp each with
one monotonic clock. Queue delay, service time, and output delay then become
separate quantities rather than one unexplained total.

For a hypothetical single worker taking 5 ms per request, sustained capacity
cannot exceed about 200 requests/s before overhead. A burst of 100 requests
can create roughly half a second of work. A larger queue changes how much
work waits, not the service rate. Use this estimate to choose an admission
limit and then measure realistic distributions.

An open-loop load generator maintains offered arrivals even when replies slow.
A closed-loop generator waits for replies and may hide overload by reducing
the offered rate. Report which model was used, dropped/rejected work, sample
count, percentiles, and deadline misses. The largest observed latency is not
a proven upper bound.

## Attribute resource cost

RSS counts shared pages in every process; PSS apportions them. A growing heap
after requests stop may be a leak or allocator retention. Compare live
allocations, private dirty memory, and repeated steady-state cycles before
classifying it.

Use `perf stat` for counters when available, sampled stacks for CPU paths,
and cgroup pressure/throttling for waiting caused by resource policy. PSI
reports time tasks were stalled; it is not bytes of I/O or percentage CPU
utilization. See [PSI](https://docs.kernel.org/accounting/psi.html).

Re-run the representative workload after a change with diagnostics at normal
production levels. Improving average throughput while delaying shutdown or
increasing the worst queue age can violate the original requirement.

## Related topics

- [Stage 14 overview](index.md)
- [Testing and verification](../testing-and-verification/index.md)
