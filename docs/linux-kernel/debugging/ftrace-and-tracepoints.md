---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Ftrace And Tracepoints

## What Problem Does This Solve?

Ftrace and tracepoints capture runtime execution and timing evidence without relying only on logs.

They answer questions logs are bad at:

- which callback ran first?
- how long did a function take?
- did an IRQ fire before work was queued?
- did a workqueue callback run after remove began?
- is a driver polling too often?
- which scheduler or IRQ events surrounded the failure?

## Core Concepts

- tracing filesystem
- function tracing
- function graph tracing
- trace events
- tracepoints
- filters
- trace markers
- trace-cmd

## Mental Model

Tracing answers "what happened when?" Logs explain selected states; traces reveal execution order, timing, and callback paths.

```text
logs:
  selected driver messages

trace:
  timestamped execution and event stream
```

Use filters before enabling broad tracing. A useful trace is small enough to inspect.

## Tracing Filesystem

The tracing filesystem is commonly mounted at:

```text
/sys/kernel/tracing
```

or older paths:

```text
/sys/kernel/debug/tracing
```

Check:

```sh
mount | grep tracing
ls /sys/kernel/tracing
```

Mount in a lab if needed:

```sh
sudo mount -t tracefs nodev /sys/kernel/tracing
```

If tracefs is under debugfs on a target, debugfs policy controls access.

## Basic Safe Pattern

Use this shape:

```sh
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo nop | sudo tee current_tracer
echo > trace
# configure filters/events
echo 1 | sudo tee tracing_on
# run small test
echo 0 | sudo tee tracing_on
sudo cat trace > /tmp/trace.txt
```

Keep tracing windows short. Configure first, then enable, then run the test, then disable.

## Function Tracing

Function tracing records function entries.

Example:

```sh
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo function | sudo tee current_tracer
echo 'demo_*' | sudo tee set_ftrace_filter
echo > trace
echo 1 | sudo tee tracing_on
# run workload
echo 0 | sudo tee tracing_on
sudo cat trace
```

Use function filters. Tracing every kernel function usually creates too much data and overhead.

Available functions:

```sh
grep demo available_filter_functions
```

## Function Graph Tracing

Function graph tracing records entry, exit, and duration.

```sh
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo function_graph | sudo tee current_tracer
echo 'demo_*' | sudo tee set_graph_function
echo > trace
echo 1 | sudo tee tracing_on
# run workload
echo 0 | sudo tee tracing_on
sudo cat trace
```

Use this for:

- slow probe
- long IRQ thread
- slow sysfs operation
- unexpected sleepable path duration

Do not use function graph tracing blindly on timing-sensitive systems without measuring overhead.

## Trace Events And Tracepoints

Trace events expose structured kernel events.

List events:

```sh
cd /sys/kernel/tracing
cat available_events | grep irq
cat available_events | grep workqueue
```

Enable IRQ events:

```sh
echo 1 | sudo tee events/irq/irq_handler_entry/enable
echo 1 | sudo tee events/irq/irq_handler_exit/enable
```

Enable workqueue events:

```sh
echo 1 | sudo tee events/workqueue/workqueue_queue_work/enable
echo 1 | sudo tee events/workqueue/workqueue_execute_start/enable
echo 1 | sudo tee events/workqueue/workqueue_execute_end/enable
```

Capture:

```sh
echo > trace
echo 1 | sudo tee tracing_on
# run test
echo 0 | sudo tee tracing_on
sudo cat trace
```

Tracepoints are usually more stable and lower-volume than broad function tracing.

## Event Filters

Many events support filters.

Inspect format:

```sh
cat events/irq/irq_handler_entry/format
```

Set filter:

```sh
echo 'irq == 42' | sudo tee events/irq/irq_handler_entry/filter
```

Clear filter:

```sh
echo 0 | sudo tee events/irq/irq_handler_entry/filter
```

Filtering at the event source keeps traces small.

## Trace Markers

Userspace can insert markers into the trace:

```sh
echo "starting demo test" | sudo tee /sys/kernel/tracing/trace_marker
```

Use markers to correlate a userspace command with kernel events.

Example:

```sh
echo "before read" | sudo tee /sys/kernel/tracing/trace_marker
cat /dev/demo0
echo "after read" | sudo tee /sys/kernel/tracing/trace_marker
```

## `trace-cmd`

`trace-cmd` wraps tracefs operations and stores traces for later review.

IRQ and workqueue capture:

```sh
sudo trace-cmd record -e irq -e workqueue sleep 5
sudo trace-cmd report
```

Function graph with filter:

```sh
sudo trace-cmd record -p function_graph -l 'demo_*' -- ./run-test
sudo trace-cmd report
```

`trace-cmd` is usually easier for repeatable captures and sharing evidence.

## Common Driver Trace Scenarios

### IRQ Does Not Fire

```sh
sudo trace-cmd record -e irq sleep 5
sudo trace-cmd report | grep -i demo
```

Also check:

```sh
cat /proc/interrupts | grep -i demo
```

### Work Runs After Remove

Enable workqueue events and driver function filter:

```sh
sudo trace-cmd record -e workqueue -p function -l 'demo_*' -- ./unbind-test
```

Look for:

```text
remove starts
work queued
work executes
driver data freed
```

### Probe Is Slow

Use function graph tracing around probe functions:

```sh
sudo trace-cmd record -p function_graph -l 'demo_*' -- ./bind-test
```

Check whether the delay is in regulator, firmware, bus transaction, or polling code.

## Tracing Overhead

Tracing changes behavior:

- logs and traces add timing overhead
- function graph tracing is heavier than event tracing
- broad filters create large buffers
- printing trace output while tracing can distort timing

Mitigation:

- filter before enabling tracing
- capture short windows
- record to buffer/file and inspect later
- repeat without tracing to confirm behavior
- document tracing configuration

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| trace is empty | tracing disabled, wrong filter, path not executed | `tracing_on`, filters |
| trace is huge | filter too broad | set function/event filters |
| target becomes slow | tracing overhead too high | reduce tracer/events |
| expected event missing | config lacks tracepoint or subsystem path not hit | `available_events` |
| timestamps do not match logs | different clock/source or missing markers | trace options and markers |
| permission denied | tracefs/debugfs policy | mount and access control |

## Practice Exercises

### Exercise 1: Filtered Function Trace

Capture only functions matching your driver prefix:

```sh
echo function | sudo tee current_tracer
echo 'demo_*' | sudo tee set_ftrace_filter
```

Run one operation and save the trace.

### Exercise 2: IRQ And Workqueue Timeline

Capture IRQ and workqueue events while triggering one hardware event. Explain the ordering.

### Exercise 3: Trace Marker Correlation

Insert trace markers before and after a userspace operation and correlate them with kernel callbacks.

## Debugging Checklist

- Keep tracing windows short.
- Use filters before enabling broad tracing.
- Check tracing overhead.
- Correlate trace timestamps with logs.
- Save the exact tracing commands.
- Prefer tracepoints for structured events.
- Use function graph tracing for duration questions.
- Disable tracing after capture.

## Related Topics

- [Perf Overview](perf-overview.md)
- [Context Rules](../execution-and-concurrency/context-rules.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [Workqueues](../execution-and-concurrency/workqueues.md)

## Official References

- [ftrace](https://docs.kernel.org/trace/ftrace.html)
- [Linux Tracing Technologies](https://docs.kernel.org/trace/index.html)
- [Trace events](https://docs.kernel.org/trace/events.html)
