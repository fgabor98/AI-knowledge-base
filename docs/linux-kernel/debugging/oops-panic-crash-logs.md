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

They are often the only evidence after:

- NULL pointer dereference
- use-after-free
- invalid MMIO access
- stack corruption
- BUG/WARN checks
- lockups that panic
- page faults in kernel mode
- fatal exceptions in IRQ or workqueue context

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

```text
first fault:
  likely closest to root cause

later warnings/panics:
  may be fallout from corrupted state
```

Preserve the whole log, but start analysis at the first exception or warning that changed the system state.

## Oops Versus Panic

Oops:

- kernel detected a serious fault
- current task may be killed
- system may limp onward
- kernel becomes tainted
- later behavior is less trustworthy

Panic:

- kernel stops or reboots according to panic policy
- system is considered unrecoverable
- crash dump or persistent log may be collected

Some systems turn oops into panic:

```text
oops=panic
panic_on_warn=1
```

Check the command line and sysctls before interpreting behavior.

## Preserve Full Evidence

Capture:

```text
full serial log from before the crash
first oops/panic
all preceding warnings
kernel version
taint flags
command line
loaded modules
final .config
vmlinux and System.map
module binaries
reproduction steps
```

Do not paste only the bottom of the panic. The useful frame may be earlier.

## Anatomy Of An Oops

Common fields:

```text
BUG: kernel NULL pointer dereference
Unable to handle kernel paging request
Oops: ...
CPU: ...
PID: ...
Comm: ...
Tainted: ...
Hardware name: ...
pc/ip/rip: ...
lr/call trace: ...
Call Trace:
Code:
```

Focus on:

- fault type
- faulting address
- instruction pointer / program counter
- current task and context
- taint state
- loaded modules
- call trace
- first driver frame

## Taint Flags

Check taint:

```sh
cat /proc/sys/kernel/tainted
```

Crash logs often include:

```text
Tainted: G           O       6.6.0-custom #1
```

Taint can indicate:

- proprietary module
- out-of-tree module
- forced module load
- prior warning
- machine check
- staging driver

Taint does not prove causality, but it affects supportability and confidence.

## Symbolization

You need matching build artifacts.

Useful artifacts:

```text
vmlinux
System.map
modules
Module.symvers
source tree
final .config
```

If a trace shows:

```text
demo_work_fn+0x34/0x120 [demo]
```

the crash is 0x34 bytes into `demo_work_fn`. Use matching symbols to map that to source.

Possible tools:

```sh
addr2line -e vmlinux address
scripts/faddr2line vmlinux function+offset/size
```

For module addresses, use module-specific symbol handling and exact `.ko` artifacts.

## First Driver Frame

Do not automatically blame the top frame. The top frame may be a generic helper that was passed bad data.

Example:

```text
copy_to_user
demo_read
vfs_read
ksys_read
```

The driver frame `demo_read` is more interesting than `copy_to_user`.

Ask:

```text
which driver-owned pointer was passed?
what lifetime protected it?
what lock protected it?
which context was this?
```

## Common Crash Patterns

### NULL Pointer

Evidence:

```text
BUG: kernel NULL pointer dereference
```

Check:

- missing resource check
- failed allocation not handled
- optional provider treated as required
- `platform_get_drvdata()` returned NULL
- file private data not initialized

### Use-After-Free

Evidence:

```text
KASAN: use-after-free
callback after remove
crash after rmmod/unbind/close
```

Check:

- work canceled?
- timer stopped?
- IRQ freed/disabled?
- open file references?
- sysfs callbacks?
- runtime PM callbacks?

### Invalid MMIO Access

Evidence:

```text
external abort
bus error
unable to handle paging request near register access
```

Check:

- resource address and size
- power/clock/reset state
- mapping with `ioremap` helper
- access width
- device removed/suspended

### Sleep In Atomic

Evidence:

```text
BUG: sleeping function called from invalid context
```

Check:

- hard IRQ or timer callback
- spinlock held
- `GFP_KERNEL` in atomic path
- I2C/SPI/regulator call from atomic context

## Panic Policy

Runtime:

```sh
cat /proc/sys/kernel/panic
cat /proc/sys/kernel/panic_on_oops
```

Command line:

```text
panic=10
oops=panic
panic_on_warn=1
```

Policy affects whether you get a chance to inspect the system after an oops.

## Persistent Crash Evidence

Use persistent capture before you need it:

- serial console logger
- pstore/ramoops
- kdump/crashkernel where appropriate
- persistent journal
- netconsole in lab setups
- bootloader reset reason

After reboot, check previous boot logs:

```sh
journalctl -k -b -1
```

Only works if the system had persistent journal storage and rebooted cleanly enough to keep it.

## Crash Dump Overview

Kdump captures memory for postmortem analysis after panic. It requires:

- reserved crash kernel memory
- kdump userspace setup
- storage/network target
- matching symbols
- tested panic path

For embedded targets, pstore/ramoops is often simpler for first evidence. Kdump is more complete but needs product-level setup.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| only final panic preserved | first oops lost | serial/pstore setup |
| symbols do not line up | wrong `vmlinux` or module | build identity |
| crash after unload | async callback lifetime bug | work/timer/IRQ teardown |
| crash in usercopy | bad userspace pointer handling or freed state | file path and private data |
| crash only under load | race/lifetime bug | KASAN/KCSAN/lockdep |
| reboot without logs | watchdog or panic policy too fast | pstore and timeout |

## Practice Exercises

### Exercise 1: Oops Reading Drill

Take one oops log and identify:

```text
first fault
faulting address
instruction pointer
first driver frame
current task
taint flags
loaded module list
```

### Exercise 2: Symbol Mapping

Use matching `vmlinux` or module artifacts to map a `function+offset/size` frame to source.

### Exercise 3: Lifetime Hypothesis

For a crash after unbind or `rmmod`, list every async callback that could still touch driver state.

## Debugging Checklist

- Preserve full serial logs.
- Check the first exception.
- Match logs to exact kernel build artifacts.
- Check whether the crash follows teardown, interrupt, or userspace entry paths.
- Record taint state.
- Keep unstripped symbols for release builds.
- Check command-line panic policy.
- Enable KASAN/lockdep/KCSAN for reproducible bugs where appropriate.

## Related Topics

- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)
- [Kernel Release Artifacts](../../build-systems/advanced/linux-kernel/kernel-release-artifacts.md)
- [Watchdog Reset Diagnosis](watchdog-reset-diagnosis.md)
- [Kernel Memory And I/O](../memory-and-io/index.md)

## Official References

- [Bug hunting](https://docs.kernel.org/admin-guide/bug-hunting.html)
- [Tainted kernels](https://docs.kernel.org/admin-guide/tainted-kernels.html)
- [pstore block oops/panic logger](https://docs.kernel.org/admin-guide/pstore-blk.html)
