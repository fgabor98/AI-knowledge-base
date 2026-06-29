---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Input Subsystem

## What Problem Does This Solve?

The input subsystem exposes buttons, keys, touch devices, switches, relative axes, absolute axes, and similar event-producing devices through a common userspace event interface.

Use input when the device represents human or system input events:

- key press
- button release
- lid open/close switch
- touchscreen coordinate
- rotary encoder movement
- accelerometer used as an input control

Userspace receives events through:

```text
/dev/input/event*
```

## Core Concepts

- input device
- `struct input_dev`
- event types
- key codes
- switch codes
- absolute axes
- relative axes
- `input_set_capability()`
- `input_report_key()`
- `input_report_switch()`
- `input_report_abs()`
- `input_sync()`
- `/dev/input/event*`
- `evtest`

## Mental Model

Input drivers report semantic events, not raw electrical transitions.

```text
GPIO went low
  -> not the ABI

KEY_ENTER pressed
  -> input event ABI
```

The driver translates hardware state into input events.

## Minimal Input Device

```c
struct demo_input {
    struct device *dev;
    struct input_dev *input;
};

static int demo_probe(struct platform_device *pdev)
{
    struct demo_input *priv;
    struct input_dev *input;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    input = devm_input_allocate_device(&pdev->dev);
    if (!input)
        return -ENOMEM;

    priv->dev = &pdev->dev;
    priv->input = input;

    input->name = "demo-button";
    input->phys = "demo/input0";
    input->id.bustype = BUS_HOST;

    input_set_capability(input, EV_KEY, KEY_ENTER);

    ret = input_register_device(input);
    if (ret)
        return ret;

    return 0;
}
```

Report:

```c
input_report_key(priv->input, KEY_ENTER, 1);
input_sync(priv->input);

input_report_key(priv->input, KEY_ENTER, 0);
input_sync(priv->input);
```

## Event Types

Common event types:

| Type | Use |
| --- | --- |
| `EV_KEY` | Keys and buttons. |
| `EV_SW` | Switch state such as lid open/closed. |
| `EV_REL` | Relative movement such as mouse delta. |
| `EV_ABS` | Absolute axes such as touch coordinates. |
| `EV_MSC` | Miscellaneous events. |
| `EV_LED` | Keyboard LEDs. |
| `EV_REP` | Repeat settings. |

Use the type that matches the semantic meaning.

## Keys And Buttons

Set capability:

```c
input_set_capability(input, EV_KEY, KEY_POWER);
```

Report:

```c
input_report_key(input, KEY_POWER, pressed);
input_sync(input);
```

Report both press and release. Userspace expects state transitions.

## Switches

For a lid or mode switch:

```c
input_set_capability(input, EV_SW, SW_LID);
input_report_switch(input, SW_LID, closed);
input_sync(input);
```

Switches represent state, not momentary key actions.

## Absolute Axes

For touch/position:

```c
input_set_abs_params(input, ABS_X, 0, 1023, 0, 0);
input_set_abs_params(input, ABS_Y, 0, 1023, 0, 0);
```

Report:

```c
input_report_abs(input, ABS_X, x);
input_report_abs(input, ABS_Y, y);
input_sync(input);
```

Use multitouch helpers for real multitouch devices.

## Device Tree Key Codes

Many input devices use standard bindings that include Linux key codes:

```dts
button-user {
    label = "user";
    gpios = <&gpio1 12 GPIO_ACTIVE_LOW>;
    linux,code = <KEY_ENTER>;
};
```

If a standard driver such as `gpio-keys` fits, use it instead of writing a custom input driver.

## Testing With `evtest`

List devices:

```sh
cat /proc/bus/input/devices
ls /dev/input
```

Run:

```sh
evtest /dev/input/event0
```

Watch events:

```text
type 1 (EV_KEY), code 28 (KEY_ENTER), value 1
type 1 (EV_KEY), code 28 (KEY_ENTER), value 0
type 0 (EV_SYN), code 0 (SYN_REPORT), value 0
```

If events are missing, check whether the driver calls `input_sync()`.

## Debounce

Mechanical buttons bounce. Solutions include:

- hardware debounce
- GPIO controller debounce support
- delayed work in driver
- standard `gpio-keys` debounce support
- polling interval choice

Do not report every electrical bounce as a user event.

## Power And Wakeup

Input devices are often wake sources:

```c
device_init_wakeup(dev, true);
```

For GPIO/IRQ-backed buttons, coordinate:

- IRQ wake enable
- pinctrl sleep state
- power domain state
- debounce after resume

Wake policy belongs in product requirements, not accidental driver behavior.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no event node | input device not registered | probe logs |
| events not visible | permissions or wrong event device | `evtest`, udev |
| press but no release | driver reports only one edge | event logic |
| repeated noise | missing debounce | hardware/driver debounce |
| wrong key | wrong key code or DT property | `linux,code` |
| userspace ignores event | wrong event type | EV_KEY vs EV_SW |

## Common Mistakes

- Reporting raw GPIO state instead of semantic input events.
- Forgetting `input_sync()`.
- Reporting only press or only release.
- Using custom character devices for buttons.
- Writing a custom driver when `gpio-keys` or another standard driver fits.
- Ignoring debounce.
- Using key codes that do not match product behavior.

## Practice Exercises

### Exercise 1: Dummy Key

Register an input device that reports `KEY_ENTER` from a timer or test path. Observe with `evtest`.

### Exercise 2: GPIO Button

Use a GPIO descriptor and report press/release state through input.

### Exercise 3: Switch Versus Key

Model the same hardware as `EV_KEY` and `EV_SW` in a design review. Decide which semantic is correct.

## Debugging Checklist

- Does `/proc/bus/input/devices` show the device?
- Is the correct event type/code advertised?
- Does the driver report both state changes?
- Is `input_sync()` called after reports?
- Is debounce handled?
- Are permissions correct for userspace?
- Is a standard input driver already available?

## Related Topics

- [Polled Input Devices](polled-input-devices.md)
- [IRQ-Based Input Devices](irq-based-input-devices.md)
- [GPIO Consumer API](gpio-consumer-api.md)
- [Threaded Interrupts](threaded-interrupts.md)

## Official References

- [Input Subsystem](https://docs.kernel.org/driver-api/input.html)
- [Event codes](https://docs.kernel.org/input/event-codes.html)
