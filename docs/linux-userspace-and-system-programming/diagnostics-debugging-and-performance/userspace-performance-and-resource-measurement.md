# Userspace Performance And Resource Measurement

## Measure the right quantity

Separate wall time, CPU time, scheduler wait, I/O wait, allocation time, and device latency. Report throughput and distributions, especially p95/p99 or deadline misses; an average can hide a single unacceptable stall. Define workload, input size, warm-up, cache state, CPU frequency policy, and concurrency before comparing versions.

Useful first measurements include `time`, `getrusage()`, `/proc` counters, cgroup memory/CPU events, `strace -T -c`, and `perf stat`. Sampling profilers such as `perf record` can identify hot code with less distortion than extensive tracing.

## Resource boundaries

Track resident memory, peak allocation, file descriptors, threads, processes, dirty pages, I/O bytes, context switches, and pressure stall information. A memory leak is not the only memory failure: fragmentation, page cache growth, a cgroup limit, or a single oversized input can look similar.

## Experiment quality

Repeat enough runs to show variance, pin down environmental differences, and avoid optimizing instrumentation overhead. Change one variable, preserve a baseline, and add a benchmark or production metric only when it represents a real requirement. Performance fixes must retain correctness, cancellation, backpressure, and bounded resource use.
