---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IRQ Handling

## What Problem Does This Solve?

Interrupt handling lets hardware notify the kernel about events without polling.

A driver uses IRQ handling when it needs to react to:

- data ready
- transfer complete
- FIFO threshold
- error condition
- GPIO input edge
- button press
- device fault
- wakeup event

The handler must be fast, context-correct, and matched to the device's interrupt semantics.

## Core Concepts

- IRQ number
- hard IRQ handler
- top half
- interrupt context
- `request_irq()`
- `devm_request_irq()`
- `request_threaded_irq()`
- `devm_request_threaded_irq()`
- `irqreturn_t`
- `IRQ_HANDLED`
- `IRQ_NONE`
- `IRQ_WAKE_THREAD`
- interrupt flags
- shared IRQs
- trigger type
- wake IRQs
- `/proc/interrupts`

## Mental Model

The hard IRQ handler must do the smallest safe amount of work:

```text
interrupt fires
-> check whether device caused it
-> acknowledge or mask the source if required
-> capture minimal state
-> wake thread/work/wait queue if needed
-> return
```

Slow or sleepable work belongs elsewhere.

## Getting An IRQ

Platform driver:

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return dev_err_probe(&pdev->dev, irq, "failed to get irq\n");
```

Named IRQ:

```c
irq = platform_get_irq_byname(pdev, "data-ready");
if (irq < 0)
    return dev_err_probe(&pdev->dev, irq,
                         "failed to get data-ready irq\n");
```

GPIO-backed IRQ:

```c
irq_gpio = devm_gpiod_get(dev, "irq", GPIOD_IN);
if (IS_ERR(irq_gpio))
    return dev_err_probe(dev, PTR_ERR(irq_gpio),
                         "failed to get irq gpio\n");

irq = gpiod_to_irq(irq_gpio);
if (irq < 0)
    return dev_err_probe(dev, irq, "failed to map gpio irq\n");
```

## Minimal Hard IRQ Handler

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    u32 status;

    status = readl(priv->base + DEMO_STATUS);
    if (!(status & DEMO_STATUS_IRQ))
        return IRQ_NONE;

    writel(DEMO_STATUS_IRQ, priv->base + DEMO_STATUS);
    priv->irq_count++;

    return IRQ_HANDLED;
}
```

Request:

```c
ret = devm_request_irq(dev, irq, demo_irq, 0,
                       dev_name(dev), priv);
if (ret)
    return dev_err_probe(dev, ret, "failed to request irq\n");
```

This is appropriate only if status access and acknowledgement are non-sleeping.

## Handler Return Values

| Return | Use |
| --- | --- |
| `IRQ_HANDLED` | The device caused and handled the interrupt. |
| `IRQ_NONE` | The interrupt was not from this device. Important for shared lines. |
| `IRQ_WAKE_THREAD` | Wake the threaded handler. |

For shared IRQs, always check device status before returning `IRQ_HANDLED`.

Bad:

```c
return IRQ_HANDLED; /* without checking the device */
```

This hides interrupt routing problems and can break shared IRQ diagnostics.

## IRQ Flags

Common flags:

| Flag | Meaning |
| --- | --- |
| `IRQF_SHARED` | Allow line sharing. Handler must detect ownership. |
| `IRQF_ONESHOT` | Keep interrupt masked until threaded handler finishes. |
| `IRQF_TRIGGER_RISING` | Rising-edge trigger. |
| `IRQF_TRIGGER_FALLING` | Falling-edge trigger. |
| `IRQF_TRIGGER_HIGH` | Level-high trigger. |
| `IRQF_TRIGGER_LOW` | Level-low trigger. |
| `IRQF_NO_AUTOEN` | Do not enable immediately after request. |

Prefer trigger type from firmware data where possible. Do not encode conflicting trigger type in both Device Tree and request flags.

## What Hard IRQ Handlers Must Not Do

Do not call:

- `msleep()`
- `mutex_lock()`
- `i2c_transfer()`
- `spi_sync()`
- `regmap_read()` over sleeping buses
- `copy_to_user()`
- blocking allocation with `GFP_KERNEL`
- long loops

Hard IRQ handlers run with severe context restrictions. Use threaded interrupts when in doubt.

## Acknowledging The Device

Many devices require clearing interrupt status:

```c
status = readl(base + STATUS);
writel(status & IRQ_BITS, base + STATUS);
```

Some devices clear on read. Some use write-one-to-clear. Some require clearing multiple registers in a documented order.

If a level-triggered interrupt condition remains asserted when the handler returns, the IRQ will fire again immediately.

Always read the hardware manual.

## Shared IRQs

For shared IRQs:

```c
ret = devm_request_irq(dev, irq, demo_irq, IRQF_SHARED,
                       dev_name(dev), priv);
```

Handler:

```c
if (!(status & DEMO_STATUS_IRQ))
    return IRQ_NONE;
```

The `dev_id` pointer must be unique per registered action. It is used for freeing and identifying the handler.

## Disabling And Enabling IRQs

Sometimes a driver needs to mask interrupts temporarily:

```c
disable_irq(irq);
enable_irq(irq);
```

`disable_irq()` waits for running handlers and can sleep. `disable_irq_nosync()` does not wait.

Use carefully. Often it is better to mask the interrupt source in the device's own registers.

## Wake IRQs

If the interrupt should wake the system:

```c
device_init_wakeup(dev, true);
```

During suspend:

```c
enable_irq_wake(irq);
```

During resume:

```c
disable_irq_wake(irq);
```

Wake policy also depends on Device Tree, power domains, pinctrl sleep states, and system policy.

## Debugging IRQs

Check mapping:

```sh
cat /proc/interrupts
```

Check per-IRQ details:

```sh
ls /proc/irq/<irq>
cat /proc/irq/<irq>/spurious
```

Check logs:

```sh
dmesg | grep -i -E 'irq|interrupt|spurious'
```

Check runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'interrupts|interrupt-parent' /tmp/running.dts
```

Use a scope or logic analyzer when the electrical line itself is suspect.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `platform_get_irq()` fails | missing interrupt property | runtime DT |
| `request_irq()` fails with busy | line already requested or sharing mismatch | `/proc/interrupts` |
| IRQ count never changes | wrong pinmux, trigger, controller, hardware | `/proc/interrupts`, scope |
| interrupt storm | status not cleared or wrong level polarity | device status register |
| "nobody cared" message | handler returned `IRQ_NONE` repeatedly | status check, routing |
| sleeping warning | hard handler called sleepable API | threaded IRQ |

## Common Mistakes

- Performing I2C/SPI transfers in hard IRQ context.
- Returning `IRQ_HANDLED` without checking status on shared lines.
- Forgetting to clear a level-triggered interrupt.
- Requesting the wrong named IRQ.
- Confusing GPIO offset with IRQ number.
- Logging every interrupt with `dev_info()`.
- Ignoring interrupt trigger polarity in Device Tree.

## Practice Exercises

### Exercise 1: Count Interrupts

Add an atomic or protected counter in an IRQ handler and expose it through debug logs or a low-rate sysfs attribute.

### Exercise 2: Convert To Threaded IRQ

Move a sleepable status read out of the hard handler into a threaded handler.

### Exercise 3: Debug A Missing IRQ

Trace from Device Tree to `platform_get_irq()` to `/proc/interrupts`, then trigger the hardware.

## Debugging Checklist

- Does the driver retrieve the expected IRQ?
- Does request succeed?
- Is the trigger type correct?
- Does `/proc/interrupts` show the handler name?
- Does the count increase?
- Does the handler acknowledge the device correctly?
- Does the handler avoid sleeping?
- Are logs rate-limited?
- If shared, does the handler return `IRQ_NONE` when appropriate?

## Related Topics

- [Interrupt Processing Model](interrupt-processing-model.md)
- [Threaded Interrupts](threaded-interrupts.md)
- [Context Rules](../execution-and-concurrency/context-rules.md)
- [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [Wake Sources](../power-management/wake-sources.md)

## Official References

- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
