---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Remoteproc Framework

## What Problem Does This Solve?

The remoteproc framework gives Linux a common way to manage auxiliary processors
inside a system-on-chip.

Without a framework, every SoC driver would need to invent its own interface
for:

- firmware loading
- reset release
- power-domain control
- memory carveouts
- virtio/RPMsg setup
- crash reporting
- recovery policy
- userspace control

Remoteproc separates common lifecycle handling from SoC-specific operations.

```text
remoteproc core:
  common lifecycle, firmware parsing, resources, sysfs, crash handling

platform remoteproc driver:
  SoC-specific reset, boot address, clocks, power domains, mailboxes, memory map
```

## Architecture

At a high level:

```text
Device Tree node
  -> platform remoteproc driver probes
     -> allocates/registers struct rproc
        -> remoteproc core exposes /sys/class/remoteproc/remoteprocN
           -> firmware can be loaded and started
              -> resource table is parsed
                 -> carveouts, trace buffers, and virtio devices are created
                    -> platform start callback boots the remote core
```

The remoteproc core does not know how to toggle a specific SoC reset line or
program a boot vector. The platform driver supplies those operations.

## Core Concepts

### Remote Processor

A remote processor is an auxiliary processing core controlled or monitored by
Linux:

```text
Cortex-R5
Cortex-M4
PRU
DSP
video firmware core
sensor hub
```

The exact core type matters less than the integration pattern: firmware,
memory, start/stop control, communication, and crash policy.

### `struct rproc`

The platform driver registers a remote processor with the remoteproc core. The
`struct rproc` object represents the Linux-side lifecycle state.

Conceptual driver flow:

```c
static int demo_rproc_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct rproc *rproc;
    struct demo_rproc *priv;
    int ret;

    rproc = rproc_alloc(dev, dev_name(dev), &demo_rproc_ops,
                        "demo-fw.elf", sizeof(*priv));
    if (!rproc)
        return -ENOMEM;

    priv = rproc->priv;
    priv->dev = dev;
    priv->rproc = rproc;

    ret = demo_get_resources(pdev, priv);
    if (ret)
        goto err_free;

    ret = rproc_add(rproc);
    if (ret)
        goto err_free;

    platform_set_drvdata(pdev, rproc);
    return 0;

err_free:
    rproc_free(rproc);
    return ret;
}
```

This is only a shape. Real platform drivers vary by SoC and kernel version.

### Remoteproc Operations

The platform driver provides operations such as:

```c
static const struct rproc_ops demo_rproc_ops = {
    .start = demo_rproc_start,
    .stop = demo_rproc_stop,
    .kick = demo_rproc_kick,
};
```

Common operation responsibilities:

| Operation | Typical Responsibility |
| --- | --- |
| `start` | program boot address, power domain, clocks, reset release |
| `stop` | stop remote core, assert reset, quiesce interrupts |
| `kick` | notify remote core that a virtqueue has work |
| `da_to_va` | translate remote device address to Linux kernel mapping, if needed |
| `parse_fw` | platform-specific firmware/resource-table parsing, if needed |
| `handle_rsc` | platform-specific resource handling, if needed |

Ordinary device-driver developers usually consume remoteproc behavior rather
than writing these platform drivers. Board-porting and SoC-maintenance work may
require understanding them.

## Remoteproc States

Remoteproc exposes lifecycle state through sysfs:

```sh
ls /sys/class/remoteproc
cat /sys/class/remoteproc/remoteproc0/name
cat /sys/class/remoteproc/remoteproc0/state
cat /sys/class/remoteproc/remoteproc0/firmware
```

Common state strings include:

| State | Meaning |
| --- | --- |
| `offline` | remote processor is not running under remoteproc |
| `running` | remote processor is running |
| `suspended` | remote processor is suspended, if supported |
| `crashed` | remoteproc detected or was told about a crash |
| `attached` | Linux attached to a processor already running |
| `detached` | Linux detached from an attached processor, if supported |

Exact state support depends on the platform driver.

Common controls:

```sh
echo start | sudo tee /sys/class/remoteproc/remoteproc0/state
echo stop  | sudo tee /sys/class/remoteproc/remoteproc0/state
```

Some attached-core drivers may also support detach operations. Do not assume
every remoteproc instance can be stopped safely.

## Firmware Selection

The firmware file is visible through sysfs:

```sh
cat /sys/class/remoteproc/remoteproc0/firmware
```

Some platforms allow changing it while the core is stopped:

```sh
echo demo-fw.elf | sudo tee /sys/class/remoteproc/remoteproc0/firmware
```

The file is loaded through the kernel firmware loader, normally from:

```text
/lib/firmware/
```

Firmware names may come from:

- platform driver defaults
- Device Tree `firmware-name`
- ACPI or platform data
- userspace sysfs override, if supported

Changing the firmware name does not fix incompatible Device Tree memory,
mailbox, or RPMsg protocol expectations. Firmware selection is one part of a
larger platform contract.

## Resource Table

Remote firmware often contains a resource table. The remoteproc core reads it
before starting the core.

The resource table can describe:

- memory carveouts
- device memory mappings
- trace buffers
- virtio devices
- vrings for virtio devices

Conceptually:

```text
firmware ELF
  -> program segments
  -> resource table
       carveout: memory needed by firmware
       trace:    firmware log buffer
       vdev:     virtio/RPMsg device and vrings
```

Resource table entries are not a product protocol. They describe resources the
remoteproc core needs in order to load and run the firmware.

## Carveouts And Address Translation

Remote firmware is linked for the remote core's address view. Linux has its own
view of memory. The platform driver maps between them.

Important terms:

```text
device address:
  address used by the remote core or firmware image

physical address:
  SoC physical memory address

kernel virtual address:
  Linux mapping used while loading firmware
```

Example problem:

```text
firmware expects code at device address 0x80000000
Linux reserved-memory node is at physical address 0x9c000000
platform address translation is missing
  -> firmware loads to the wrong place
```

When remoteproc reports carveout or address translation errors, inspect the
firmware linker script, resource table, reserved-memory nodes, and platform
driver address map together.

## Virtio Devices

If the resource table declares a virtio device, remoteproc creates the Linux
virtio side. RPMsg is commonly exposed this way.

Flow:

```text
remoteproc starts firmware
  -> resource table declares virtio RPMsg vdev
  -> remoteproc registers virtio device
  -> virtio_rpmsg_bus binds
  -> RPMsg channels appear when announced
```

If the remote core boots but no RPMsg device appears, check whether the firmware
resource table actually declares the virtio device and vrings expected by Linux.

## Kicks, Mailboxes, And Interrupts

Virtio queues need notification in both directions:

```text
Linux adds buffer to vring
  -> remoteproc kick
     -> mailbox/interrupt to remote core

remote core adds message
  -> mailbox/interrupt to Linux
     -> Linux handles virtqueue
```

The platform remoteproc driver usually implements `.kick` by using a mailbox,
interrupt controller, or SoC-specific doorbell.

Messaging failures often come from this layer:

- wrong mailbox phandle
- wrong mailbox channel
- interrupt not routed
- interrupt trigger wrong
- power domain for mailbox is off
- remote firmware waits for a different notify ID

## Start And Stop

Start usually includes:

```text
ensure memory is ready
load firmware segments
process resource table
enable clocks and power domains
program boot vector or entry point
release reset
enable notifications
```

Stop usually includes:

```text
ask or force remote core to stop
disable notifications
assert reset
disable clocks or power domains
unregister virtio/RPMsg devices
mark state offline
```

The stop operation can be dangerous if the remote core owns safety-critical or
power-management work. Whether stop is allowed is a platform policy question.

## Attach And Detach

Some systems boot remote firmware before Linux. In that case Linux may attach to
an already-running processor instead of loading and starting it.

Attach model:

```text
bootloader starts R5 firmware
  -> Linux remoteproc driver probes
  -> driver detects running core
  -> Linux attaches
  -> Linux may expose RPMsg channels or monitoring
```

Detach model:

```text
Linux stops managing an attached core
  -> core may keep running
  -> Linux releases its control/monitoring relationship
```

Do not treat attach as equivalent to start. Attach assumes firmware, memory, and
communication resources are already initialized.

## Crash Handling

Remoteproc can report a crash when the platform detects a fault, watchdog, or
fatal firmware condition.

Conceptual flow:

```text
platform detects remote fault
  -> rproc_report_crash()
  -> remoteproc marks state crashed
  -> coredump handling runs if configured
  -> recovery policy decides restart
```

Sysfs controls commonly include recovery and coredump attributes:

```sh
cat /sys/class/remoteproc/remoteproc0/recovery
cat /sys/class/remoteproc/remoteproc0/coredump
```

Exact options depend on kernel version and platform support. Use these controls
in a lab to prevent automatic recovery from hiding the first failure.

## Sysfs And Debugfs Inspection

Basic inspection:

```sh
for r in /sys/class/remoteproc/remoteproc*; do
    echo "$r"
    cat "$r/name"
    cat "$r/state"
    cat "$r/firmware"
done
```

Kernel logs:

```sh
dmesg | grep -i remoteproc
dmesg | grep -i rproc
dmesg | grep -i firmware
```

Debugfs, if enabled:

```sh
sudo find /sys/kernel/debug -maxdepth 3 -iname '*remoteproc*' -o -iname '*rproc*'
```

Trace buffers may appear under remoteproc debugfs paths on platforms that
declare firmware trace resources.

## Minimal Lab Workflow

1. Identify remoteproc devices:

   ```sh
   ls /sys/class/remoteproc
   ```

2. Inspect state and firmware:

   ```sh
   cat /sys/class/remoteproc/remoteproc0/name
   cat /sys/class/remoteproc/remoteproc0/state
   cat /sys/class/remoteproc/remoteproc0/firmware
   ```

3. Start the remote core:

   ```sh
   echo start | sudo tee /sys/class/remoteproc/remoteproc0/state
   ```

4. Inspect logs:

   ```sh
   dmesg | tail -100
   ```

5. Stop it only if safe:

   ```sh
   echo stop | sudo tee /sys/class/remoteproc/remoteproc0/state
   ```

Before using stop on a product board, confirm the core is not responsible for
safety, power, storage, networking, or another live service.

## Platform Driver Responsibilities

A remoteproc platform driver often needs:

- MMIO resources
- reset controls
- clocks
- power domains
- memory-region references
- mailbox channels
- interrupts
- firmware name selection
- address translation tables
- crash notification handling

Probe should use normal kernel resource APIs:

```c
priv->rst = devm_reset_control_get_exclusive(dev, NULL);
if (IS_ERR(priv->rst))
    return dev_err_probe(dev, PTR_ERR(priv->rst),
                         "failed to get reset\n");

priv->mbox = mbox_request_channel_byname(&priv->cl, "tx");
if (IS_ERR(priv->mbox))
    return dev_err_probe(dev, PTR_ERR(priv->mbox),
                         "failed to get mailbox\n");
```

Resource failures should be reported with `dev_err_probe()` so probe deferral
does not become noisy.

## Common Bugs

| Bug | Symptom | First Checks |
| --- | --- | --- |
| remoteproc driver not enabled | no `/sys/class/remoteproc` entry | kernel config, Device Tree status |
| provider not ready | probe defers | clocks, resets, power domains, mailbox providers |
| wrong firmware name | start fails with firmware error | sysfs `firmware`, `/lib/firmware`, `dmesg` |
| resource table missing | no virtio/RPMsg devices | firmware build, linker/resource table |
| carveout mismatch | load failure or memory corruption | reserved memory, linker script, address translation |
| mailbox wrong | remote runs but messages hang | mailbox phandles/channels, interrupts |
| stop unsupported but used | remote service disappears or system hangs | ownership model |
| automatic recovery hides crash | logs show repeated restarts | disable recovery in lab and capture evidence |

## Practice Exercises

1. List remoteproc instances on a lab target and record each `name`, `state`,
   and `firmware` attribute.
2. Start a non-critical remote core and capture the `dmesg` messages from probe
   through running state.
3. Stop the core, change the firmware attribute to a missing file, and observe
   the failure path.
4. Find the platform remoteproc driver for your SoC and identify its `.start`,
   `.stop`, and `.kick` implementations.
5. Inspect the firmware resource table output from the firmware build and map
   each entry to Linux-side behavior.

## Review Checklist

- Is Linux supposed to start this core, attach to it, or leave it alone?
- Is the remoteproc platform driver enabled and bound?
- Are reset, clock, power-domain, mailbox, and memory resources described?
- Does the firmware name match the installed rootfs file?
- Does the firmware resource table match Linux expectations?
- Are reserved-memory ranges large enough and non-overlapping?
- Is crash recovery policy appropriate for development and production?
- Does stopping the core have product-level safety consequences?

## Related Topics

- [Firmware Loading](firmware-loading.md)
- [Reserved Memory](reserved-memory.md)
- [Virtio And RPMsg](virtio-rpmsg.md)

## Official References

- [Remote Processor Framework](https://docs.kernel.org/staging/remoteproc.html)
- [Remoteproc Sysfs ABI](https://docs.kernel.org/ABI/testing/sysfs-class-remoteproc)
- [Remote Processor Messaging](https://docs.kernel.org/staging/rpmsg.html)
