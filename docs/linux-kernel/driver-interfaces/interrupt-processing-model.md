---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Interrupt Processing Model

## What Problem Does This Solve?

Interrupt processing connects a hardware signal from an interrupt controller to the action registered by a driver.

Drivers usually see a Linux IRQ number and a handler callback, but the full path has several layers:

```text
hardware event
-> interrupt controller input
-> interrupt domain mapping
-> Linux IRQ number
-> generic IRQ core
-> flow handler
-> driver action
```

Understanding that path helps debug missing interrupts, wrong trigger types, storming lines, shared IRQ confusion, and GPIO-expander interrupt problems.

## Core Concepts

- interrupt controller
- interrupt domain
- hardware IRQ
- Linux IRQ number
- `irq_desc`
- `irq_chip`
- `irqaction`
- interrupt flow handler
- top half
- threaded interrupt
- chained interrupt handler
- nested threaded interrupt
- interrupt affinity
- trigger type
- interrupt masking
- wake IRQ

## Mental Model

Hardware interrupts are translated before a driver sees them.

```text
Device Tree interrupt specifier
  <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>

interrupt controller driver
  maps hardware line to Linux IRQ

generic IRQ core
  manages masking, flow, actions, stats

driver
  requests Linux IRQ and handles device event
```

A driver should not assume the Linux IRQ number is a stable hardware line. It is a kernel-managed identifier.

## Hardware IRQ Versus Linux IRQ

Hardware IRQ:

```text
line 42 on a GIC
pin 7 on a GPIO expander
MSI vector from PCIe
```

Linux IRQ:

```text
integer returned by platform_get_irq()
integer shown in /proc/interrupts
integer passed to request_irq()
```

Mapping happens through IRQ domains.

Driver code:

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return dev_err_probe(&pdev->dev, irq, "failed to get irq\n");
```

Do not hard-code Linux IRQ numbers.

## Device Tree Interrupt Specifiers

Simple platform example:

```dts
demo@10000000 {
    compatible = "example,demo-mmio";
    reg = <0x0 0x10000000 0x0 0x1000>;
    interrupts = <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>;
};
```

GPIO interrupt example:

```dts
interrupt-parent = <&gpio1>;
interrupts = <7 IRQ_TYPE_LEVEL_LOW>;
```

Named interrupts:

```dts
interrupt-names = "data-ready", "error";
interrupts = <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>,
             <GIC_SPI 43 IRQ_TYPE_LEVEL_HIGH>;
```

Driver:

```c
irq = platform_get_irq_byname(pdev, "data-ready");
```

The meaning of the cells is defined by the interrupt controller binding.

## Interrupt Flow

The generic IRQ core has flow handlers for common electrical and controller behavior:

- edge-triggered interrupts
- level-triggered interrupts
- fasteoi controllers
- hierarchical controllers
- chained controller paths

As a normal device driver author, you usually do not choose a flow handler directly. You configure trigger type through firmware data or request flags and implement your handler correctly.

For level-triggered devices, the driver usually must clear the device's interrupt condition before returning, or the line will immediately fire again.

For edge-triggered devices, the driver must avoid missing events between status reads and acknowledgements.

## Driver Actions

When a driver requests an IRQ, it registers an action:

```c
ret = devm_request_irq(dev, irq, demo_irq, 0, dev_name(dev), priv);
```

or a threaded action:

```c
ret = devm_request_threaded_irq(dev, irq, demo_irq, demo_irq_thread,
                                IRQF_ONESHOT, dev_name(dev), priv);
```

The core calls the handler when the interrupt fires.

Handler return values matter:

| Return | Meaning |
| --- | --- |
| `IRQ_HANDLED` | This driver handled the interrupt. |
| `IRQ_NONE` | This interrupt was not for this device. Important for shared IRQs. |
| `IRQ_WAKE_THREAD` | Wake the threaded handler. |

## Hard IRQ Context

Hard IRQ handlers run in interrupt context.

Hard IRQ handlers must not:

- sleep
- perform I2C/SPI transactions
- call `copy_to_user()`
- take mutexes
- perform unbounded work
- allocate with `GFP_KERNEL`
- wait for completions that can sleep

They can usually:

- read/write MMIO registers
- acknowledge device interrupt state
- store simple state
- wake a thread or wait queue
- schedule work
- return `IRQ_WAKE_THREAD`

If your handler needs to sleep, use a threaded IRQ or defer work.

## Threaded Interrupts

Threaded interrupts split work:

```text
hard handler:
  quick check/acknowledge
  return IRQ_WAKE_THREAD

thread handler:
  sleepable work
  I2C/SPI transfers
  mutexes
  input/IIO event reporting
```

Many bus-connected devices should use threaded IRQs because status reads happen over I2C or SPI.

## Chained And Nested Interrupts

Interrupt controller drivers and GPIO expanders may implement chained or nested interrupt handling.

Example:

```text
SoC IRQ line
-> GPIO controller interrupt
-> GPIO line child IRQ
-> consumer driver IRQ handler
```

For a GPIO expander:

```text
SoC GPIO IRQ
-> expander parent IRQ
-> expander driver reads status over I2C
-> child IRQs are dispatched to consumers
```

This is provider-driver work. Normal consumer drivers just request their mapped IRQ.

## `/proc/interrupts`

Inspect:

```sh
cat /proc/interrupts
```

Example shape:

```text
           CPU0       CPU1
 42:        10          0     GICv3  42 Level     demo
```

Useful questions:

- Does the IRQ appear?
- Does the count increase?
- Which controller owns it?
- Is the trigger type visible?
- Which handler name is attached?
- Which CPUs handle it?

## Interrupt Affinity

Some systems allow interrupt CPU affinity:

```sh
cat /proc/irq/<irq>/smp_affinity
cat /proc/irq/<irq>/effective_affinity
```

Affinity matters for performance, latency, and power, but most beginner driver bugs are not solved by changing affinity. First prove mapping, trigger type, and handler behavior.

## Wake IRQs

Some interrupts can wake the system from suspend.

Typical pieces:

- device wakeup capability
- interrupt marked as wake-capable
- driver enables wake during suspend
- power domain remains able to signal wake

Driver code may use APIs such as:

```c
device_init_wakeup(dev, true);
enable_irq_wake(irq);
disable_irq_wake(irq);
```

Exact policy belongs in the power management chapter.

## Common Debug Flow

1. Confirm runtime Device Tree contains the interrupt property.
2. Confirm interrupt controller provider probed.
3. Confirm driver retrieved a Linux IRQ.
4. Confirm `request_irq()` or `request_threaded_irq()` succeeded.
5. Check `/proc/interrupts`.
6. Trigger the hardware event.
7. Check whether count increases.
8. Check whether handler logs or state changes.
9. Check trigger type and device acknowledgement if storming.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| IRQ not found in probe | missing/wrong `interrupts` property | runtime DT, binding |
| IRQ count never increases | wrong pinmux, trigger, controller, hardware line | `/proc/interrupts`, scope |
| IRQ storms | level condition not cleared or wrong polarity | device status, trigger |
| handler says spurious | shared IRQ and wrong status check | return values |
| sleeping warning | hard handler did sleepable work | threaded IRQ |
| wake from suspend fails | wake policy or power domain issue | wakeup settings |

## Common Mistakes

- Hard-coding Linux IRQ numbers.
- Treating GPIO line numbers as IRQ numbers.
- Sleeping in hard IRQ context.
- Returning `IRQ_HANDLED` for shared IRQs without checking device status.
- Forgetting to clear level-triggered interrupt condition.
- Putting I2C/SPI reads in a hard handler.
- Debugging handler code before confirming the interrupt count changes.

## Practice Exercises

### Exercise 1: Trace An IRQ

Find a device with an interrupt in Device Tree. Map:

```text
Device Tree interrupt specifier
-> driver platform_get_irq()
-> /proc/interrupts line
-> handler name
```

### Exercise 2: Trigger And Count

Trigger a known interrupt source and watch:

```sh
watch -n 0.5 cat /proc/interrupts
```

### Exercise 3: Identify Sleepable Work

Review an IRQ handler and list every operation that might sleep. Move those operations into a threaded handler or workqueue.

## Debugging Checklist

- Does runtime Device Tree contain the expected interrupt?
- Is the interrupt controller enabled and probed?
- Does probe retrieve a Linux IRQ?
- Did request succeed?
- Does `/proc/interrupts` show the handler?
- Does the interrupt count increase?
- Is the trigger type correct?
- Is the device interrupt source acknowledged correctly?
- Is sleepable work outside hard IRQ context?

## Related Topics

- [IRQ Handling](irq-handling.md)
- [Threaded Interrupts](threaded-interrupts.md)
- [Context Rules](../execution-and-concurrency/context-rules.md)
- [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [Wake Sources](../power-management/wake-sources.md)

## Official References

- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Unreliable Guide To Locking](https://docs.kernel.org/kernel-hacking/locking.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
