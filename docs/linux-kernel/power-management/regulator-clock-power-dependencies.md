---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Regulator And Clock Power Dependencies

## What Problem Does This Solve?

Most hardware blocks are not made usable by a single enable bit. A device may
need several dependencies to be valid before register access, bus traffic, DMA,
or interrupts are safe.

Common dependencies:

```text
power domain
regulators
clocks
resets
pinctrl states
GPIO enables
PHYs
interconnect bandwidth
firmware-owned power sequence
startup delays
```

A driver fails when it controls one dependency while accidentally assuming the
others are already correct.

Example:

```text
driver enables clock
driver reads ID register
  -> regulator was never enabled
  -> bus fault
```

or:

```text
driver enables regulator and clock
driver reads ID register
  -> reset line still asserted
  -> read returns zero
```

This page gives a practical sequencing model for driver power-up, power-down,
runtime PM, system sleep, and probe failure handling.

## The Hardware Manual Owns The Order

There is no universal power sequence. The datasheet and board design decide the
correct order.

Common sequence:

```text
enable regulators
wait for rails to settle
prepare and enable clocks
deassert reset
wait for startup
select default pins
access registers
```

Another valid sequence for different hardware:

```text
assert reset
enable regulators
wait for rail
select default pins
enable reference clock
deassert reset
wait for firmware boot
access registers
```

Another:

```text
power domain on
enable bus clock
enable functional clock
release isolation through provider
deassert reset
```

Do not invent delays or order from guesswork. Start from:

- SoC reference manual
- device datasheet
- board schematic
- Device Tree or ACPI binding
- subsystem examples
- vendor downstream driver, if available
- measured hardware signals when the documentation is incomplete

## Dependency Types

### Regulators

Regulators provide power rails.

Device Tree:

```dts
sensor@48 {
    compatible = "example,sensor";
    reg = <0x48>;
    vdd-supply = <&vdd_3v3>;
    vddio-supply = <&vddio_1v8>;
};
```

Driver:

```c
static const char * const demo_supply_names[] = {
    "vdd",
    "vddio",
};

for (i = 0; i < ARRAY_SIZE(demo_supply_names); i++)
    priv->supplies[i].supply = demo_supply_names[i];

ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(demo_supply_names),
                              priv->supplies);
if (ret)
    return dev_err_probe(dev, ret, "failed to get supplies\n");
```

Enable:

```c
ret = regulator_bulk_enable(ARRAY_SIZE(demo_supply_names), priv->supplies);
if (ret)
    return ret;
```

Disable:

```c
regulator_bulk_disable(ARRAY_SIZE(demo_supply_names), priv->supplies);
```

Getting a regulator does not necessarily enable it. Optional supplies should
only be optional if the binding says they are optional.

### Clocks

Clocks provide timing for register access, buses, functional logic, or external
interfaces.

Device Tree:

```dts
spi@10040000 {
    compatible = "example,spi";
    reg = <0x10040000 0x1000>;
    clocks = <&clk SPI_BUS>, <&clk SPI_CORE>;
    clock-names = "bus", "core";
};
```

Driver state:

```c
struct demo_priv {
    struct clk_bulk_data clks[2];
};
```

Resource acquisition:

```c
priv->clks[0].id = "bus";
priv->clks[1].id = "core";

ret = devm_clk_bulk_get(dev, ARRAY_SIZE(priv->clks), priv->clks);
if (ret)
    return dev_err_probe(dev, ret, "failed to get clocks\n");
```

Enable:

```c
ret = clk_bulk_prepare_enable(ARRAY_SIZE(priv->clks), priv->clks);
if (ret)
    return ret;
```

Disable:

```c
clk_bulk_disable_unprepare(ARRAY_SIZE(priv->clks), priv->clks);
```

Clock prepare may sleep. Clock enable may be usable from atomic context for some
framework paths, but consumer drivers should generally use
`clk_prepare_enable()` from sleepable probe/PM paths unless they have a specific
reason to split prepare and enable.

### Resets

Reset lines hold hardware in a known state or release it after power is valid.

Device Tree:

```dts
demo@10000000 {
    compatible = "example,demo";
    reg = <0x10000000 0x1000>;
    resets = <&resetctrl 5>;
    reset-names = "core";
};
```

Driver:

```c
priv->rst = devm_reset_control_get_optional_exclusive(dev, "core");
if (IS_ERR(priv->rst))
    return dev_err_probe(dev, PTR_ERR(priv->rst),
                         "failed to get reset\n");
```

Use:

```c
ret = reset_control_assert(priv->rst);
if (ret)
    return ret;

usleep_range(1000, 2000);

ret = reset_control_deassert(priv->rst);
if (ret)
    return ret;
```

Some hardware requires reset asserted while rails ramp. Other hardware requires
clocks before reset can be released. Follow the documented sequence.

### Pinctrl States

Pinctrl selects pin muxing and electrical configuration.

Device Tree:

```dts
uart3: serial@10030000 {
    pinctrl-names = "default", "sleep";
    pinctrl-0 = <&uart3_default_pins>;
    pinctrl-1 = <&uart3_sleep_pins>;
};
```

PM callbacks:

```c
pinctrl_pm_select_default_state(dev);
pinctrl_pm_select_sleep_state(dev);
```

Pin state can affect:

- whether an IRQ reaches the GPIO controller
- leakage during suspend
- external reset lines
- bus pull-ups
- whether a powered-off peripheral drives a pin

### Power Domains

Power domains group devices into shared power islands.

Device Tree:

```dts
serial@10030000 {
    compatible = "example,uart";
    reg = <0x10030000 0x1000>;
    power-domains = <&power 3>;
};
```

Leaf drivers usually rely on runtime PM and the device core to coordinate the
domain. Do not write provider registers directly from the consumer driver.

### GPIO Enables

Some boards use GPIOs as enables, resets, or mode pins:

```dts
enable-gpios = <&gpio1 3 GPIO_ACTIVE_HIGH>;
reset-gpios = <&gpio1 4 GPIO_ACTIVE_LOW>;
```

Driver:

```c
priv->enable_gpio = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
if (IS_ERR(priv->enable_gpio))
    return dev_err_probe(dev, PTR_ERR(priv->enable_gpio),
                         "failed to get enable GPIO\n");
```

Use:

```c
gpiod_set_value_cansleep(priv->enable_gpio, 1);
usleep_range(1000, 2000);
```

Use the sleepable `*_cansleep()` GPIO accessors unless you know the GPIO
controller can be accessed atomically.

### PHYs And External Blocks

Some devices need PHYs or companion blocks:

```text
USB controller
  -> USB PHY
  -> reference clock
  -> regulator
  -> reset
```

The subsystem often defines the correct APIs:

```c
ret = phy_init(priv->phy);
ret = phy_power_on(priv->phy);
```

Do not replace subsystem APIs with ad hoc power toggles.

### Interconnect And Bandwidth

Some SoCs need interconnect bandwidth votes before high-throughput access:

```text
display controller
  -> memory bus bandwidth request
  -> underrun avoided
```

The specific APIs and bindings depend on platform support. Treat bandwidth as a
power/performance dependency, not just a performance tuning knob.

### Power Sequencing Providers

Some platforms have complex shared sequences that should not be open-coded in
every consumer. The kernel power sequencing API lets a provider expose a named
sequence target to consumers.

Conceptual model:

```text
consumer requests target
  -> provider enables shared regulators, GPIOs, delays, and units
  -> multiple consumers share reference-counted sequence
```

Use this only when the platform or subsystem provides a sequencing provider. It
is not a replacement for ordinary regulator, clock, reset, and pinctrl APIs in a
simple driver.

## Build A Single Power-On Helper

Keep the sequence in one helper so probe, runtime resume, and error recovery use
the same logic.

Example:

```c
static int demo_hw_power_on(struct demo_priv *priv)
{
    int ret;

    ret = regulator_bulk_enable(DEMO_NUM_SUPPLIES, priv->supplies);
    if (ret)
        return ret;

    usleep_range(1000, 2000);

    ret = clk_bulk_prepare_enable(DEMO_NUM_CLKS, priv->clks);
    if (ret)
        goto err_disable_regulators;

    ret = reset_control_deassert(priv->rst);
    if (ret)
        goto err_disable_clks;

    usleep_range(5000, 6000);

    ret = pinctrl_pm_select_default_state(priv->dev);
    if (ret)
        goto err_assert_reset;

    return 0;

err_assert_reset:
    reset_control_assert(priv->rst);
err_disable_clks:
    clk_bulk_disable_unprepare(DEMO_NUM_CLKS, priv->clks);
err_disable_regulators:
    regulator_bulk_disable(DEMO_NUM_SUPPLIES, priv->supplies);
    return ret;
}
```

Power-off reverses the successful parts:

```c
static void demo_hw_power_off(struct demo_priv *priv)
{
    pinctrl_pm_select_sleep_state(priv->dev);
    reset_control_assert(priv->rst);
    clk_bulk_disable_unprepare(DEMO_NUM_CLKS, priv->clks);
    regulator_bulk_disable(DEMO_NUM_SUPPLIES, priv->supplies);
}
```

This example is not a universal order. It shows the shape: every enable step has
a clear unwind step.

## Probe Integration

Probe should acquire resources, run the initial power-on sequence, identify or
initialize the device, then enable runtime PM.

```c
static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_priv *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    platform_set_drvdata(pdev, priv);
    priv->dev = dev;

    ret = demo_get_resources(priv);
    if (ret)
        return ret;

    ret = demo_hw_power_on(priv);
    if (ret)
        return ret;

    ret = demo_check_id(priv);
    if (ret)
        goto err_power_off;

    ret = demo_program_defaults(priv);
    if (ret)
        goto err_power_off;

    pm_runtime_set_active(dev);
    pm_runtime_enable(dev);
    pm_runtime_set_autosuspend_delay(dev, 1000);
    pm_runtime_use_autosuspend(dev);
    pm_runtime_mark_last_busy(dev);
    pm_runtime_put_autosuspend(dev);

    return 0;

err_power_off:
    demo_hw_power_off(priv);
    return ret;
}
```

Do not call `pm_runtime_enable()` before callbacks can safely run. Do not read
device registers before the power sequence has completed.

## Runtime PM Integration

Runtime callbacks can use the same helpers:

```c
static int demo_runtime_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    ret = demo_hw_power_on(priv);
    if (ret)
        return ret;

    return demo_restore_state(priv);
}

static int demo_runtime_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    demo_save_state(priv);
    demo_stop_io(priv);
    demo_hw_power_off(priv);

    return 0;
}
```

Operation paths must bracket hardware access:

```c
ret = pm_runtime_resume_and_get(dev);
if (ret)
    return ret;

ret = demo_do_transfer(priv);

pm_runtime_mark_last_busy(dev);
pm_runtime_put_autosuspend(dev);
return ret;
```

## System Sleep Integration

If runtime suspend is the correct system sleep low-power state:

```c
static int demo_suspend(struct device *dev)
{
    return pm_runtime_force_suspend(dev);
}

static int demo_resume(struct device *dev)
{
    return pm_runtime_force_resume(dev);
}
```

If system sleep requires wake configuration or a different pin state, wrap or
extend the sequence:

```c
static int demo_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);
    int ret;

    if (device_may_wakeup(dev)) {
        ret = demo_enable_wake_mode(priv);
        if (ret)
            return ret;
        enable_irq_wake(priv->irq);
    }

    ret = pm_runtime_force_suspend(dev);
    if (ret && device_may_wakeup(dev)) {
        disable_irq_wake(priv->irq);
        demo_disable_wake_mode(priv);
    }

    return ret;
}
```

Check whether runtime suspend disables a dependency needed for wake. If it does,
system suspend needs a different path.

## Shared Resources And Reference Counts

Regulators, clocks, power domains, and power sequencers may be shared. The
frameworks usually maintain reference counts.

Driver rules:

- request resources by role name, not provider internals
- enable only what the driver needs
- disable only what the driver enabled
- match every successful enable with a disable
- do not assume disable physically turns a shared resource off
- do not change voltage or rate of a shared resource without checking policy

Example:

```text
regulator_disable(vdd)
  -> this consumer's enable count drops
  -> rail may stay on because another device still uses it
```

This is correct. A consumer driver asks for its own requirement; the framework
coordinates shared state.

## Startup Delays

Delays should come from hardware requirements:

```text
rail ramp time
oscillator stable time
reset deassertion time
firmware boot time
PLL lock time
sensor conversion startup
```

Use sleepable delays in sleepable contexts:

```c
usleep_range(1000, 2000);
msleep(20);
```

Use polling helpers for status bits:

```c
ret = readl_poll_timeout(priv->base + DEMO_STATUS, val,
                         val & DEMO_PLL_LOCKED, 10, 10000);
if (ret)
    return ret;
```

Avoid unexplained delays:

```c
msleep(100); /* bad unless the hardware requirement is documented */
```

Prefer comments that cite the hardware reason:

```c
/* Datasheet: oscillator stable within 5 ms after core clock enable. */
usleep_range(5000, 6000);
```

## Error Unwinding

Every power-on step needs a matching cleanup path. Use reverse order.

Pattern:

```c
ret = step1();
if (ret)
    return ret;

ret = step2();
if (ret)
    goto err_step1;

ret = step3();
if (ret)
    goto err_step2;

return 0;

err_step2:
undo_step2();
err_step1:
undo_step1();
return ret;
```

For one-time probe setup, `devm_add_action_or_reset()` can keep cleanup local:

```c
static void demo_disable_regulators(void *data)
{
    struct demo_priv *priv = data;

    regulator_bulk_disable(DEMO_NUM_SUPPLIES, priv->supplies);
}

ret = regulator_bulk_enable(DEMO_NUM_SUPPLIES, priv->supplies);
if (ret)
    return ret;

ret = devm_add_action_or_reset(dev, demo_disable_regulators, priv);
if (ret)
    return ret;
```

Do not use devm cleanup as a substitute for runtime PM power transitions. Devm
actions run at device teardown, not whenever the device idles.

## Probe Deferral

Dependency providers may not be ready when the consumer probes.

Examples:

- regulator provider not probed
- clock controller missing
- reset controller disabled in config
- power-domain provider not registered
- GPIO controller not ready

Return `-EPROBE_DEFER` from provider APIs through `dev_err_probe()`:

```c
priv->vdd = devm_regulator_get(dev, "vdd");
if (IS_ERR(priv->vdd))
    return dev_err_probe(dev, PTR_ERR(priv->vdd),
                         "failed to get vdd\n");
```

This avoids noisy logs for normal deferral and keeps the real error visible.

## Debugging Dependency Failures

Symptoms and checks:

| Symptom | Likely Dependency | First Check |
| --- | --- | --- |
| bus fault on first read | power domain or clock off | domain summary, clock summary |
| ID register returns zero | reset asserted or rail missing | reset line, regulator summary |
| works after warm boot only | bootloader left resources on | cold boot, disable unused resources |
| fails after runtime idle | resume sequence incomplete | runtime PM logs and register restore |
| fails after suspend | sleep pin state or wake path wrong | pinctrl, wake source, domain state |
| probe defers forever | provider missing | config, firmware node, driver enabled |
| high idle power | resource not disabled or usage leak | runtime usage, clock/regulator summaries |

Useful debugfs paths when enabled:

```sh
sudo cat /sys/kernel/debug/clk/clk_summary
sudo cat /sys/kernel/debug/regulator/regulator_summary
sudo cat /sys/kernel/debug/pm_genpd/pm_genpd_summary
sudo cat /sys/kernel/debug/gpio
sudo cat /sys/kernel/debug/pinctrl/*/pinmux-pins
```

Runtime firmware description:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'clocks|clock-names|supply|resets|reset-names|power-domains|pinctrl' /tmp/running.dts
```

Kernel logs:

```sh
dmesg | grep -Ei 'defer|regulator|clk|clock|reset|genpd|power domain|pinctrl'
```

## Common Sequencing Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| Register access before clock enable | bus fault or timeout | enable required bus/functional clocks first |
| Reset deasserted before rail stable | intermittent probe | wait for rail and follow reset timing |
| Missing pinctrl default | GPIO/IRQ/bus does not work | define/select correct pin state |
| Sleep state breaks wake pin | missed wake | preserve wake pin input/bias |
| Runtime suspend disables shared wake resource | no system wake | use separate system sleep path |
| No failure unwind | leaked clock/regulator after probe fail | reverse successful steps |
| Optional resource used as required | NULL pointer or broken hardware | make binding and driver agree |
| Shared regulator voltage changed casually | other device instability | respect constraints and board policy |
| Delay used instead of status polling | slow and unreliable startup | poll documented ready bit |

## Practice Exercises

1. Choose a driver and write its power-up sequence as plain text before reading
   the code. Compare your sequence with the implementation.
2. Identify every `devm_*_get()` resource and decide whether it is a power,
   clock, reset, pin, bus, or wake dependency.
3. Find one failure path and verify that every successful enable is unwound.
4. Force runtime suspend/resume repeatedly and check whether the device still
   works after 100 cycles.
5. Inspect `clk_summary`, `regulator_summary`, and `pm_genpd_summary` before
   and after using the device.

## Review Checklist

- Is the sequence derived from hardware documentation?
- Are resource names aligned with the binding?
- Are optional resources truly optional?
- Does power-up have complete reverse-order failure handling?
- Does runtime resume restore every dependency needed for register access?
- Does runtime suspend stop I/O before removing resources?
- Does system suspend preserve wake dependencies?
- Are shared resources controlled only through framework APIs?
- Are delays justified and bounded?

## Related Topics

- [Clocks](../driver-interfaces/clocks.md)
- [Regulators](../driver-interfaces/regulators.md)
- [Resets](../driver-interfaces/resets.md)
- [Pinctrl](../driver-interfaces/pinctrl.md)
- [Runtime PM](runtime-pm.md)
- [Power Domains](power-domains.md)

## Official References

- [Regulator API](https://docs.kernel.org/driver-api/regulator.html)
- [Common Clock Framework](https://docs.kernel.org/driver-api/clk.html)
- [Reset Controller API](https://docs.kernel.org/driver-api/reset.html)
- [PINCTRL Subsystem](https://docs.kernel.org/driver-api/pin-control.html)
- [Power Sequencing API](https://docs.kernel.org/driver-api/pwrseq.html)
