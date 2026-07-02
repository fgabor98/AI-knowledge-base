---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# R5 And M4 Firmware Lifecycle

## What Problem Does This Solve?

Cortex-R5 and Cortex-M4 class remote cores are common in heterogeneous embedded
SoCs. They often run firmware for real-time control, safety islands, low-power
tasks, sensor processing, or vendor platform services.

Their lifecycle is more complex than "load firmware and run":

- bootloader may start firmware before Linux
- Linux may attach to already-running firmware
- Linux may own start/stop through remoteproc
- secure firmware may control reset and power
- firmware may need to survive Linux suspend
- firmware may be safety-critical and must not be stopped casually
- firmware updates may require bootloader changes

The first technical question is ownership. The first product question is what
happens if the firmware stops.

## Ownership Models

### Linux-Started Core

Linux owns the lifecycle:

```text
boot
  -> remote core held in reset
  -> Linux remoteproc driver probes
  -> Linux loads firmware
  -> Linux starts core
  -> Linux can stop/restart core
```

Use when:

- firmware is an application-specific coprocessor workload
- stopping the core is safe under Linux control
- firmware file lives in Linux-managed storage
- remoteproc recovery policy is acceptable

### Bootloader-Started Core

The bootloader starts the remote core before Linux:

```text
bootloader
  -> loads R5/M4 firmware
  -> starts core
  -> passes memory/device state to Linux

Linux
  -> remoteproc driver attaches
  -> monitors or communicates
```

Use when:

- firmware must run before Linux
- firmware initializes hardware needed by Linux
- firmware participates in early boot or safety policy
- boot chain signs/authenticates firmware before Linux

Linux must not assume it can reload or stop this firmware unless the platform
explicitly supports it.

### Firmware-Owned Core

Secure firmware, system firmware, or a management controller owns the core.

Linux role:

```text
request service
monitor state if exposed
avoid direct reset/power operations
```

Use when:

- core handles platform management
- core is part of a safety/security island
- direct Linux control would violate isolation
- firmware is authenticated and managed outside Linux

### Development Model

During bring-up, developers may use remoteproc to start/stop firmware manually.

This is useful for:

- firmware smoke tests
- RPMsg protocol testing
- crash/recovery experiments
- memory-map validation

Do not confuse a development control model with final product policy.

## Lifecycle States

A useful lifecycle model:

```text
not loaded
  -> firmware selected
  -> resources available
  -> loaded
  -> started or attached
  -> running
  -> suspended or low-power
  -> crashed
  -> recovered or stopped
  -> updated
```

For each state, define:

- owner
- allowed transitions
- Linux sysfs controls
- firmware expectations
- userspace service behavior
- crash evidence preservation
- update rules

## Bootloader Handoff

When bootloader starts firmware, Linux needs a clear handoff contract.

Checklist:

- Which firmware image did the bootloader load?
- Where did it load code and data?
- Is that memory reserved in Linux DTB?
- Is the remote core already running when Linux probes?
- Are mailboxes/interrupts initialized?
- Are clocks and power domains kept on?
- Does Linux attach or ignore the core?
- Can Linux detach without stopping it?
- How is firmware version exposed to Linux?

Common failure:

```text
bootloader starts R5 firmware in DDR
Linux DTB does not reserve the DDR region
Linux overwrites the firmware later
```

Another:

```text
bootloader starts firmware
Linux remoteproc driver assumes core is stopped
driver resets it during probe
platform service disappears
```

## Linux Start Flow

For Linux-started cores:

```text
remoteproc driver probes
  -> resources acquired
  -> firmware selected
  -> start requested
  -> firmware loaded into carveouts
  -> resource table parsed
  -> power/clock/reset sequence runs
  -> boot address programmed
  -> reset released
  -> remote firmware initializes
  -> RPMsg channels appear if used
```

Lab commands:

```sh
cat /sys/class/remoteproc/remoteproc0/name
cat /sys/class/remoteproc/remoteproc0/firmware
echo start | sudo tee /sys/class/remoteproc/remoteproc0/state
dmesg | tail -100
```

Start failures should be debugged in order:

1. firmware file availability
2. resource table parsing
3. reserved-memory/carveout fit
4. reset/clock/power-domain sequence
5. boot address and entry point
6. remote-side early initialization
7. mailbox/RPMsg setup

## Attach Flow

For already-running cores:

```text
Linux remoteproc driver probes
  -> detects core running
  -> maps existing resources
  -> attaches to lifecycle
  -> exposes state as attached/running depending on driver
  -> communication channels become available
```

Attach is not the same as start:

| Start | Attach |
| --- | --- |
| Linux loads firmware | firmware already loaded |
| Linux programs boot address | bootloader/firmware already did |
| Linux releases reset | core already running |
| resource table may come from firmware file | resource information may come from running firmware or platform data |

Debug attach by checking both bootloader logs and Linux logs. If Linux only sees
the system after handoff, the most important evidence may be gone unless the
bootloader records it.

## Stop And Restart Policy

Stopping an R5/M4 core may be harmless or dangerous depending on its role.

Safe to stop in lab:

```text
test firmware controlling no external hardware
```

Risky to stop:

```text
motor control firmware
safety watchdog firmware
power-management companion
industrial communication firmware
storage or networking offload in active use
```

Before enabling stop/restart in product software:

- quiesce userspace clients
- stop data paths
- put external hardware in safe state
- preserve logs
- stop or detach through the supported platform mechanism
- restart and renegotiate protocol versions
- reinitialize shared memory

## Firmware Update Flow

Linux-started firmware update:

```text
stop clients
stop remote core
install new firmware atomically
set firmware name if needed
start remote core
verify version handshake
restart clients
```

Bootloader-started firmware update:

```text
install firmware into bootloader-controlled storage
update boot metadata/signature
reboot or reset remote core through boot flow
Linux attaches after handoff
verify version handshake
```

Do not update the rootfs copy of a bootloader-started firmware and assume the
running remote core will change. The owner of the boot flow decides.

## Shared Memory Across Restart

After remote restart:

- clear or reinitialize shared-memory control blocks
- reset producer/consumer indices
- invalidate stale sequence numbers
- renegotiate ABI version
- recreate RPMsg endpoints
- complete pending Linux requests with errors
- discard old firmware-owned pointers or handles

Bad pattern:

```text
Linux keeps pending request seq=42
remote firmware restarts and seq counter resets
late response or reused seq corrupts Linux state
```

Use generation counters or protocol reconnect handshakes when remote restart is
expected.

## Power Management

R5/M4 cores often interact with suspend/resume and low-power states.

Questions:

- Does firmware continue running while Linux suspends?
- Can firmware wake Linux?
- Does Linux need to notify firmware before suspend?
- Are shared memory and mailbox retained?
- Are clocks/power domains retained?
- Can firmware access peripherals while Linux has suspended their drivers?
- Does remote firmware need a low-power command before suspend?

Example coordination:

```text
Linux suspend
  -> userspace service tells remote firmware to enter low-power mode
  -> remote firmware acks
  -> Linux arms wake interrupt
  -> system suspends
```

Resume:

```text
Linux wakes
  -> remote firmware reports wake reason
  -> Linux renegotiates protocol state if needed
  -> normal service resumes
```

Do not assume ordinary device suspend callbacks are enough for firmware that
continues running independently.

## Safety And Isolation

R5 cores in particular may be used for safety-related functions. Even when the
hardware is not certified for your product, the architectural pattern matters.

Design questions:

- Can Linux crash without stopping the remote core?
- Can the remote core reset Linux?
- Can Linux overwrite remote memory?
- Does the remote core have unrestricted DMA?
- Are safety outputs put into a safe state on firmware crash?
- Does a watchdog supervise the remote core?
- Who owns recovery after watchdog expiration?

If safety is involved, do not rely on ad hoc remoteproc start/stop scripts as
the lifecycle policy. Treat the remote core as a separate product component with
requirements and verification.

## Debugging Lifecycle Issues

Basic Linux checks:

```sh
cat /sys/class/remoteproc/remoteproc*/name
cat /sys/class/remoteproc/remoteproc*/state
cat /sys/class/remoteproc/remoteproc*/firmware
dmesg | grep -Ei 'remoteproc|rproc|firmware|mailbox|reset'
```

Bootloader-started checks:

- bootloader log
- firmware image name/hash loaded by bootloader
- bootloader memory load address
- DTB reserved-memory nodes
- attach-related kernel logs

Power/reset checks:

```sh
dmesg | grep -Ei 'reset|clock|power domain|genpd|mailbox'
sudo cat /sys/kernel/debug/pm_genpd/pm_genpd_summary
sudo cat /sys/kernel/debug/clk/clk_summary
```

RPMsg checks:

```sh
find /sys/bus/rpmsg -maxdepth 4 -print
ls -l /dev/rpmsg*
```

## Common Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| wrong ownership assumption | Linux resets a bootloader-started core | model attach/start correctly |
| firmware memory not reserved | random Linux corruption | reserve all remote-owned memory |
| update path changes wrong file | firmware appears not to update | update owner-controlled storage |
| no version handshake | mixed kernel/firmware silently misbehaves | add ABI negotiation |
| stop used on live control firmware | external hardware fault | define safe quiesce policy |
| restart does not clear state | stale messages or ring indices | reset shared memory/generation |
| suspend ignores remote core | missed wake or unsafe output | define suspend handshake |
| crash recovery too fast | first fault evidence lost | preserve logs/coredump before restart |

## Practice Exercises

1. Pick one R5 or M4 core on a target. Determine whether it is Linux-started,
   bootloader-started, attached, or firmware-owned.
2. Trace the start or attach path in `dmesg` and write the lifecycle sequence in
   plain text.
3. Compare the firmware file path in sysfs with the product image manifest.
4. Design a restart handshake that clears shared memory and renegotiates ABI
   version.
5. Write a suspend/resume policy for a remote firmware that must wake Linux.

## Review Checklist

- Is lifecycle ownership explicit?
- Are bootloader-started memory regions reserved in Linux?
- Is start/stop safe for the remote core's product role?
- Does firmware update flow match the owner of boot/start?
- Is ABI/version compatibility checked after every start or attach?
- Is shared memory reinitialized after restart?
- Is suspend/resume behavior defined?
- Are crash logs preserved before recovery?

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Firmware Loading](firmware-loading.md)
- [Device Tree Nodes For Remote Cores](device-tree-nodes-for-remote-cores.md)

## Official References

- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [Remoteproc Sysfs ABI](https://docs.kernel.org/ABI/testing/sysfs-class-remoteproc)
- [Remote Processor Messaging](https://docs.kernel.org/staging/rpmsg.html)
