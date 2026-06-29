---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Polled Input Devices

## What Problem Does This Solve?

Polled input drivers support devices that do not have a usable interrupt line or where periodic sampling is acceptable.

Polling is simple but costs periodic CPU wakeups and adds latency. Prefer interrupts when hardware supports them cleanly.

## Core Concepts

- polling interval
- input polling helper
- GPIO button polling
- debounce
- event reporting
- CPU cost
- latency tradeoff
- `input_setup_polling()`
- `input_set_poll_interval()`

## Mental Model

Polling samples state periodically:

```text
timer/work fires every N ms
-> read hardware state
-> compare with previous state
-> report input event if changed
```

Polling reports semantic input events, not raw reads.

## Minimal Shape

```c
struct demo_button {
    struct device *dev;
    struct input_dev *input;
    struct gpio_desc *gpio;
    bool last_pressed;
};

static void demo_poll(struct input_dev *input)
{
    struct demo_button *btn = input_get_drvdata(input);
    bool pressed;

    pressed = gpiod_get_value_cansleep(btn->gpio);
    if (pressed == btn->last_pressed)
        return;

    btn->last_pressed = pressed;
    input_report_key(input, KEY_ENTER, pressed);
    input_sync(input);
}
```

Probe:

```c
input = devm_input_allocate_device(dev);
if (!input)
    return -ENOMEM;

btn->input = input;
input_set_drvdata(input, btn);

input->name = "demo-polled-button";
input_set_capability(input, EV_KEY, KEY_ENTER);

input_setup_polling(input, demo_poll);
input_set_poll_interval(input, 20);

ret = input_register_device(input);
```

Check your kernel tree for exact helper availability and signatures.

## Choosing Poll Interval

Tradeoff:

| Interval | Effect |
| --- | --- |
| shorter | lower latency, more CPU wakeups |
| longer | lower overhead, more latency |

For human buttons, 10 to 50 ms is often reasonable depending on debounce and product feel. For sensors, IIO may be a better subsystem.

## Debounce

Polling naturally filters some bounce if interval is long enough, but not always.

Strategies:

- require stable state across multiple polls
- use hardware debounce
- use GPIO controller debounce if available
- use standard `gpio-keys-polled` style drivers where appropriate

Example stable-count idea:

```text
if sampled state changed:
  increment candidate count
  accept only after N identical samples
```

## GPIO Access

Use `gpiod_get_value_cansleep()` if the GPIO may come from an expander:

```c
pressed = gpiod_get_value_cansleep(btn->gpio);
```

Do not use hard IRQ assumptions in polling code. Poll callbacks run in sleepable context in the common helper model.

## Standard Drivers

For simple GPIO buttons, check existing drivers and bindings before writing a custom one:

- `gpio-keys`
- `gpio-keys-polled`

Custom drivers are appropriate when the device has nonstandard protocol, aggregation, or product-specific behavior not covered by existing drivers.

## Debugging Polled Input

Input devices:

```sh
cat /proc/bus/input/devices
evtest /dev/input/eventX
```

GPIO state:

```sh
gpioinfo
```

Power impact:

```sh
powertop
```

Logs:

```sh
dmesg | grep -i input
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| event latency too high | polling interval too long | interval |
| CPU wakeups too high | interval too short | powertop |
| repeated events | debounce missing | state comparison |
| no release event | previous state not tracked | logic |
| GPIO read warning | wrong accessor for sleeping provider | cansleep |
| wrong semantics | should be IRQ or standard driver | design |

## Common Mistakes

- Polling hardware that has a clean interrupt line.
- Reporting events every poll instead of only on change.
- Forgetting `input_sync()`.
- Ignoring debounce.
- Polling too fast for no product reason.
- Using custom polled driver when `gpio-keys-polled` fits.

## Practice Exercises

### Exercise 1: Polled GPIO Button

Use a GPIO descriptor, poll every 20 ms, and report `KEY_ENTER` only on state changes.

### Exercise 2: Debounce

Add a two-sample stable-state filter. Compare events with `evtest`.

### Exercise 3: Poll Interval Tradeoff

Measure subjective latency and CPU wakeups at 10 ms, 50 ms, and 200 ms.

## Debugging Checklist

- Is polling necessary, or is IRQ better?
- Is the polling interval justified?
- Does the driver report only state changes?
- Does debounce prevent noisy events?
- Does it report both press and release?
- Does it use cansleep GPIO access where needed?
- Is a standard driver available?

## Related Topics

- [Input Subsystem](input-subsystem.md)
- [GPIO Consumer API](gpio-consumer-api.md)
- [Timers](../execution-and-concurrency/timers.md)
- [Workqueues](../execution-and-concurrency/workqueues.md)

## Official References

- [Input Subsystem](https://docs.kernel.org/driver-api/input.html)
- [Event codes](https://docs.kernel.org/input/event-codes.html)
