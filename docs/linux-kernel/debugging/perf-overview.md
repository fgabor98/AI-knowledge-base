---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Perf Overview

## What Problem Does This Solve?

Perf helps investigate CPU cost, scheduling behavior, counters, and profiling data across kernel and userspace.

Use perf when the question is about where time or CPU cycles go:

- which function burns CPU?
- is a driver polling too much?
- is an interrupt storm consuming the system?
- does a workload spend time in userspace, syscall path, or driver callbacks?
- how does a change affect CPU cost?

Do not start with perf for missing devices, failed matching, resource lookup errors, or boot configuration mistakes.

## Core Concepts

- sampling
- call graphs
- hardware counters
- software events
- scheduler events
- kernel symbols
- flame graphs
- overhead

## Mental Model

Use perf when the problem is performance or timing, not when the first problem is missing hardware resources or failed probe.

```text
logs:
  what errors were reported?

ftrace/tracepoints:
  what happened when?

perf:
  where is CPU time spent?
```

Perf samples execution. Samples are evidence of cost distribution, not direct proof of causality.

## Requirements

Kernel config and product policy affect perf:

```text
CONFIG_PERF_EVENTS
CONFIG_KALLSYMS
CONFIG_DEBUG_INFO
frame pointers or ORC/unwind support depending on architecture
perf_event_paranoid sysctl
symbol artifacts
```

Runtime checks:

```sh
cat /proc/sys/kernel/perf_event_paranoid
which perf
uname -r
```

For useful kernel call graphs, keep matching symbols:

```text
vmlinux
System.map
modules
final .config
```

## First Commands

Live top view:

```sh
sudo perf top
```

Record a short profile:

```sh
sudo perf record -g -- sleep 10
sudo perf report
```

Record while running a test:

```sh
sudo perf record -g -- ./run-demo-test
sudo perf report
```

Keep recording windows short and reproducible.

## Call Graphs

Call graphs show call paths. Quality depends on unwind support and symbols.

Common shape:

```sh
sudo perf record -g --call-graph dwarf -- ./workload
```

or:

```sh
sudo perf record -g -- ./workload
```

The best option depends on architecture, kernel config, and available debug info.

If call graphs look broken:

- check symbol availability
- check unwind support
- check stripped binaries/modules
- check whether frame pointers are enabled or another unwinder is available

## Kernel Symbols

If perf shows raw addresses or unknown symbols, check:

```sh
cat /proc/kallsyms | head
ls -l /boot/System.map-$(uname -r)
ls -l /usr/lib/debug/boot/vmlinux-$(uname -r)
```

For embedded targets, you may need to collect data on target and analyze on host with matching artifacts.

Archive the exact kernel build artifacts with releases.

## Driver CPU Burn Example

Symptom:

```text
CPU usage high when device is idle
```

First checks:

```sh
top
cat /proc/interrupts | grep -i demo
```

Perf:

```sh
sudo perf record -g -- sleep 10
sudo perf report
```

Possible findings:

- tight polling loop in workqueue
- interrupt handler firing continuously
- timer interval too small
- userspace repeatedly calling ioctl/read
- lock contention causing spin

Use perf to find where time is spent, then use logs/tracing to explain why that path is active.

## Interrupt And Scheduler Events

Perf can record tracepoint events too:

```sh
sudo perf list 'irq:*'
sudo perf record -e irq:irq_handler_entry -e irq:irq_handler_exit -- sleep 5
sudo perf script
```

Scheduler events:

```sh
sudo perf record -e sched:sched_switch -- sleep 5
sudo perf script
```

For detailed event timelines, `trace-cmd` or ftrace may be more convenient. For CPU cost and sampling, perf is a better fit.

## Counting Events

Use `perf stat` for counters:

```sh
sudo perf stat -- ./run-demo-test
```

Example with repeated runs:

```sh
sudo perf stat -r 5 -- ./run-demo-test
```

Counters depend on hardware and permissions. Interpret them carefully on heterogeneous SoCs and virtualized systems.

## Comparing Before And After

Make performance debugging reproducible:

```text
same kernel profile
same command line
same CPU governor
same workload
same duration
same thermal state if possible
same symbol artifacts
```

Record both:

```sh
sudo perf record -o before.data -g -- ./workload
sudo perf record -o after.data -g -- ./workload
```

Then compare reports.

## Perf And Driver Development

Good perf questions:

- did moving work from polling to IRQ reduce CPU usage?
- is a buffer copy dominating the read path?
- is DMA completion handling expensive?
- is a lock contention path hot?
- is userspace causing excessive syscalls?

Bad perf questions:

- why did `platform_get_irq()` return `-EINVAL`?
- why did Device Tree matching fail?
- why did the module signature fail?

Classify first.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no kernel symbols | missing `vmlinux`/kallsyms/debug info | symbol artifacts |
| poor call graphs | unwinder/frame pointer/debug info issue | call graph mode |
| permission denied | `perf_event_paranoid` or LSM policy | sysctl and security policy |
| overhead too high | recording too much data | shorter window, fewer events |
| misleading conclusion | samples treated as causality | corroborate with trace/logs |
| target storage fills | perf data too large | output path and duration |

## Practice Exercises

### Exercise 1: Idle Profile

Record 10 seconds while the system is idle:

```sh
sudo perf record -g -- sleep 10
sudo perf report
```

Identify whether your driver appears at all.

### Exercise 2: Workload Profile

Run a driver workload under perf and identify the top kernel and userspace costs.

### Exercise 3: Interrupt Storm Check

Combine:

```sh
cat /proc/interrupts
sudo perf top
```

Decide whether CPU cost aligns with interrupt activity.

## Debugging Checklist

- Confirm symbol availability.
- Check sampling overhead.
- Record the workload and duration.
- Do not infer causality from samples alone.
- Keep target and analysis artifacts matched.
- Use ftrace/tracepoints for ordering questions.
- Check permissions and `perf_event_paranoid`.
- Compare profiles under controlled conditions.

## Related Topics

- [Ftrace And Tracepoints](ftrace-and-tracepoints.md)
- [Debugging](../../debugging/index.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [Kernel Release Artifacts](../../build-systems/advanced/linux-kernel/kernel-release-artifacts.md)

## Official References

- [perf: Linux profiling with performance counters](https://perf.wiki.kernel.org/)
- [Linux Tracing Technologies](https://docs.kernel.org/trace/index.html)
