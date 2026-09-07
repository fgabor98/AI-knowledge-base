---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Stage 14: Diagnostics, Debugging, And Performance

Debugging is evidence collection followed by hypothesis testing. Start with the smallest reproducible symptom, capture the process identity and environment, then choose a tool that observes the suspected boundary without changing timing so much that the problem disappears.

## A repeatable workflow

1. Preserve the exact binary, libraries, configuration, kernel, and target identity.
2. Record timestamps, exit status, signal, resource limits, and recent lifecycle events.
3. Classify the failure: input, application logic, ABI/loader, deployment, kernel/UAPI, device, or hardware.
4. Reproduce with one variable changed at a time.
5. Capture the narrowest useful trace or dump.
6. Form a falsifiable hypothesis and run a focused experiment.
7. Add a regression test, telemetry, or runbook entry after the fix.

## Tool selection

Use `strace` for syscall boundaries, `/proc` for runtime state, GDB for execution and memory, ELF tools for loader/ABI questions, and `perf` or resource counters for performance. On a target, serial logs and kernel evidence often matter more than a host reproduction.

## Completion checklist

- [ ] A crash can be correlated with a build and target identity.
- [ ] Operators can collect evidence without rebuilding the device.
- [ ] Diagnostics distinguish symptom from first failing boundary.
- [ ] Performance claims include workload, warm-up, sampling overhead, and tail latency.
- [ ] Cross-layer failures retain both userspace and kernel/device evidence.

## Stage exercise

Choose one reproducible failure and produce a compact investigation bundle:
instance identity, expected transition, first discriminating evidence,
hypothesis, controlled experiment and outcome. Use the appropriate leaf page
to choose tools, then explain why the evidence supports the conclusion.

Repeat once with a process hang and once with a loader or deployment failure.
The deliverable is a reproducible diagnosis and a regression or runbook step,
including any evidence that remained unavailable on the target.

## Learning materials

1. [Userspace Failure Taxonomy And Evidence](userspace-failure-taxonomy-and-evidence.md)
2. [Strace, Procfs, And Runtime Inspection](strace-procfs-and-runtime-inspection.md)
3. [GDB, Core Dumps, And Symbols](gdb-core-dumps-and-symbols.md)
4. [ELF, ABI, And Loader Diagnostics](elf-abi-and-loader-diagnostics.md)
5. [Userspace Performance And Resource Measurement](userspace-performance-and-resource-measurement.md)
6. [Target Hardware And Cross-Layer Debugging](target-hardware-and-cross-layer-debugging.md)
