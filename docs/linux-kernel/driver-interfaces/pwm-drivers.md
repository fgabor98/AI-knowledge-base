---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# PWM Driver Overview

## What Problem Does This Solve?

PWM drivers and consumers control pulse-width modulated signals for backlights, motors, buzzers, regulators, fans, LEDs, and other hardware functions.

PWM support has two sides:

```text
PWM provider
  exposes hardware PWM channels

PWM consumer
  requests a PWM and applies period/duty/polarity/state
```

Most device drivers are consumers. Only timer/PWM-controller drivers are providers.

## Core Concepts

- PWM provider
- PWM consumer
- PWM chip
- PWM device
- period
- duty cycle
- polarity
- enable state
- `struct pwm_state`
- `devm_pwm_get()`
- `pwm_get_state()`
- `pwm_apply_might_sleep()`
- Device Tree PWM specifiers
- subsystem-specific consumers

## Mental Model

PWM hardware should be exposed through the PWM subsystem. Product drivers should consume a named PWM rather than program timer registers directly.

```dts
pwms = <&pwm0 0 20000000 PWM_POLARITY_NORMAL>;
```

Driver:

```c
pwm = devm_pwm_get(dev, NULL);
pwm_get_state(pwm, &state);
state.period = 20000000;
state.duty_cycle = 10000000;
state.enabled = true;
pwm_apply_might_sleep(pwm, &state);
```

Time units are usually nanoseconds in the PWM API.

## Device Tree Consumer Example

```dts
buzzer {
    compatible = "example,pwm-buzzer";
    pwms = <&pwm3 0 1000000 PWM_POLARITY_NORMAL>;
};
```

Backlight-style example:

```dts
backlight {
    compatible = "pwm-backlight";
    pwms = <&pwm1 0 50000 PWM_POLARITY_NORMAL>;
    brightness-levels = <0 4 8 16 32 64 128 255>;
    default-brightness-level = <5>;
};
```

If a standard PWM consumer driver exists, use it instead of writing a custom one.

## Consumer Driver Example

```c
struct demo_pwm {
    struct device *dev;
    struct pwm_device *pwm;
};

static int demo_probe(struct platform_device *pdev)
{
    struct demo_pwm *priv;
    struct pwm_state state;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;

    priv->pwm = devm_pwm_get(&pdev->dev, NULL);
    if (IS_ERR(priv->pwm))
        return dev_err_probe(&pdev->dev, PTR_ERR(priv->pwm),
                             "failed to get pwm\n");

    pwm_get_state(priv->pwm, &state);
    state.period = 1000000;
    state.duty_cycle = 500000;
    state.enabled = true;

    ret = pwm_apply_might_sleep(priv->pwm, &state);
    if (ret)
        return dev_err_probe(&pdev->dev, ret,
                             "failed to apply pwm state\n");

    return 0;
}
```

Some older kernel trees use `pwm_apply_state()`. Follow the API available in the tree you build against.

## Disable Safely

On remove or suspend:

```c
struct pwm_state state;

pwm_get_state(priv->pwm, &state);
state.enabled = false;
pwm_apply_might_sleep(priv->pwm, &state);
```

Some hardware requires duty cycle to be set to zero before disabling. Follow subsystem and hardware expectations.

## Duty Cycle And Period

Duty cycle must not exceed period:

```c
if (duty > period)
    return -EINVAL;
```

Common conversions:

```c
duty = DIV_ROUND_CLOSEST_ULL((u64)period * percent, 100);
```

Avoid overflow by using 64-bit arithmetic when multiplying nanosecond periods.

## Polarity

PWM polarity defines whether active time is high or low:

```c
state.polarity = PWM_POLARITY_NORMAL;
```

or:

```c
state.polarity = PWM_POLARITY_INVERSED;
```

Polarity may be fixed by board wiring or provider capability. Do not flip it casually in the driver when the binding already describes it.

## Sleepable Application

PWM providers may sleep when applying state. Use:

```c
pwm_apply_might_sleep()
```

from sleepable contexts.

Do not apply PWM state from hard IRQ context. Defer to a threaded IRQ or workqueue.

## Provider Versus Consumer

Consumer driver:

```text
backlight, buzzer, motor, regulator-like consumer
requests a PWM
```

Provider driver:

```text
SoC timer/PWM controller
registers PWM channels
```

Do not program PWM controller registers directly from a consumer driver unless that driver is itself the PWM provider.

## Existing Consumers

Before writing a custom driver, check for existing subsystem consumers:

- `pwm-backlight`
- `pwm-fan`
- PWM LEDs
- PWM regulators
- buzzer/feedback drivers

Using standard consumers gives userspace a standard ABI.

## Debugging PWM

Runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'pwms|pwm-' /tmp/running.dts
```

Debugfs may expose PWM state:

```sh
find /sys/kernel/debug -path '*pwm*' -type f 2>/dev/null
```

Electrical validation:

- scope
- logic analyzer
- verify pinmux
- verify period
- verify duty cycle
- verify polarity

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `devm_pwm_get()` fails | missing `pwms` property or provider | DT, provider driver |
| output pin silent | pinctrl not muxed to PWM | pinctrl debugfs |
| wrong frequency | period units/conversion wrong | scope, state |
| inverted behavior | polarity mismatch | DT flags, scope |
| apply fails | unsupported period/duty/rate | provider limits |
| suspend leaves output active | missing disable or sleep handling | PM callbacks |

## Common Mistakes

- Confusing duty cycle percentage with nanoseconds.
- Letting duty cycle exceed period.
- Programming timer registers directly in a consumer driver.
- Ignoring pinctrl.
- Applying PWM state from atomic context.
- Writing custom driver when `pwm-backlight` or another standard consumer fits.
- Not defining safe disabled state.

## Practice Exercises

### Exercise 1: Consume A PWM

Add a `pwms` property to a dummy node, request it, and apply a 50 percent duty cycle.

### Exercise 2: Measure The Output

Use a scope or logic analyzer to confirm period, duty, and polarity.

### Exercise 3: Replace Custom Control

For a backlight or fan, compare a custom consumer with the standard PWM subsystem driver.

## Debugging Checklist

- Does runtime Device Tree contain `pwms`?
- Did the PWM provider probe?
- Is pinctrl selecting the PWM function?
- Are period and duty in nanoseconds?
- Is polarity correct?
- Does the provider support the requested state?
- Is state disabled safely during remove/suspend?
- Is an existing subsystem consumer more appropriate?

## Related Topics

- [Pinctrl](pinctrl.md)
- [Device Tree Hardware Description](../fundamentals/device-tree-hardware-description.md)
- [Power Management](../power-management/index.md)

## Official References

- [Pulse Width Modulation (PWM) interface](https://docs.kernel.org/driver-api/pwm.html)
