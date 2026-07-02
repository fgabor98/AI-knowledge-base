---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Remote Core Logs And Crashes

## What Problem Does This Solve?

Remote-core failures do not always look like Linux kernel failures. The Linux
kernel may keep running while firmware on an R5, M4, PRU, DSP, or other remote
core has crashed, stopped responding, corrupted shared memory, or restarted
silently.

Symptoms:

```text
RPMsg requests time out
remoteproc state becomes crashed
firmware restarts repeatedly
messages corrupt under load
watchdog resets the whole board
external device stops responding
Linux logs show only "remote processor crashed"
```

Debugging needs evidence from both sides:

```text
Linux side:
  remoteproc state, dmesg, coredump, trace buffers, RPMsg devices

remote side:
  firmware logs, fault registers, stack, watchdog reason, shared-memory markers
```

## Failure Classes

Classify the failure before changing code.

| Failure Class | Typical Symptom | First Evidence |
| --- | --- | --- |
| firmware load failure | remote core never starts | `dmesg`, firmware path, resource table |
| early firmware crash | state changes to crashed soon after start | remoteproc logs, trace banner |
| communication hang | remote remains running but RPMsg times out | vrings, mailbox interrupts, firmware task state |
| shared-memory corruption | random data or kernel crash | carveouts, cacheability, bounds checks |
| watchdog reset | remote or whole board resets | watchdog logs, reset reason, persistent storage |
| automatic recovery loop | repeated start/crash messages | recovery policy, first crash evidence |
| silent firmware fault | no Linux error, service stops | firmware heartbeat, shared counters |
| suspend/resume fault | works before sleep, fails after resume | power-domain, mailbox, memory retention |

## Remoteproc State And Logs

Start with sysfs:

```sh
for r in /sys/class/remoteproc/remoteproc*; do
    echo "$r"
    cat "$r/name"
    cat "$r/state"
    cat "$r/firmware"
done
```

Check remoteproc logs:

```sh
dmesg | grep -Ei 'remoteproc|rproc|rpmsg|virtio|firmware|carveout|vring'
```

Useful questions:

- Did the remote core ever reach running state?
- Did it crash during firmware load, start, RPMsg initialization, or later?
- Did RPMsg devices appear and then disappear?
- Did the kernel report a coredump or recovery?
- Did the crash happen after a specific Linux request?

## Recovery Policy

Remoteproc may support recovery controls:

```sh
cat /sys/class/remoteproc/remoteproc0/recovery
```

Typical policies are platform/kernel dependent, but the important distinction is:

```text
automatic recovery enabled:
  useful for availability
  can hide first-failure evidence

automatic recovery disabled:
  useful for debugging
  leaves the core crashed/stopped for inspection
```

In a lab, disable automatic recovery when you need first-fault evidence. In a
product, choose policy based on safety and availability requirements.

Example workflow:

```sh
cat /sys/class/remoteproc/remoteproc0/recovery
echo disabled | sudo tee /sys/class/remoteproc/remoteproc0/recovery
# reproduce crash
cat /sys/class/remoteproc/remoteproc0/state
dmesg | tail -200
```

Do not leave recovery disabled on a system that relies on automatic restart
unless that is deliberate.

## Coredumps

Remoteproc can expose coredump behavior on platforms that support it:

```sh
cat /sys/class/remoteproc/remoteproc0/coredump
```

The exact modes depend on kernel version. Common concepts:

- disabled: do not collect coredump
- enabled: collect coredump through kernel mechanism
- inline: keep coredump flow tied to recovery path

Development tradeoff:

```text
coredump enabled:
  more evidence
  more memory/time overhead
  may delay recovery

coredump disabled:
  faster recovery
  less evidence
```

If Linux exposes coredumps through devcoredump, inspect:

```sh
ls /sys/class/devcoredump
```

Preserve coredumps before restarting or rebooting. Some evidence is
single-shot.

## Trace Buffers

Remote firmware can expose trace buffers through resource table entries. Linux
may make those available through remoteproc debugfs paths when supported.

Find likely paths:

```sh
sudo find /sys/kernel/debug -maxdepth 4 \
    -iname '*remoteproc*' -o -iname '*rproc*' -o -iname '*trace*'
```

Trace buffer content often looks like firmware logs:

```text
[00000123] boot
[00000124] rpmsg init
[00000125] waiting for command
[00000310] fault: invalid state 7
```

Trace buffer limitations:

- may be overwritten by circular logging
- may not flush if firmware crashes early
- may require resource table declaration
- may not survive power loss
- may use firmware-specific formatting

Add a boot banner with firmware version and ABI version. It is one of the most
useful trace lines.

## Firmware Heartbeats

For silent faults, add a heartbeat:

```text
shared memory:
  heartbeat counter increments every 100 ms
  state field records firmware mode
  last_error records fatal code
```

Linux monitor:

```text
read heartbeat
wait interval
read heartbeat again
if unchanged and firmware should be active:
  report timeout
```

Design carefully:

- heartbeat period must account for low-power modes
- Linux must use correct memory ordering/cache handling
- heartbeat alone does not prove the full service is healthy
- a remote firmware crash may freeze the last "healthy" value

## Watchdogs

Remote cores may be watched by:

- internal firmware watchdog
- SoC watchdog assigned to remote core
- Linux-side heartbeat watchdog
- external supervisor
- safety island monitor

Watchdog failures may reset:

- only the remote core
- a subsystem
- the entire SoC
- an external safety output

Debug questions:

- Which watchdog fired?
- Who configured it?
- Is the reset reason preserved?
- Did automatic recovery erase evidence?
- Was the watchdog petting path blocked by interrupt, memory, or scheduler
  issues?

For whole-board resets, use:

- pstore/ramoops
- bootloader reset reason
- PMIC reset reason
- SoC reset status registers
- external debugger
- serial console logs

## RPMsg Timeout Debugging

If RPMsg requests time out while remoteproc state remains running:

Check Linux side:

```sh
cat /sys/class/remoteproc/remoteproc0/state
find /sys/bus/rpmsg -maxdepth 4 -print
cat /proc/interrupts
dmesg | grep -Ei 'rpmsg|virtio|vring|mailbox|mbox'
```

Check firmware side:

- did firmware receive the message?
- did firmware send a response?
- is the firmware task blocked?
- did the endpoint name/address change?
- are vring buffers exhausted?
- did the protocol reject the message?

Separate transport from protocol:

```text
transport failure:
  interrupts/vrings/mailbox broken

protocol failure:
  message delivered but firmware rejects or ignores it
```

Add firmware counters:

```text
rx_count
tx_count
bad_len_count
bad_version_count
last_cmd
last_error
```

These counters often shorten debugging more than extra Linux logs.

## Shared-Memory Corruption Debugging

Symptoms:

- invalid command values
- impossible indices
- kernel oops near RPMsg or client driver
- firmware hard fault after Linux writes data
- intermittent failures under traffic

Checklist:

- Are all buffers inside reserved memory?
- Are lengths validated on both sides?
- Are ring indices bounded?
- Are producer/consumer ownership rules clear?
- Are cache maintenance and barriers correct?
- Did firmware restart while Linux still used old state?
- Is the remote core using an address from the wrong address space?

Use canaries in lab protocols:

```c
struct demo_shared {
    __le32 magic;
    __le32 abi;
    __le32 producer;
    __le32 consumer;
    __le32 guard0;
    u8 data[4096];
    __le32 guard1;
};
```

Check guards after heavy traffic.

## Crash Reproduction Discipline

Use reproducible notes:

```text
kernel commit:
DTB:
firmware hash:
remoteproc name:
firmware sysfs value:
recovery mode:
coredump mode:
sleep state, if relevant:
test command:
first bad log line:
```

Before reproducing:

```sh
dmesg -C
cat /sys/class/remoteproc/remoteproc0/state
cat /sys/class/remoteproc/remoteproc0/firmware
```

After reproducing:

```sh
dmesg > /tmp/remote-crash.dmesg
cat /sys/class/remoteproc/remoteproc0/state > /tmp/remote-state.txt
sudo find /sys/kernel/debug -maxdepth 4 -iname '*trace*' -print
```

Preserve logs before restarting the remote core.

## Product Diagnostics

A product should expose enough diagnostics to answer:

- which firmware version is running?
- how many crashes occurred?
- what was the last fatal reason?
- did recovery happen?
- what was the last RPMsg command?
- are shared-memory counters sane?
- what remoteproc state is visible?
- what firmware image hash was installed?

Possible surfaces:

- kernel logs
- sysfs/debugfs for development builds
- product health service
- persistent crash partition
- pstore/ramoops
- remote firmware trace buffer
- userspace diagnostic command

Do not rely on debugfs as the only production diagnostic surface. Debugfs is a
development interface and may be disabled.

## Common Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| automatic recovery hides first fault | only repeated restart logs remain | disable recovery in lab, preserve evidence |
| no firmware version in logs | cannot match crash to image | add boot banner/version handshake |
| trace buffer not declared | no remote logs | add resource table trace resource if supported |
| coredump disabled during development | no crash memory | enable coredump when practical |
| watchdog reset loses logs | board reboots without evidence | add pstore/reset reason capture |
| RPMsg timeout blamed on protocol | mailbox/vring actually broken | separate transport and protocol checks |
| firmware restart not handled | stale Linux state | reset protocol state on endpoint remove/probe |
| shared counters cached incorrectly | false heartbeat failure | fix coherency and barriers |

## Practice Exercises

1. Capture remoteproc state, firmware name, and recent `dmesg` before and after
   starting a remote core.
2. Find whether your platform exposes remoteproc recovery and coredump sysfs
   attributes.
3. Add a firmware boot banner containing firmware version, ABI version, and
   build ID.
4. Design a shared-memory diagnostic block with heartbeat, last command, last
   error, RX count, and TX count.
5. Reproduce an RPMsg timeout and decide whether evidence points to transport or
   protocol.

## Debugging Checklist

- Did the remote core start, attach, crash, or silently stop responding?
- Is automatic recovery enabled?
- Is coredump collection enabled where useful?
- Are firmware trace buffers available?
- Does firmware expose version and heartbeat information?
- Are RPMsg devices still present?
- Are mailbox interrupts firing?
- Are shared-memory guards/counters sane?
- Are logs preserved before restart or reboot?
- Does product diagnostics expose enough information without debugfs?

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Watchdog Reset Diagnosis](../debugging/watchdog-reset-diagnosis.md)
- [Embedded Productization](../../embedded-productization/index.md)

## Official References

- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [Remoteproc Sysfs ABI](https://docs.kernel.org/ABI/testing/sysfs-class-remoteproc)
- [Remote Processor Messaging](https://docs.kernel.org/staging/rpmsg.html)
