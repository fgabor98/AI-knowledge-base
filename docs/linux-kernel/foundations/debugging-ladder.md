---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Debugging Ladder

## What Problem Does This Solve?

Kernel debugging tools vary in cost, complexity, and runtime impact. Beginners often jump to heavy tools before collecting basic evidence. The debugging ladder gives an ordered approach: start with cheap, stable evidence, then move to more intrusive tools only when they answer a question the lower level cannot answer.

## Core Concepts

- `dmesg`
- `dev_*` logging
- dynamic debug
- sysfs inspection
- debugfs inspection
- ftrace
- tracepoints
- perf
- lockdep
- KASAN
- KGDB
- crash dumps
- kernel taint flags

## The Ladder

```text
1. classify the failure
2. read kernel logs
3. inspect module/device/sysfs state
4. add or enable targeted debug logs
5. inspect debugfs and subsystem state
6. capture traces
7. profile or sample
8. enable debug configs and sanitizers
9. use interactive debugging or crash analysis
```

Each step should answer a specific question.

## Step 1: Classify The Failure

Before collecting tools, classify:

- build failure
- module load failure
- missing device
- match/bind failure
- probe failure
- runtime I/O failure
- interrupt failure
- teardown failure
- suspend/resume failure
- oops
- hang
- watchdog reset

Example:

```text
"No /dev node" is not enough.
Better: "probe succeeded, class device registration did not create expected userspace node."
```

Use [Failure Taxonomy](failure-taxonomy.md) for detail.

## Step 2: Kernel Logs

Commands:

```bash
dmesg --time-format=iso
dmesg --follow
journalctl -k -b
journalctl -k -f
```

Useful filters:

```bash
dmesg | grep -i demo
dmesg | grep -E "probe|defer|fail|error|irq|timeout"
journalctl -k -b -g demo
```

Add device-scoped logs:

```c
dev_info(&pdev->dev, "probe started\n");
dev_err(&pdev->dev, "failed to read status: %d\n", ret);
```

Prefer `dev_err_probe` for probe paths that may defer:

```c
return dev_err_probe(&pdev->dev, ret, "failed to get regulator\n");
```

Why `dev_*` matters:

```text
demo 48000000.demo: failed to get regulator: -517
```

The log carries device identity.

## Kernel Taint Flags

When reading crash logs or bug reports, check whether the kernel is tainted. Taint flags record conditions that may affect supportability or debugging confidence, such as proprietary modules, forced module loading, warnings, or out-of-tree code.

Check runtime taint:

```bash
cat /proc/sys/kernel/tainted
```

In logs, taint often appears near warnings or oops reports:

```text
CPU: 0 PID: 1234 Comm: test Tainted: G           O       6.6.0-custom #1
```

Practical use:

- if an out-of-tree module is loaded, note it in the bug report
- if a warning tainted the kernel earlier, preserve the first warning
- if forced module loading was used, distrust later behavior until reproduced cleanly

Taint does not automatically identify the bug, but it is important context for interpreting kernel evidence.

## Step 3: Module And Device State

Module state:

```bash
lsmod | grep demo
modinfo ./demo.ko
cat /sys/module/demo/refcnt
find /sys/module/demo -maxdepth 2 -type f | sort
```

Platform device state:

```bash
find /sys/bus/platform/devices -maxdepth 1 -name '*demo*' -print
readlink /sys/bus/platform/devices/48000000.demo/driver
cat /sys/bus/platform/devices/48000000.demo/modalias
```

Driver state:

```bash
find /sys/bus/platform/drivers/demo -maxdepth 1 -type l -print
cat /sys/bus/platform/drivers/demo/uevent 2>/dev/null
```

Character device state:

```bash
ls -l /dev/demo*
udevadm info /dev/demo0
cat /proc/devices | grep demo
```

IRQ state:

```bash
cat /proc/interrupts | grep -i demo
```

These commands often reveal that the problem is not in the runtime callback at all; the device never bound or the userspace node was never created.

## Step 4: Dynamic Debug

Dynamic debug can enable compiled `pr_debug` and `dev_dbg` call sites at runtime.

Check support:

```bash
mount | grep debugfs
ls /sys/kernel/debug/dynamic_debug/control
```

Mount debugfs if appropriate in a lab:

```bash
sudo mount -t debugfs none /sys/kernel/debug
```

Enable debug for a module:

```bash
echo 'module demo +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Enable debug for one file:

```bash
echo 'file drivers/iio/adc/demo.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Disable:

```bash
echo 'module demo -p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Good use:

```c
dev_dbg(dev, "status register %#x\n", status);
```

Bad use:

```c
dev_dbg(dev, "loop\n");  /* floods logs in a hot path */
```

## Step 5: sysfs And debugfs

sysfs is for kernel object state and stable-ish ABI:

```bash
find /sys/bus/platform/devices/48000000.demo -maxdepth 2 -type f -print
cat /sys/bus/platform/devices/48000000.demo/uevent
```

debugfs is for diagnostics and is not a stable product ABI:

```bash
ls /sys/kernel/debug
cat /sys/kernel/debug/gpio
cat /sys/kernel/debug/clk/clk_summary
cat /sys/kernel/debug/regulator/regulator_summary
```

Use debugfs to answer questions such as:

- is a GPIO requested?
- is a clock enabled?
- is a regulator on?
- does regmap expose cached registers?
- are trace events available?

Do not design product behavior that depends on debugfs.

## Step 6: ftrace And Tracepoints

Tracing answers "what happened when?"

Manual function trace example:

```bash
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo function | sudo tee current_tracer
echo demo_* | sudo tee set_ftrace_filter
echo 1 | sudo tee tracing_on
# run test
echo 0 | sudo tee tracing_on
sudo cat trace
```

Trace events:

```bash
cd /sys/kernel/tracing
sudo cat available_events | grep irq
echo 1 | sudo tee events/irq/irq_handler_entry/enable
echo 1 | sudo tee events/irq/irq_handler_exit/enable
echo 1 | sudo tee tracing_on
# trigger interrupt
sudo cat trace
```

With `trace-cmd`:

```bash
sudo trace-cmd record -e irq -e workqueue sleep 5
sudo trace-cmd report
```

Use tracing when logs cannot show ordering, timing, or callback paths.

## Step 7: perf

Use perf for performance and sampling questions:

```bash
sudo perf top
sudo perf record -g -- sleep 10
sudo perf report
```

Good questions for perf:

- where is CPU time spent?
- is a driver polling too much?
- is an interrupt storm consuming CPU?
- what call path dominates a workload?

Bad questions for perf:

- why did `probe` fail?
- why is the Device Tree node missing?
- why did module loading fail?

Use the right tool for the failure class.

## Step 8: Debug Configs And Sanitizers

Development kernels can enable stronger checks:

- lockdep for locking problems
- KASAN for memory bugs
- KMSAN for uninitialized memory bugs where available
- UBSAN for undefined behavior checks
- DEBUG_ATOMIC_SLEEP for sleep-in-atomic warnings
- kmemleak for leaks
- dynamic debug and ftrace support

Example symptom:

```text
BUG: sleeping function called from invalid context
```

This points toward execution context misuse, often sleeping while holding a spinlock or inside hard IRQ context.

## Step 9: KGDB And Crash Dumps

Use heavier tools after lower levels identify a reason.

KGDB can inspect a live kernel but changes timing and requires setup.

Crash dump analysis can inspect state after a panic but requires configured capture infrastructure.

Use for:

- hard-to-localize crashes
- complex state inspection
- postmortem analysis
- failures that cannot be reproduced with simpler instrumentation

Do not make KGDB the first debugging habit.

## Examples

### Example: Probe Fails With `-EPROBE_DEFER`

Log:

```text
demo 48000000.demo: failed to get vdd: -517
```

Ladder:

```bash
dmesg | grep demo
find /sys/bus/platform/devices -name '*demo*'
cat /sys/kernel/debug/regulator/regulator_summary
```

Likely cause:

- regulator provider did not probe yet
- Device Tree supply name is wrong
- regulator driver missing from config

Next:

- inspect Device Tree binding
- check provider node status
- check final `.config`

### Example: IRQ Handler Never Runs

Evidence:

```bash
cat /proc/interrupts | grep demo
```

Ladder:

- verify Device Tree interrupt specifier
- verify interrupt controller probed
- check pinmux and electrical signal
- enable IRQ tracepoints
- add a log in threaded handler if hard handler fires

Trace:

```bash
sudo trace-cmd record -e irq sleep 5
sudo trace-cmd report | grep demo
```

### Example: Character Device Read Blocks Forever

Evidence:

```bash
cat /dev/demo0
```

hangs.

Ladder:

```bash
ps -eLo pid,tid,comm,state,wchan:30 | grep demo
dmesg | tail -100
cat /proc/interrupts | grep demo
```

Questions:

- is the wait condition ever set?
- is `wake_up` called?
- is the interrupt firing?
- is the file opened in blocking or nonblocking mode?

## Common Mistakes

- Adding broad logs before classifying the failure.
- Ignoring the first error and focusing on later fallout.
- Using debugfs as product ABI.
- Enabling broad function tracing and drowning in output.
- Running perf for probe failures.
- Using KGDB before confirming module/device/probe state.
- Forgetting that debug configs can change timing.

## Debugging Checklist

- What specific question am I answering?
- What is the cheapest tool that can answer it?
- Did I preserve the first failure evidence?
- Is the issue build, load, match, probe, resource, runtime, teardown, or crash?
- Do I need state, ordering, timing, or CPU cost evidence?
- Will the tool change the timing or behavior?

## Related Topics

- [Failure Taxonomy](failure-taxonomy.md)
- [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)
- [Dynamic Debug](../debugging/dynamic-debug.md)
- [Ftrace And Tracepoints](../debugging/ftrace-and-tracepoints.md)
- [Perf Overview](../debugging/perf-overview.md)

## Official References

- Dynamic debug HOWTO: <https://docs.kernel.org/admin-guide/dynamic-debug-howto.html>
- ftrace documentation: <https://docs.kernel.org/trace/ftrace.html>
- Linux tracing documentation: <https://docs.kernel.org/trace/index.html>
- Kernel hacking guides: <https://docs.kernel.org/kernel-hacking/index.html>
