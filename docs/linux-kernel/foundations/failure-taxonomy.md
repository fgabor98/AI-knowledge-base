---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Failure Taxonomy

## What Problem Does This Solve?

"The driver does not work" is not a useful debugging statement. Kernel failures must be classified by when they happen and what evidence proves the failure point.

This page provides a beginner-friendly taxonomy for driver failures, including symptoms, first commands, likely causes, and next steps.

## Core Concepts

- build failure
- module load failure
- symbol mismatch
- missing device
- match failure
- probe failure
- deferred probe
- resource lookup failure
- runtime I/O failure
- interrupt failure
- userspace ABI failure
- teardown failure
- suspend/resume failure
- oops
- hang
- watchdog reset
- race
- lifetime bug
- tainted kernel

## Decision Tree

Start here:

```text
did it build?
  no -> build failure
  yes
    did the module or kernel boot/load?
      no -> load/boot failure
      yes
        does the device object exist?
          no -> firmware/bus/device creation failure
          yes
            did the driver bind?
              no -> match/autoload failure
              yes
                did probe succeed?
                  no -> probe/resource/provider failure
                  yes
                    does the userspace ABI exist?
                      no -> subsystem/class/udev failure
                      yes
                        does runtime behavior work?
                          no -> callback/I/O/IRQ/state failure
                          crash/hang/reset -> stability failure
```

## Failure Classes

### Build Failure

Symptoms:

```text
error: implicit declaration of function
error: invalid use of undefined type
make[1]: *** [scripts/Makefile.build:...] Error 1
```

First commands:

```bash
make V=1
uname -r
ls /lib/modules/$(uname -r)/build
```

Likely causes:

- wrong kernel headers
- API changed between kernel versions
- missing include
- missing Kconfig dependency
- compiling userspace assumptions into kernel code
- cross-compiler mismatch

Next steps:

- confirm target kernel version
- inspect nearby in-tree driver using the same API
- check `include/linux/` header for current prototype
- check Kconfig dependencies

### Module Load Failure

Symptoms:

```text
insmod: ERROR: could not insert module demo.ko: Invalid module format
insmod: ERROR: could not insert module demo.ko: Unknown symbol in module
insmod: ERROR: could not insert module demo.ko: Operation not permitted
```

First commands:

```bash
dmesg | tail -100
modinfo demo.ko
uname -r
lsmod | grep demo
```

Likely causes:

- vermagic mismatch
- missing exported symbol
- dependency module not loaded
- module signing or lockdown policy
- wrong architecture
- module already loaded

Next steps:

```bash
modinfo demo.ko | grep vermagic
modinfo demo.ko | grep depends
sudo modprobe dependency_name
dmesg | grep -i "module"
```

### Missing Device

Symptoms:

- module loads but `probe` never runs
- no device under `/sys/bus/platform/devices`
- no I2C/SPI client appears

First commands:

```bash
find /sys/bus/platform/devices -maxdepth 1 -print
find /sys/bus/i2c/devices -maxdepth 2 -print
find /sys/bus/spi/devices -maxdepth 2 -print
dmesg | grep -i "of:"
```

Likely causes:

- Device Tree node missing
- wrong DTB deployed
- node has `status = "disabled"`
- parent bus did not probe
- wrong I2C/SPI chip-select or address declaration
- ACPI or platform data absent

Next steps:

```bash
cat /proc/cmdline
find /proc/device-tree -maxdepth 4 -name compatible -print
tr '\0' '\n' < /proc/device-tree/path/to/node/status
```

### Match Or Autoload Failure

Symptoms:

- device exists but no driver symlink
- manual `modprobe` needed
- `probe` never runs

First commands:

```bash
cat /sys/bus/platform/devices/DEVICE/modalias
modinfo demo.ko | grep alias
readlink /sys/bus/platform/devices/DEVICE/driver
```

Likely causes:

- `compatible` string mismatch
- missing `MODULE_DEVICE_TABLE`
- driver registered on wrong bus
- module not installed in module search path
- `depmod` not run
- driver name mismatch for non-DT platform devices

Next steps:

```bash
sudo depmod -a
sudo modprobe demo
echo DEVICE | sudo tee /sys/bus/platform/drivers/demo/bind
```

Use manual bind only as a diagnostic tool.

### Probe Failure

Symptoms:

```text
demo 48000000.demo: probe failed: -22
demo: probe of 48000000.demo failed with error -517
```

First commands:

```bash
dmesg | grep -i demo
find /sys/kernel/debug/devices_deferred -type f -maxdepth 1 -print -exec cat {} \; 2>/dev/null
```

Likely causes:

- missing regulator, clock, reset, GPIO, IRQ, or MMIO resource
- provider driver not ready
- invalid Device Tree property
- hardware ID mismatch
- power sequencing failure
- pinmux missing

Next steps:

- preserve original error code
- use `dev_err_probe` in probe paths
- check provider subsystem debugfs
- check Device Tree binding
- check final kernel config

### Deferred Probe

Symptoms:

```text
-EPROBE_DEFER
error -517
```

Meaning:

The driver asked for a resource whose provider is not ready yet. This is not necessarily fatal.

First commands:

```bash
cat /sys/kernel/debug/devices_deferred 2>/dev/null
dmesg | grep -i defer
```

Likely providers:

- regulator
- clock
- reset controller
- GPIO controller
- pinctrl provider
- PHY
- interrupt controller

Next steps:

- check whether provider driver is enabled
- check provider Device Tree node
- check probe order only after confirming provider exists

### Runtime I/O Failure

Symptoms:

- `read()` returns `-EIO`
- I2C/SPI transaction fails
- register reads return unexpected values
- userspace command times out

First commands:

```bash
dmesg | tail -100
strace -o trace.txt your-test-command
cat /sys/kernel/debug/regmap/*/registers 2>/dev/null
```

Likely causes:

- hardware not powered
- clock disabled
- reset asserted
- wrong bus address
- wrong SPI mode
- bad pinmux
- runtime PM suspended the device
- locking bug

Next steps:

- check power dependencies
- check bus-level tools cautiously
- add targeted dynamic debug
- inspect regmap/debugfs if available

### Interrupt Failure

Symptoms:

- `/proc/interrupts` count does not change
- handler logs never appear
- interrupt storm
- system becomes unresponsive after enabling IRQ

First commands:

```bash
cat /proc/interrupts | grep -i demo
dmesg | grep -i irq
sudo trace-cmd record -e irq sleep 5
```

Likely causes:

- wrong interrupt specifier
- wrong trigger type
- interrupt controller not probed
- pinmux issue
- line stuck active
- IRQ not acknowledged in hardware
- shared IRQ handler returns wrong status

Next steps:

- check Device Tree interrupt parent
- check trigger type
- confirm hardware status clear sequence
- use threaded IRQ for sleepable work

### Userspace ABI Failure

Symptoms:

- `/dev` node missing
- sysfs file missing
- permission denied
- userspace sees unexpected format
- `udev` rule does not apply

First commands:

```bash
ls -l /dev/demo*
udevadm info /dev/demo0
find /sys/class -maxdepth 3 -name '*demo*'
dmesg | tail -100
```

Likely causes:

- subsystem registration failed
- class/device creation failed
- udev rule missing
- permissions wrong
- ABI not documented or unstable
- userspace using wrong node

Next steps:

- verify kernel object exists before debugging udev
- check major/minor numbers
- inspect `uevent`
- test as root only to separate permissions from driver behavior

### Teardown Failure

Symptoms:

- `rmmod` hangs
- `rmmod` says module in use
- crash after unload
- workqueue callback runs after remove

First commands:

```bash
lsmod | grep demo
cat /sys/module/demo/refcnt
dmesg | tail -100
```

Likely causes:

- open file descriptor
- active sysfs callback
- IRQ still registered
- timer still active
- work not canceled
- reference leak
- use-after-free

Next steps:

- close userspace handles
- stop hardware before freeing state
- cancel timers and work
- free IRQs before freeing data
- use KASAN in a debug kernel

### Suspend/Resume Failure

Symptoms:

- suspend fails
- system wakes immediately
- device missing after resume
- I/O fails after resume

First commands:

```bash
dmesg | grep -i "suspend\\|resume\\|wakeup"
cat /sys/kernel/debug/wakeup_sources 2>/dev/null
```

Likely causes:

- device not quiesced
- wake IRQ misconfigured
- state not restored
- runtime PM interaction
- clock/regulator sequencing problem

Next steps:

- test suspend phases
- add PM debug logs
- trace suspend/resume callbacks
- compare with device disabled

### Oops Or Panic

Symptoms:

```text
BUG: kernel NULL pointer dereference
Unable to handle kernel paging request
Kernel panic - not syncing
```

First commands:

- capture full serial log
- preserve first fault
- match symbols to exact build
- check taint state

```bash
cat /proc/sys/kernel/tainted
```

Likely causes:

- NULL pointer
- use-after-free
- stack corruption
- invalid MMIO access
- bad callback lifetime
- locking bug
- out-of-tree or forced modules affecting the evidence

Next steps:

- decode stack trace
- map faulting address to source
- enable KASAN/lockdep if reproducible
- inspect teardown and async paths

### Hang Or Watchdog Reset

Symptoms:

- no shell response
- no logs after point X
- watchdog reset reason after reboot

First commands after reboot:

```bash
dmesg -T | head
journalctl -k -b -1
cat /sys/class/watchdog/watchdog*/status 2>/dev/null
```

Likely causes:

- deadlock
- interrupt storm
- infinite loop in kernel context
- hardware bus transaction stuck
- scheduler starvation
- watchdog feeding stopped

Next steps:

- capture serial logs
- enable lockup detectors
- use ftrace with short windows
- inspect watchdog owner and timeout

## Summary Table

| Failure class | First evidence | First commands |
|---|---|---|
| build failure | compiler output | `make V=1` |
| load failure | `insmod` error, dmesg | `modinfo`, `dmesg` |
| missing device | no sysfs device | `find /sys/bus/.../devices` |
| match failure | no driver symlink | `modalias`, `modinfo alias` |
| probe failure | probe error code | `dmesg`, provider debugfs |
| runtime I/O | syscall or bus error | `dmesg`, `strace`, subsystem debugfs |
| IRQ failure | no count or storm | `/proc/interrupts`, irq tracepoints |
| ABI failure | missing node/file | `/dev`, sysfs, `udevadm` |
| teardown failure | unload hang/crash | `lsmod`, refcnt, logs |
| crash | oops/panic | serial log, symbols |
| hang/reset | no progress/reboot | serial log, previous boot logs |

## Common Mistakes

- Skipping classification and randomly changing code.
- Treating `-EPROBE_DEFER` as a normal permanent failure.
- Debugging `/dev` before checking whether `probe` succeeded.
- Ignoring the exact negative error code.
- Looking only at the final error, not the first error.
- Assuming module load means hardware exists.
- Assuming hardware exists because Device Tree source was edited, without checking runtime DTB.

## Debugging Checklist

- What phase failed?
- What is the first error code?
- Which kernel object should exist at this phase?
- Which command proves it exists or does not exist?
- Which resource or provider is missing?
- Is this a userspace policy issue or kernel driver issue?
- Can the failure be reproduced with one minimal test?

## Related Topics

- [Debugging Ladder](debugging-ladder.md)
- [Probe Failure Debugging](../debugging/probe-failure-debugging.md)
- [Device Model Primer](device-model-primer.md)
- [Execution Context Primer](execution-context-primer.md)

## References

- Driver Model binding: <https://docs.kernel.org/driver-api/driver-model/binding.html>
- Dynamic debug HOWTO: <https://docs.kernel.org/admin-guide/dynamic-debug-howto.html>
- Kernel oops tracing: <https://docs.kernel.org/admin-guide/bug-hunting.html>
