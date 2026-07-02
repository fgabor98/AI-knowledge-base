---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IRQ-Based Input Devices

## What Problem Does This Solve?

IRQ-based input drivers report input events in response to hardware interrupts, commonly from buttons, switches, touch controllers, encoders, or ready signals.

Use IRQ-based input when hardware can signal state changes reliably and latency or power matters.

## Core Concepts

- GPIO IRQ
- interrupt trigger type
- threaded IRQ
- debounce
- `input_report_key()`
- `input_report_switch()`
- `input_sync()`
- wakeup source
- event device
- press and release reporting

## Mental Model

Use the interrupt to detect a possible state change, then report the semantic input event through the input subsystem.

```text
GPIO edge
-> IRQ thread reads debounced state
-> input_report_key(KEY_ENTER, pressed)
-> input_sync()
-> userspace reads /dev/input/eventX
```

The IRQ is not the userspace ABI. The input event is.

## Device Tree Button Example

A standard binding may use `gpio-keys`:

```dts
gpio-keys {
    compatible = "gpio-keys";

    button-user {
        label = "user";
        gpios = <&gpio1 12 GPIO_ACTIVE_LOW>;
        linux,code = <KEY_ENTER>;
        debounce-interval = <10>;
        wakeup-source;
    };
};
```

Use standard `gpio-keys` when it fits. Write a custom driver only for nonstandard behavior.

## Custom IRQ-Based Button Shape

Private state:

```c
struct demo_button {
    struct device *dev;
    struct input_dev *input;
    struct gpio_desc *gpio;
    int irq;
    bool last_pressed;
};
```

IRQ thread:

```c
static irqreturn_t demo_button_irq(int irq, void *data)
{
    struct demo_button *btn = data;
    bool pressed;

    pressed = gpiod_get_value_cansleep(btn->gpio);
    if (pressed != btn->last_pressed) {
        btn->last_pressed = pressed;
        input_report_key(btn->input, KEY_ENTER, pressed);
        input_sync(btn->input);
    }

    return IRQ_HANDLED;
}
```

Probe:

```c
btn->gpio = devm_gpiod_get(dev, "button", GPIOD_IN);
if (IS_ERR(btn->gpio))
    return dev_err_probe(dev, PTR_ERR(btn->gpio),
                         "failed to get button gpio\n");

btn->irq = gpiod_to_irq(btn->gpio);
if (btn->irq < 0)
    return dev_err_probe(dev, btn->irq, "failed to map irq\n");

input = devm_input_allocate_device(dev);
if (!input)
    return -ENOMEM;

btn->input = input;
input->name = "demo-irq-button";
input_set_capability(input, EV_KEY, KEY_ENTER);

ret = input_register_device(input);
if (ret)
    return ret;

ret = devm_request_threaded_irq(dev, btn->irq,
                                NULL, demo_button_irq,
                                IRQF_TRIGGER_RISING |
                                IRQF_TRIGGER_FALLING |
                                IRQF_ONESHOT,
                                dev_name(dev), btn);
```

Use trigger flags from Device Tree where possible instead of hard-coding conflicting flags.

## Debounce

Mechanical buttons bounce. Options:

- hardware debounce
- GPIO controller debounce
- delayed work after IRQ
- ignore changes until stable
- standard `gpio-keys` debounce support

Simple delayed-work model:

```text
IRQ fires
-> schedule delayed work for debounce interval
-> delayed work reads GPIO and reports stable state
```

This avoids reporting every edge.

## Wakeup

For wake-capable buttons:

```c
device_init_wakeup(dev, true);
```

Suspend:

```c
if (device_may_wakeup(dev))
    enable_irq_wake(btn->irq);
```

Resume:

```c
if (device_may_wakeup(dev))
    disable_irq_wake(btn->irq);
```

Also check pinctrl sleep state and power domains. A wake IRQ cannot fire if the pin or controller is powered down incorrectly.

## Trigger Type

Buttons often need both edges:

```text
press edge
release edge
```

If only one edge is available, the driver may need to read state and synthesize release through polling or delayed work, but that is less ideal.

Level-triggered lines require the condition to be cleared or changed before returning from the handler.

## Testing

Input:

```sh
cat /proc/bus/input/devices
evtest /dev/input/eventX
```

Interrupts:

```sh
cat /proc/interrupts
```

GPIO:

```sh
gpioinfo
```

Wake:

```sh
cat /sys/devices/.../power/wakeup
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| press detected but release missing | only one edge configured | trigger type |
| noisy repeated events | no debounce | delayed work/hardware debounce |
| IRQ count increases but no input event | report logic or missing `input_sync()` | evtest, logs |
| no IRQ count | pinmux/trigger/controller issue | `/proc/interrupts`, scope |
| wake fails | wake IRQ not enabled or pinctrl sleep issue | wake settings |
| sleep warning | GPIO expander read in hard handler | threaded IRQ |

## Common Mistakes

- Reporting raw IRQ count instead of semantic key state.
- Forgetting release events.
- Forgetting `input_sync()`.
- Ignoring debounce.
- Reading sleeping GPIOs in hard IRQ context.
- Hard-coding trigger type that conflicts with Device Tree.
- Writing custom button driver when `gpio-keys` fits.

## Practice Exercises

### Exercise 1: IRQ Button

Use a GPIO as an input button and report `KEY_ENTER` through the input subsystem.

### Exercise 2: Add Debounce

Move reporting to delayed work and require a stable value after 10 ms.

### Exercise 3: Test Wake

Mark the button as wake-capable and test suspend/resume behavior.

## Debugging Checklist

- Does the GPIO line read correctly?
- Does it map to an IRQ?
- Does `/proc/interrupts` count change?
- Is the handler threaded if GPIO access can sleep?
- Are both press and release reported?
- Is debounce handled?
- Does `evtest` show the expected key code?
- Is wake policy configured intentionally?

## Related Topics

- [Input Subsystem](input-subsystem.md)
- [Threaded Interrupts](threaded-interrupts.md)
- [GPIO Consumer API](gpio-consumer-api.md)
- [Wake Sources](../power-management/wake-sources.md)

## Official References

- [Input Subsystem](https://docs.kernel.org/driver-api/input.html)
- [Event codes](https://docs.kernel.org/input/event-codes.html)
- [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
