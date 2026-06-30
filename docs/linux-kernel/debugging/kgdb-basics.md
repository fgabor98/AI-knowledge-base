---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# KGDB Basics

## What Problem Does This Solve?

KGDB allows source-level debugging of the running kernel through a debugger connection.

It is useful when you need to stop the kernel and inspect state that logs and tracing cannot explain:

- a complex data structure is corrupted
- a crash happens near a specific callback
- a lock or state machine needs interactive inspection
- the failure path is known, but the state at the failure is unclear

KGDB is not a first-line tool. It stops the target and changes timing, so it can hide races, watchdog behavior, and interrupt timing problems.

## Core Concepts

- kgdb
- kgdboc
- gdb
- breakpoints
- serial console conflicts
- debug symbols
- remote debugging
- stop-the-world behavior

## Mental Model

KGDB is a heavy debugging tool for cases where logs and tracing are not enough. It changes system behavior and needs a controlled lab setup.

```text
target kernel:
  built with KGDB support and debug symbols

transport:
  serial console, keyboard, or another supported I/O path

host:
  gdb with matching vmlinux

workflow:
  break into kernel
  inspect state
  continue or reboot target
```

If stopping the system would make the bug disappear or trigger a watchdog reset, KGDB may not be the right tool.

## When To Use KGDB

Good uses:

- inspect a driver object after a known callback runs
- break on a function that is not timing-sensitive
- inspect an oops reproduction in a controlled lab
- step through init/probe logic on non-production hardware
- verify assumptions about pointers, lists, locks, or state

Poor uses:

- first investigation of a missing device
- high-frequency IRQ timing bugs
- watchdog handoff failures
- production-only races
- bugs that require uninterrupted real-time behavior

Use logs, dynamic debug, ftrace, and crash logs first.

## Configuration Requirements

Common config areas:

```text
CONFIG_KGDB
CONFIG_KGDB_SERIAL_CONSOLE
CONFIG_DEBUG_INFO
CONFIG_FRAME_POINTER or suitable unwinder/debug info
CONFIG_KALLSYMS
```

Exact symbols depend on architecture and kernel version.

Keep matching artifacts:

```text
vmlinux
System.map
modules
final .config
source tree
```

If symbols do not match the running kernel, GDB output will mislead you.

## KGDB Over Serial

`kgdboc` means KGDB over console.

Example command-line shape:

```text
kgdboc=ttyS0,115200
```

The serial device and baud rate must match the target hardware.

Important conflict:

```text
the same serial port may also be the kernel console
```

Sharing can work in some setups, but it is fragile. A separate debug UART is cleaner when available.

## Starting A Session

On target, trigger KGDB entry:

```sh
echo g | sudo tee /proc/sysrq-trigger
```

On host:

```sh
gdb vmlinux
(gdb) target remote /dev/ttyUSB0
```

Depending on setup, you may need serial settings such as baud rate before connecting. Use the host serial device that connects to the target's KGDB port.

## Basic GDB Commands

```gdb
bt
info threads
thread apply all bt
p variable
p *priv
x/16x address
list *function_name
break demo_probe
continue
step
next
delete breakpoints
detach
```

For kernel work, `bt`, `p *object`, and breakpoints on known callbacks are often more useful than single-stepping everything.

## Inspecting Driver State

If you have a global pointer or can reach the object from a stack frame:

```gdb
(gdb) p priv
(gdb) p *priv
(gdb) p priv->stopping
(gdb) p priv->irq
(gdb) p priv->work
```

For linked lists, use caution. Kernel list macros are not always pleasant from GDB. Sometimes it is clearer to add a temporary debug helper or use crash/drgn-style tooling for postmortem analysis.

## Breakpoints

Break on probe:

```gdb
(gdb) break demo_probe
(gdb) continue
```

Break on error path:

```gdb
(gdb) break demo_start_dma if len > 4096
```

Breakpoints in hot paths can make the target unusable:

- IRQ handlers
- scheduler paths
- high-frequency timers
- spinlock-heavy code

Prefer tracepoints/logs for those.

## Watchdog And KGDB

Stopping the kernel can stop watchdog feeding.

Before using KGDB:

- disable watchdog in a lab profile, or
- lengthen timeout, or
- understand hardware nowayout behavior, or
- accept that the board may reset

Never assume KGDB pause time is invisible to the platform.

## SMP And Stop-The-World Behavior

KGDB stops the kernel so GDB can inspect it. On SMP systems, other CPUs may be stopped or controlled depending on architecture and KGDB state.

Implications:

- timing changes
- lockups may no longer reproduce
- hardware continues in some cases while software is stopped
- external devices may timeout
- remote peers may disconnect

Use it in a controlled lab, not as a normal production diagnostic.

## Recovery Plan

Before attaching:

```text
know how to reset the board
have serial console or power control
save current logs
disable or account for watchdog
avoid critical storage writes during breakpoints
```

If the session hangs, use a hardware reset or known recovery path.

## KGDB Versus Other Tools

| Need | Prefer |
| --- | --- |
| inspect first boot error | logs |
| see callback ordering | ftrace/tracepoints |
| find CPU hot path | perf |
| inspect crash after reboot | oops/crash dump |
| stop and inspect live state | KGDB |
| diagnose missing DT node | sysfs/runtime DT |

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| GDB symbols wrong | mismatched `vmlinux` | kernel build identity |
| cannot connect | serial device/baud/wiring wrong | console and host serial |
| target resets while stopped | watchdog active | watchdog policy |
| bug disappears | KGDB changes timing | use tracing/sanitizers |
| breakpoint never hits | code path not executed or module not loaded | logs, symbols, module state |
| console becomes unusable | KGDB and console share serial poorly | transport policy |

## Practice Exercises

### Exercise 1: Symbol Match

Boot a KGDB-capable kernel and verify that host GDB uses the exact matching `vmlinux`.

### Exercise 2: Break On Probe

Set a breakpoint on one driver probe function, bind the device in a lab, inspect the platform device, and continue.

### Exercise 3: Watchdog Interaction

Document what happens if the kernel is stopped in KGDB longer than the watchdog timeout. Do this only on a lab target.

## Debugging Checklist

- Confirm serial wiring and console ownership.
- Match symbols to the running kernel.
- Avoid using KGDB on timing-sensitive failures without accounting for perturbation.
- Keep a recovery path for hung sessions.
- Check watchdog behavior before stopping the kernel.
- Save logs before attaching.
- Prefer breakpoints on narrow known paths.
- Do not use KGDB as the first tool for missing-device failures.

## Related Topics

- [Oops, Panic, And Crash Logs](oops-panic-crash-logs.md)
- [Debug Vs Production Configs](../configuration-and-platform-policy/debug-vs-production-configs.md)
- [Debugging](../../debugging/index.md)
- [Watchdog Options](../configuration-and-platform-policy/watchdog-options.md)

## Official References

- [KGDB](https://docs.kernel.org/dev-tools/kgdb.html)
- [Kernel hacking guides](https://docs.kernel.org/kernel-hacking/index.html)
