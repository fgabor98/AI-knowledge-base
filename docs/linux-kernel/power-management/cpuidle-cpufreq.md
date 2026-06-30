---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# cpuidle And cpufreq

## What Problem Does This Solve?

`cpuidle` and `cpufreq` are CPU power-management subsystems. Driver developers
do not usually write these drivers unless they work on platform support, but
ordinary device drivers strongly affect whether CPU power management works well.

Bad driver behavior can keep CPUs awake:

```text
driver polls every 1 ms
  -> CPU wakes repeatedly
  -> deep idle states are never reached
  -> battery drain and heat increase
```

Bad latency assumptions can break workloads:

```text
driver permits deep idle during low-latency audio
  -> wakeup latency exceeds audio deadline
  -> underrun
```

Bad platform data can break scaling:

```text
OPP table has wrong voltage
  -> cpufreq requests unsupported operating point
  -> instability, throttling, or boot failure
```

This page explains enough of CPU idle and frequency scaling to reason about
driver impact, platform bring-up, and debugging.

## cpuidle Mental Model

When a CPU has no runnable task except the idle task, the kernel may ask the
processor to enter an idle state.

Conceptually:

```text
CPU becomes idle
  -> cpuidle governor predicts idle duration
  -> governor checks latency constraints
  -> cpuidle driver enters selected idle state
  -> interrupt or timer wakes CPU
```

Deeper idle states usually save more power but cost more time to enter and exit.

| cpuidle Term | Meaning |
| --- | --- |
| idle state | hardware low-power state for a CPU, core, cluster, or package |
| target residency | minimum idle duration needed for the state to save energy |
| exit latency | worst-case time to resume execution after wake |
| governor | algorithm that chooses an idle state |
| driver | platform code that enters hardware idle states |
| tickless idle | mode where the scheduler tick can stop on idle CPUs |

Example:

```text
C1:
  shallow
  low exit latency
  little power saved

C6:
  deeper
  higher exit latency
  more power saved
```

Names are architecture-specific. ARM platforms may describe CPU and cluster idle
states differently from ACPI/x86 systems.

## cpuidle Governors

The cpuidle governor chooses an idle state based on predicted idle time and
latency constraints.

Common governors include:

| Governor | Typical Role |
| --- | --- |
| `menu` | general tickless-idle governor using prediction and timers |
| `teo` | timer-events-oriented governor for tickless systems |
| `ladder` | older/simple governor often associated with non-tickless systems |
| `haltpoll` | polling-oriented idle for virtualization use cases |

Inspect:

```sh
cat /sys/devices/system/cpu/cpuidle/current_driver
cat /sys/devices/system/cpu/cpuidle/current_governor_ro
cat /sys/devices/system/cpu/cpuidle/available_governors
```

The available files depend on kernel configuration and platform support.

## Inspecting Idle States

Per-CPU idle state directories:

```sh
ls /sys/devices/system/cpu/cpu0/cpuidle
```

Common attributes:

```sh
for s in /sys/devices/system/cpu/cpu0/cpuidle/state*; do
    echo "$s"
    cat "$s/name"
    cat "$s/desc"
    cat "$s/latency"
    cat "$s/residency"
    cat "$s/usage"
    cat "$s/time"
done
```

Interpretation:

| Attribute | Meaning |
| --- | --- |
| `name` | state name |
| `desc` | platform description |
| `latency` | exit latency, usually in microseconds |
| `residency` | target residency, usually in microseconds |
| `usage` | number of times state was entered |
| `time` | cumulative time spent in the state |
| `disable` | control to disable a state, if writable |

If deep states never accumulate usage, look for frequent timers, interrupts,
polling, latency constraints, or disabled firmware/platform support.

## How Device Drivers Affect cpuidle

Drivers influence idle mostly by creating wakeups or constraints.

### Polling

Polling prevents long idle intervals.

Bad:

```c
while (!demo_ready(priv))
    msleep(1);
```

Better when hardware supports interrupts:

```c
ret = wait_for_completion_timeout(&priv->done,
                                  msecs_to_jiffies(100));
if (!ret)
    return -ETIMEDOUT;
```

For register polling during a short hardware transition, use appropriate kernel
polling helpers and datasheet timeouts:

```c
ret = readl_poll_timeout(priv->base + DEMO_STATUS, val,
                         val & DEMO_READY, 10, 10000);
```

This is acceptable for bounded hardware bring-up. It is not a replacement for an
interrupt-driven runtime design.

### Timers

Frequent timers wake CPUs even when no useful work exists.

Bad:

```c
schedule_delayed_work(&priv->poll_work, msecs_to_jiffies(10));
```

Better:

```c
schedule_delayed_work(&priv->poll_work, msecs_to_jiffies(1000));
```

Best, if hardware supports it:

```text
device interrupt
  -> threaded IRQ
     -> handle event
```

Use polling only when hardware has no event mechanism or when the subsystem
requires it.

### IRQ Storms

An interrupt that fires repeatedly keeps CPUs out of idle.

Evidence:

```sh
watch -n1 cat /proc/interrupts
```

Typical causes:

- level interrupt status not cleared
- wrong interrupt trigger type in firmware
- device interrupt unmasked before status is initialized
- wake IRQ enabled during runtime when it should be masked
- shared IRQ handler returning `IRQ_HANDLED` for unrelated events

### PM QoS Latency Constraints

Latency-sensitive drivers or subsystems may request limits on CPU or device
resume latency. Such constraints can prevent deep idle states.

Conceptual example:

```text
audio stream starts
  -> low latency required
  -> deep CPU idle states may be avoided

audio stream stops
  -> constraint removed
  -> deeper idle states may be used again
```

Use PM QoS only when the device or subsystem has a real latency requirement.
Leaking a constraint is like leaking power.

Driver review question:

```text
Does this driver request a latency constraint?
If yes, is it removed on stop, error, suspend, and remove?
```

## cpufreq Mental Model

`cpufreq` changes CPU operating performance points while the system runs.

Conceptually:

```text
scheduler observes utilization
  -> governor chooses performance level
  -> cpufreq driver programs hardware
  -> CPU runs at new frequency/voltage point
```

Terms:

| cpufreq Term | Meaning |
| --- | --- |
| P-state | CPU performance state, often frequency/voltage |
| OPP | operating performance point, commonly frequency plus voltage |
| policy | group of CPUs sharing frequency control |
| governor | algorithm that selects frequency |
| scaling driver | platform driver that programs hardware |
| transition latency | time needed to change frequency |

Many platforms cannot scale each logical CPU independently. A policy may cover a
cluster or package:

```text
policy0:
  cpu0 cpu1 cpu2 cpu3

policy4:
  cpu4 cpu5 cpu6 cpu7
```

Changing one CPU's policy can affect all CPUs in that policy.

## cpufreq Sysfs

Policy directories:

```sh
ls /sys/devices/system/cpu/cpufreq
ls /sys/devices/system/cpu/cpufreq/policy0
```

Useful files:

```sh
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_driver
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_governors
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies
cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq
cat /sys/devices/system/cpu/cpufreq/policy0/cpuinfo_transition_latency
cat /sys/devices/system/cpu/cpufreq/policy0/affected_cpus
```

Change governor in a lab:

```sh
echo performance | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
echo schedutil | sudo tee /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
```

Do not tune production systems by copying lab commands blindly. Thermal policy,
firmware, power supply limits, workload latency, and distribution policy all
matter.

## cpufreq Governors

Common governors:

| Governor | Behavior |
| --- | --- |
| `performance` | requests highest allowed frequency |
| `powersave` | requests lowest allowed frequency |
| `userspace` | lets userspace request frequency through sysfs |
| `schedutil` | uses scheduler utilization signals |
| `ondemand` | older load-sampling governor |
| `conservative` | older gradual-scaling governor |

On many current systems, `schedutil` is the normal governor because it uses
scheduler utilization data. Some hardware drivers, such as platform-specific
P-state drivers, may bypass generic governor behavior or expose driver-specific
policy controls.

## OPP Tables And Voltage Scaling

Embedded platforms often describe CPU operating points in firmware:

```dts
cpu0_opp_table: opp-table-cpu {
    compatible = "operating-points-v2";

    opp-1000000000 {
        opp-hz = /bits/ 64 <1000000000>;
        opp-microvolt = <900000>;
    };

    opp-1500000000 {
        opp-hz = /bits/ 64 <1500000000>;
        opp-microvolt = <1050000>;
    };
};
```

CPU node:

```dts
cpu@0 {
    device_type = "cpu";
    compatible = "arm,cortex-a53";
    reg = <0>;
    operating-points-v2 = <&cpu0_opp_table>;
    clocks = <&clk CPU_CLK>;
    cpu-supply = <&vdd_cpu>;
};
```

The cpufreq driver uses this data with clock and regulator providers to move
between operating points.

Wrong OPP data can cause:

- boot instability
- random crashes under load
- thermal throttling that appears too early or too late
- undervoltage at high frequency
- excessive power at low workload

When bringing up a board, validate OPPs against the SoC datasheet, speed grade,
PMIC constraints, and board power design.

## Thermal And Power Limits

cpufreq does not act alone. Frequency may be limited by:

- thermal framework cooling devices
- firmware power limits
- battery or charger state
- regulator current limits
- platform service processor policy
- cgroup or userspace policy
- BIOS/UEFI restrictions

Symptom:

```text
scaling_governor = performance
scaling_max_freq = high
scaling_cur_freq = lower than expected
```

Possible explanation:

```text
thermal zone is hot
  -> cooling device limits policy max frequency
  -> cpufreq cannot request highest OPP
```

Check thermal zones:

```sh
for z in /sys/class/thermal/thermal_zone*; do
    echo "$z"
    cat "$z/type"
    cat "$z/temp"
done
```

Check cooling devices:

```sh
for c in /sys/class/thermal/cooling_device*; do
    echo "$c"
    cat "$c/type"
    cat "$c/cur_state"
    cat "$c/max_state"
done
```

## Scheduler Interaction

CPU frequency selection is tightly coupled with scheduling on many systems.
Driver behavior affects scheduler signals:

- threaded IRQs run on CPUs and contribute load
- workqueues can wake CPUs repeatedly
- busy waiting looks like CPU demand
- frequent small wakeups may prevent both deep idle and stable frequency choices
- real-time or deadline workloads can drive higher frequencies

Example:

```text
driver polls sensor every 5 ms
  -> CPU wakes often
  -> idle duration prediction is short
  -> deep idle states unused
  -> schedutil sees recurring utilization
  -> CPU may run at higher frequency than expected
```

Fixes may include interrupt-driven design, batching, longer autosuspend delays,
or moving periodic work to subsystem-managed polling.

## Device Drivers And CPU Latency

Some devices need quick CPU response:

- audio playback/capture
- high-speed networking
- real-time industrial I/O
- touch input during active display
- storage queues with tight latency targets

The driver or subsystem may use latency constraints while the workload is
active. The key practice is scoped lifetime:

```text
stream starts
  -> add latency constraint

stream stops
  -> remove latency constraint
```

Do not add permanent constraints in probe unless the platform design truly
requires them.

## Measuring Impact

Use measurements before changing power policy.

Idle state usage:

```sh
watch -n1 'grep . /sys/devices/system/cpu/cpu0/cpuidle/state*/usage'
```

Frequency:

```sh
watch -n1 'grep . /sys/devices/system/cpu/cpufreq/policy*/scaling_cur_freq'
```

Interrupts:

```sh
watch -n1 cat /proc/interrupts
```

Timers:

```sh
sudo cat /proc/timer_list
```

Power events with tracing:

```sh
sudo trace-cmd record -e power -e irq -e timer sleep 10
sudo trace-cmd report
```

Performance counters:

```sh
perf stat -a sleep 10
```

Use workload-specific metrics too: audio underruns, packet latency, frame time,
sensor sample jitter, or application response time.

## Debugging Deep Idle Not Used

Checklist:

- Are deep idle states present and enabled?
- Are exit latencies too high for active PM QoS constraints?
- Is a timer firing too frequently?
- Is an IRQ storm waking the CPU?
- Is a driver polling instead of waiting?
- Is `CONFIG_NO_HZ_IDLE` disabled or `nohz=off` on the command line?
- Is platform firmware exposing only shallow states?
- Is virtualization using a polling idle driver?

Commands:

```sh
cat /sys/devices/system/cpu/cpuidle/current_driver
cat /sys/devices/system/cpu/cpuidle/current_governor_ro
grep . /sys/devices/system/cpu/cpu*/cpuidle/state*/disable
grep . /sys/devices/system/cpu/cpu*/cpuidle/state*/usage
```

## Debugging Frequency Scaling

Checklist:

- Is a cpufreq driver loaded?
- Which governor is active?
- Are min/max policy limits restricting frequency?
- Are OPPs and regulator supplies described correctly?
- Is thermal throttling active?
- Is firmware imposing a limit?
- Are CPUs in a shared policy?
- Is the workload actually CPU-bound?

Commands:

```sh
grep . /sys/devices/system/cpu/cpufreq/policy*/scaling_driver
grep . /sys/devices/system/cpu/cpufreq/policy*/scaling_governor
grep . /sys/devices/system/cpu/cpufreq/policy*/scaling_min_freq
grep . /sys/devices/system/cpu/cpufreq/policy*/scaling_max_freq
grep . /sys/devices/system/cpu/cpufreq/policy*/scaling_cur_freq
grep . /sys/devices/system/cpu/cpufreq/policy*/affected_cpus
```

## Common Driver Mistakes

| Mistake | CPU PM Effect | Better Practice |
| --- | --- | --- |
| Polling frequently | prevents deep idle | use IRQs or batch work |
| Unbounded busy wait | wastes CPU and raises frequency | use bounded polling helpers or sleepable waits |
| Leaking PM QoS constraint | blocks deep idle forever | add/remove constraints with workload lifetime |
| IRQ status not cleared | interrupt storm | ack/mask status correctly |
| Very short periodic work | constant wakeups | increase period or use subsystem batching |
| Wrong OPP data | unstable scaling | validate against hardware and regulator constraints |
| Ignoring shared cpufreq policy | surprising cluster-wide frequency changes | inspect `affected_cpus` |
| Measuring only frequency | wrong conclusion | measure latency, throughput, thermal, and power too |

## Practice Exercises

1. Inspect cpuidle states on a lab machine and identify the deepest state.
2. Run an idle workload and watch `usage` counters. Then add a periodic wakeup
   and observe how state residency changes.
3. Inspect cpufreq policies and identify which CPUs share a policy.
4. Change the governor in a lab and measure both performance and power/thermal
   behavior.
5. Pick a driver with periodic work and estimate whether the period prevents
   deeper idle states.

## Review Checklist

- Does the driver avoid unnecessary polling?
- Are timers and delayed work periods justified?
- Are IRQs acknowledged and masked correctly?
- Are latency constraints scoped to active workloads?
- Is platform OPP data correct for the board and silicon?
- Are thermal and firmware limits considered before blaming cpufreq?
- Are measurements based on idle states, interrupts, frequency, and workload
  behavior together?

## Related Topics

- [Power Domains](power-domains.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Perf Overview](../debugging/perf-overview.md)

## Official References

- [CPU Idle Time Management](https://docs.kernel.org/admin-guide/pm/cpuidle.html)
- [CPU Performance Scaling](https://docs.kernel.org/admin-guide/pm/cpufreq.html)
