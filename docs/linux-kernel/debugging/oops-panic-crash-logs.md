---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Oops, Panic, And Crash Logs

## What Problem Does This Solve?

Oops and panic logs provide evidence for kernel faults such as invalid memory access, BUG checks, lockups, and fatal exceptions.

## Core Concepts

- oops
- panic
- stack trace
- program counter
- fault address
- taint flags
- symbolization
- persistent storage
- crash dump overview

## Mental Model

Treat the first fault as the primary evidence. Later stack traces may be secondary damage.

## Practice Skeleton

- Capture a full oops log.
- Decode the faulting symbol.
- Identify taint flags.
- Map the fault to source when symbols are available.

## Debugging Checklist

- Preserve full serial logs.
- Check the first exception.
- Match logs to exact kernel build artifacts.
- Check whether the crash follows teardown, interrupt, or userspace entry paths.

## Related Topics

- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)
- [Kernel Release Artifacts](../../build-systems/advanced/linux-kernel/kernel-release-artifacts.md)
- [Watchdog Reset Diagnosis](watchdog-reset-diagnosis.md)
