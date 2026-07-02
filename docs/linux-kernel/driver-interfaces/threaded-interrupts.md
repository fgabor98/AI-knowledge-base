---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Threaded Interrupts

## What Problem Does This Solve?

Threaded interrupts let drivers handle interrupt-triggered work in a kernel thread where sleeping operations are allowed.

Use them when interrupt response needs:

- I2C or SPI transfers
- regmap access over a sleeping bus
- mutexes
- GPIO access through sleeping providers
- input event reporting after debounce
- IIO event processing
- longer device-status handling

## Core Concepts

- hard IRQ handler
- threaded IRQ handler
- `request_threaded_irq()`
- `devm_request_threaded_irq()`
- `IRQ_WAKE_THREAD`
- `IRQF_ONESHOT`
- sleepable context
- interrupt masking
- threaded handler latency
- IRQ thread priority

## Mental Model

Split interrupt work into two parts:

```text
hard handler:
  must not sleep
  checks ownership if needed
  acknowledges or masks urgent hardware state
  returns IRQ_WAKE_THREAD

thread handler:
  may sleep
  reads status over I2C/SPI
  takes mutexes
  reports events
  re-enables device state
```

If there is no useful hard handler, pass `NULL` as the hard handler and use `IRQF_ONESHOT`.

## Basic Threaded IRQ

```c
static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_priv *priv = data;
    int ret;
    unsigned int status;

    ret = regmap_read(priv->regmap, DEMO_STATUS, &status);
    if (ret)
        return IRQ_HANDLED;

    if (status & DEMO_STATUS_DATA_READY)
        demo_handle_data(priv);

    return IRQ_HANDLED;
}
```

Request:

```c
ret = devm_request_threaded_irq(dev, irq,
                                NULL, demo_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(dev), priv);
if (ret)
    return dev_err_probe(dev, ret, "failed to request irq\n");
```

With a `NULL` hard handler, the core uses a default primary handler that wakes the thread. `IRQF_ONESHOT` keeps the line masked until the thread completes where required.

## Hard Handler Plus Thread

For MMIO devices, a hard handler may quickly check status:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
    struct demo_priv *priv = data;
    u32 status;

    status = readl(priv->base + DEMO_STATUS);
    if (!(status & DEMO_IRQ_PENDING))
        return IRQ_NONE;

    writel(DEMO_IRQ_PENDING, priv->base + DEMO_STATUS);
    priv->pending_status = status;

    return IRQ_WAKE_THREAD;
}
```

Thread:

```c
static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_priv *priv = data;

    demo_process_status(priv, priv->pending_status);
    return IRQ_HANDLED;
}
```

Request:

```c
ret = devm_request_threaded_irq(dev, irq,
                                demo_irq, demo_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(dev), priv);
```

Protect shared state between hard handler and thread if needed.

## `IRQF_ONESHOT`

Use `IRQF_ONESHOT` when the interrupt line must remain masked while the thread runs.

This is common for:

- level-triggered interrupts
- GPIO interrupts
- sleeping bus devices
- `NULL` primary handler threaded IRQs

Without oneshot behavior, the interrupt can retrigger while the thread is still handling the previous event.

## I2C/SPI Device Example

For a sensor interrupt:

```c
static irqreturn_t sensor_irq_thread(int irq, void *data)
{
    struct sensor_priv *priv = data;
    unsigned int status;
    int ret;

    ret = regmap_read(priv->regmap, SENSOR_STATUS, &status);
    if (ret)
        return IRQ_HANDLED;

    if (status & SENSOR_DATA_READY)
        iio_trigger_poll(priv->trig);

    if (status & SENSOR_FAULT)
        dev_warn(priv->dev, "fault status %#x\n", status);

    return IRQ_HANDLED;
}
```

I2C/SPI transfers can sleep, so they belong in the thread, not a hard IRQ handler.

## Button Example

```c
static irqreturn_t button_irq_thread(int irq, void *data)
{
    struct button_priv *priv = data;
    int pressed;

    pressed = gpiod_get_value_cansleep(priv->gpio);
    input_report_key(priv->input, KEY_ENTER, pressed);
    input_sync(priv->input);

    return IRQ_HANDLED;
}
```

This handles GPIO expanders and debounce-friendly paths better than a hard handler with direct GPIO reads.

## Locking

Threaded handlers run in process context, so they may use mutexes:

```c
mutex_lock(&priv->lock);
demo_update_state(priv);
mutex_unlock(&priv->lock);
```

But be careful with lock ordering. The same lock may be used by:

- sysfs store callbacks
- read/write file operations
- runtime PM callbacks
- workqueue functions
- remove path

Avoid deadlocks between IRQ thread and remove/suspend paths.

## Disabling Hardware Interrupt Sources

Even with `IRQF_ONESHOT`, the device may keep asserting an interrupt condition. The threaded handler often must clear or mask device status:

```c
ret = regmap_write(priv->regmap, DEMO_IRQ_STATUS, status);
```

If the device requires a specific acknowledge order, follow the hardware manual.

## Threaded IRQ Versus Workqueue

Use threaded IRQ when:

- work directly corresponds to the interrupt
- you need serialized handling per IRQ
- you want IRQ masking semantics
- latency should be near the interrupt event

Use a workqueue when:

- work is not strictly tied to one interrupt invocation
- work may be coalesced
- retries or longer processing are needed
- you need a custom work execution model

Both are sleepable contexts. The lifetime and ordering rules differ.

## Debugging Threaded IRQs

Check `/proc/interrupts`:

```sh
cat /proc/interrupts
```

Look for handler names and counts.

Kernel threads may appear as IRQ threads:

```sh
ps -eLo pid,tid,comm | grep irq
```

Check logs:

```sh
dmesg | grep -i -E 'irq|thread|demo'
```

Watch storming:

```sh
watch -n 0.5 cat /proc/interrupts
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| thread never runs | hard handler did not return `IRQ_WAKE_THREAD` | return value |
| interrupt storm | device condition not cleared | status/ack sequence |
| sleep warning remains | sleepable work still in hard handler | handler audit |
| high latency | thread blocked on locks or long work | lock ordering, traces |
| missing events | status cleared too early or edge lost | hardware sequence |
| remove hangs | IRQ thread blocked or active | disable hardware, synchronization |

## Common Mistakes

- Passing a thread function but returning `IRQ_HANDLED` from the hard handler when the thread should run.
- Omitting `IRQF_ONESHOT` for a `NULL` primary handler or level-triggered device.
- Doing unbounded work in the IRQ thread.
- Forgetting that threaded handlers can still race with remove/suspend.
- Clearing interrupt status before reading all needed event state.
- Using a threaded IRQ to hide unclear hardware acknowledgement semantics.

## Practice Exercises

### Exercise 1: Convert An I2C IRQ

Take a hard handler that reads an I2C register and move the register read into a threaded handler.

### Exercise 2: Test Storm Behavior

Temporarily skip the device status clear in a lab driver and observe `/proc/interrupts`. Restore the clear immediately.

### Exercise 3: Add Rate-Limited Diagnostics

Add `dev_warn_ratelimited()` for unexpected status bits in the threaded handler.

## Debugging Checklist

- Does the IRQ count increase?
- Does the hard handler return `IRQ_WAKE_THREAD` when appropriate?
- Is `IRQF_ONESHOT` used where needed?
- Does the thread clear or mask the device condition?
- Are sleepable operations only in the thread?
- Are locks ordered consistently with sysfs/remove/runtime PM?
- Is the handler bounded and rate-limited?

## Related Topics

- [IRQ Handling](irq-handling.md)
- [Sleepable Vs Atomic Code](../execution-and-concurrency/sleepable-vs-atomic-code.md)
- [Workqueues](../execution-and-concurrency/workqueues.md)
- [I2C Client Drivers](i2c-client-drivers.md)
- [SPI Device Drivers](spi-device-drivers.md)

## Official References

- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
