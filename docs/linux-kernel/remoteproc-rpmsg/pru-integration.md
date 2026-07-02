---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# PRU Integration Overview

## What Problem Does This Solve?

Programmable Real-Time Units, commonly called PRUs, are small deterministic
cores found on some TI SoCs. They are often used when Linux cannot meet timing
requirements directly.

Typical PRU use cases:

- deterministic GPIO waveform generation
- industrial fieldbus timing
- software-defined peripheral protocols
- precise timestamping
- low-latency capture
- bit-banging protocols with tight timing
- motor/control loops that need predictable I/O

The Linux-side problem is integration:

```text
Linux:
  configure pins, load firmware, exchange data, expose control plane

PRU firmware:
  execute deterministic timing loop close to hardware
```

Do not use a PRU just to avoid writing a normal Linux driver. Use it when the
timing requirement genuinely belongs outside Linux scheduling.

## Mental Model

PRU integration has two halves:

```text
Linux side
  -> Device Tree enables PRU subsystem
  -> remoteproc loads PRU firmware
  -> pinctrl assigns pins to PRU function
  -> RPMsg or shared memory exchanges control/data
  -> userspace/kernel driver exposes product interface

PRU side
  -> firmware runs tight loop
  -> accesses PRU-local registers and shared memory
  -> signals Linux through interrupts or RPMsg
  -> obeys agreed memory/protocol layout
```

The PRU should own the deterministic inner loop. Linux should own orchestration,
configuration, policy, logging, updates, and integration with the rest of the
system.

## Core Concepts

### PRU Core

A PRU core is a remote processor with local instruction/data memories and access
to selected SoC peripherals. Depending on SoC family, PRUs may be grouped in
PRU-ICSS or PRU-ICSSG subsystems.

Important platform-specific details:

- number of PRU cores
- local memory sizes
- shared RAM layout
- interrupt controller mapping
- pinmux options
- access to external pins
- access to SoC peripherals
- remoteproc driver support

Always use the SoC technical reference manual and kernel bindings for the
specific SoC. PRU details vary significantly across families.

### PRU Firmware

PRU firmware is built for the PRU instruction set and memory map. It may include:

- code and data sections
- resource table for remoteproc
- RPMsg setup
- shared-memory layout definitions
- interrupt mappings
- firmware version metadata

The firmware linker script must match the PRU memory map and any Linux reserved
memory used for shared buffers.

### Remoteproc

Linux commonly controls PRU firmware through remoteproc:

```sh
ls /sys/class/remoteproc
cat /sys/class/remoteproc/remoteproc*/name
cat /sys/class/remoteproc/remoteproc*/state
cat /sys/class/remoteproc/remoteproc*/firmware
```

Start/stop in a lab:

```sh
echo start | sudo tee /sys/class/remoteproc/remoteproc0/state
echo stop  | sudo tee /sys/class/remoteproc/remoteproc0/state
```

Only stop a PRU core when you know it is not controlling live hardware.

### RPMsg

PRU firmware can use RPMsg to communicate with Linux.

Flow:

```text
PRU firmware initializes RPMsg
  -> announces endpoint name
  -> Linux RPMsg driver or userspace interface binds
  -> Linux sends control messages
  -> PRU sends events/status
```

RPMsg is useful for control and status. It is usually not the right path for
every high-rate sample if shared memory or a subsystem buffer model is more
appropriate.

### Shared Memory

PRU and Linux may exchange data through shared memory:

```text
control block:
  command, status, version, flags

ring buffer:
  samples or events

mailbox/interrupt:
  notification that data is ready
```

Shared memory needs:

- explicit layout
- cache/coherency rules
- ownership rules
- barriers
- versioning
- bounds checks

Do not let Linux and PRU firmware include separate hand-written definitions that
drift apart. Generate shared headers or keep a single protocol definition where
practical.

### PRU Interrupts

PRU firmware can signal Linux through PRU interrupt controllers and SoC
interrupt routing. Linux may also signal PRU through system events, mailboxes,
or RPMsg kicks depending on platform.

Debug questions:

- Which PRU system event is used?
- Which host interrupt maps to Linux?
- Is the interrupt controller configured by firmware, Linux, or both?
- Does the Device Tree describe the expected interrupt path?
- Is the interrupt level/edge behavior understood?

### Pinctrl

PRU-controlled pins must be muxed to PRU functions:

```dts
pinctrl-names = "default";
pinctrl-0 = <&pru_pins>;
```

Common pin failures:

- Linux GPIO driver still owns the pin
- pinmux left in UART/SPI/GPIO function
- wrong pull-up/pull-down or drive strength
- bootloader pin state differs from Linux state
- sleep state changes pins while PRU is expected to run

Inspect:

```sh
sudo cat /sys/kernel/debug/pinctrl/*/pinmux-pins
sudo cat /sys/kernel/debug/gpio
```

## PRU Versus Normal Linux Driver

Use a normal Linux driver when:

- timing is in milliseconds or relaxed microseconds
- subsystem already supports the device
- interrupts and DMA can handle the workload
- userspace latency is acceptable
- maintainability matters more than cycle-level timing

Use PRU when:

- timing requires deterministic instruction-level loops
- Linux scheduler latency is unacceptable
- hardware peripheral support is missing but protocol timing is strict
- data acquisition/output must happen at precise intervals
- PRU firmware can be kept small and testable

Bad reason:

```text
"I do not want to learn the Linux subsystem, so I will put everything on PRU."
```

That often creates a harder product: custom firmware, custom protocol, custom
debugging, and custom update policy.

## Integration Patterns

### RPMsg Control Plane

Linux sends configuration, PRU executes real-time loop.

```text
Linux -> PRU:
  set period
  set mode
  start
  stop

PRU -> Linux:
  started
  stopped
  error
  event counter
```

Good for:

- low-rate commands
- status
- diagnostics
- version handshake

### Shared-Memory Data Plane

Use shared memory for higher-rate data:

```text
shared ring buffer:
  producer index
  consumer index
  sample records
  flags
```

Linux maps/uses the buffer through a kernel driver or controlled userspace ABI.
The PRU writes samples or reads output data.

Requirements:

- ring overflow policy
- memory barriers
- cache maintenance or uncached mapping
- sequence counters or timestamps
- recovery after PRU restart

### Kernel Driver Front End

For product-quality integration, Linux often needs a kernel driver or subsystem
front end:

```text
PRU firmware
  -> RPMsg/shared memory
     -> Linux kernel driver
        -> IIO/input/net/tty/misc/subsystem interface
           -> userspace application
```

This keeps userspace from depending on raw PRU internals and lets normal Linux
permissions, subsystem APIs, and diagnostics apply.

### Development Userspace Front End

For bring-up, userspace tools may talk to RPMsg character devices or debug
interfaces.

Use this for:

- firmware smoke tests
- protocol exploration
- diagnostics

Avoid making raw debug interfaces the final product ABI unless that is a
deliberate support decision.

## Firmware/Linux Synchronization Contract

Define the shared ABI:

```c
#define PRU_DEMO_MAGIC 0x50525544 /* "PRUD" */

struct pru_demo_control {
    __le32 magic;
    __le16 abi_major;
    __le16 abi_minor;
    __le32 command;
    __le32 status;
    __le32 producer;
    __le32 consumer;
};
```

Rules:

- firmware writes only fields it owns
- Linux writes only fields it owns
- both sides validate `magic` and ABI version
- all indices are bounded
- ownership transfer has barriers/cache maintenance
- reset/restart reinitializes the block

If the PRU runs independently through Linux suspend/resume, also define what
happens when Linux is asleep.

## Power Management

PRU power behavior depends on platform and product use.

Questions:

- Should PRU keep running while Linux suspends?
- Can PRU wake Linux?
- Which power domain contains PRU and its pins?
- Does PRU firmware depend on clocks Linux may gate?
- Are shared memory and interrupt controllers retained?
- What happens to PRU-controlled external signals during suspend?

Example risk:

```text
PRU is generating industrial timing signal
Linux system suspend turns off PRU domain
external equipment sees invalid waveform
```

Power policy must be designed with firmware, hardware, and product requirements,
not left to default runtime PM behavior.

## Debugging PRU Integration

Start with Linux state:

```sh
cat /sys/class/remoteproc/remoteproc*/name
cat /sys/class/remoteproc/remoteproc*/state
dmesg | grep -Ei 'pru|remoteproc|rpmsg|firmware'
```

Check firmware files:

```sh
cat /sys/class/remoteproc/remoteproc*/firmware
find /lib/firmware -maxdepth 4 -type f | sort
```

Check pins:

```sh
sudo cat /sys/kernel/debug/pinctrl/*/pinmux-pins
```

Check RPMsg:

```sh
find /sys/bus/rpmsg -maxdepth 4 -print
ls -l /dev/rpmsg*
```

Check interrupts:

```sh
cat /proc/interrupts
dmesg | grep -Ei 'irq|interrupt|mailbox|pru'
```

Firmware-side visibility may require:

- trace buffer
- shared-memory debug counters
- GPIO timing pin
- logic analyzer
- vendor PRU debugger
- firmware log messages over RPMsg

## Timing Debugging

For timing-sensitive work, use hardware evidence:

- oscilloscope
- logic analyzer
- timestamp capture
- external protocol analyzer
- PRU cycle counter or firmware counters

Linux logs are usually too slow for cycle-level PRU timing. Use Linux logs for
lifecycle and control-plane events, not for proving deterministic waveform
timing.

## Common Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| wrong firmware file | PRU does not start or runs old behavior | check sysfs firmware and package |
| pinmux wrong | PRU code runs but pin does not toggle | fix pinctrl state |
| endpoint name mismatch | RPMsg client never binds | align firmware and Linux name |
| shared layout drift | corrupted commands/status | share ABI header and version |
| cache ownership undefined | stale samples or commands | define barriers/cache maintenance |
| PRU stopped while controlling hardware | unsafe output or product fault | define lifecycle policy |
| interrupt route wrong | Linux never receives event | check PRU event-to-host mapping and DTS |
| using RPMsg for high-rate samples | dropped messages or latency | use shared-memory ring/subsystem buffer |
| no firmware version check | silent mismatched behavior | add handshake |

## Practice Exercises

1. List remoteproc names on a PRU-capable target and identify which entries are
   PRU cores.
2. Load a simple PRU firmware in a lab and confirm state transitions in sysfs.
3. Toggle a PRU-controlled pin and verify pinmux through debugfs and a logic
   analyzer.
4. Create a minimal RPMsg endpoint name and bind a Linux client driver or test
   userspace tool.
5. Design a shared-memory control block with magic, ABI version, command,
   status, and counters.

## Review Checklist

- Is PRU justified by timing requirements?
- Is firmware installed and versioned with the product image?
- Are PRU pins muxed away from Linux GPIO/peripheral ownership?
- Is the RPMsg or shared-memory ABI explicitly versioned?
- Are interrupts/mailboxes routed correctly?
- Is shared memory cache/coherency defined?
- Is suspend/resume policy safe for PRU-controlled outputs?
- Is there hardware-level timing validation?

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Virtio And RPMsg](virtio-rpmsg.md)
- [Pinctrl](../driver-interfaces/pinctrl.md)

## Official References

- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [Remote Processor Messaging](https://docs.kernel.org/staging/rpmsg.html)
- [PINCTRL Subsystem](https://docs.kernel.org/driver-api/pin-control.html)
