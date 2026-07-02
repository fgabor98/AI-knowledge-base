---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Debugging Basics

This track covers the first debugging tools and habits for kernel and driver work.

It assumes you already know:

- [Kernel Foundations For Driver Developers](../foundations/index.md)
- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)

## What Problem Does This Solve?

Kernel debugging fails when evidence is collected in the wrong order. A missing `/dev` node, a probe deferral, an interrupt storm, a use-after-free, and a watchdog reset need different first commands.

This chapter builds a practical debugging workflow:

- preserve the first failure evidence
- classify the failure phase
- inspect kernel objects before changing code
- use targeted logs instead of log floods
- trace timing and callback order when logs are insufficient
- use profiling for CPU-cost questions
- use KGDB and crash analysis only when lighter tools cannot answer the question

## Learning Materials

1. [Dmesg And Log Levels](dmesg-and-log-levels.md)
2. [Dynamic Debug](dynamic-debug.md)
3. [Ftrace And Tracepoints](ftrace-and-tracepoints.md)
4. [Perf Overview](perf-overview.md)
5. [Debugfs And Sysfs Inspection](debugfs-and-sysfs-inspection.md)
6. [KGDB Basics](kgdb-basics.md)
7. [Oops, Panic, And Crash Logs](oops-panic-crash-logs.md)
8. [Watchdog Reset Diagnosis](watchdog-reset-diagnosis.md)
9. [Probe Failure Debugging](probe-failure-debugging.md)

## Mental Model

Kernel debugging starts with observable state: boot logs, driver probe logs, runtime filesystems, trace data, and crash evidence. Use stronger tools only after the failure is classified.

```text
classify failure
-> preserve first evidence
-> inspect runtime state
-> enable targeted logs
-> trace ordering/timing
-> profile CPU cost if relevant
-> use heavy tools for remaining unknowns
```

Each tool should answer a question:

| Question | First Tools |
| --- | --- |
| Did the kernel report an error? | `dmesg`, serial log, `journalctl -k` |
| Did the device exist? | `/sys/bus/.../devices`, runtime Device Tree |
| Did the driver bind? | sysfs driver links, modalias, module aliases |
| Did probe fail or defer? | logs, `devices_deferred`, provider debugfs |
| Did an interrupt fire? | `/proc/interrupts`, irq tracepoints |
| Did a callback run? | dynamic debug, ftrace |
| Where is CPU time spent? | `perf` |
| Why did it crash? | full oops, symbols, taint flags |
| Why did it reset? | reset reason, previous logs, watchdog owner |

## Debugging Ladder

Use the smallest tool that can answer the current question.

```text
1. classify the failure
2. capture boot/runtime logs
3. inspect sysfs/proc/debugfs state
4. enable dynamic debug
5. capture tracepoints or ftrace
6. profile with perf
7. enable debug configs and sanitizers
8. use KGDB or crash dumps
```

Jumping straight to KGDB for a missing Device Tree node wastes time. Running perf for a probe error wastes time. Treat tool choice as part of the diagnosis.

## Evidence Rules

For every bug report, try to keep:

- exact kernel version and commit
- final `.config`
- command line from `/proc/cmdline`
- full boot log from power-on
- first error message
- module list
- relevant sysfs/debugfs state
- reproduction steps
- whether the kernel is tainted
- whether debug configs or out-of-tree modules were used

The first error is often more valuable than the final crash. Later messages may be secondary damage.

## Common Mistakes

- Editing code before identifying the failure phase.
- Looking only at logs after login instead of from power-on.
- Losing the first oops or warning.
- Debugging `/dev` before checking whether probe succeeded.
- Ignoring exact negative error codes.
- Adding broad `dev_info()` logs in hot paths.
- Using debugfs as product ABI.
- Enabling broad function tracing without filters.
- Treating a watchdog reset as a root cause.
- Forgetting that debug configs change timing.

## Completion Criteria

You are ready to move on when you can:

- read and filter kernel logs effectively
- preserve full boot logs and previous-boot crash evidence
- distinguish log levels and choose `dev_*()` versus `pr_*()`
- enable and disable dynamic debug for a file, function, or module
- capture a short ftrace or tracepoint trace with filters
- use perf for CPU-cost and sampling questions
- inspect device, driver, module, IRQ, sysfs, and debugfs state
- explain when KGDB is appropriate and when it is too intrusive
- decode the basic structure of an oops or panic log
- classify probe failures, deferred probes, and watchdog resets
- map the next debugging step to a specific question

## Related Topics

- [Debugging And Diagnostics](../../debugging/index.md)
- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [Failure Taxonomy](../foundations/failure-taxonomy.md)
- [Debugging Ladder](../foundations/debugging-ladder.md)

## Official References

- [Message logging with printk](https://docs.kernel.org/core-api/printk-basics.html)
- [Dynamic debug](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
- [ftrace](https://docs.kernel.org/trace/ftrace.html)
- [Bug hunting](https://docs.kernel.org/admin-guide/bug-hunting.html)
