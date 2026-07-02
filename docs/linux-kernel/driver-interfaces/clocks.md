---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Clocks

## What Problem Does This Solve?

Clock APIs let drivers enable, disable, and sometimes configure clock inputs required by hardware blocks.

Many SoC peripherals do nothing until their functional or bus clocks are enabled. Other devices need a reference clock at a specific rate. Drivers should request clocks by role name and let board/SoC data describe which clock provider supplies them.

## Core Concepts

- clock provider
- clock consumer
- `struct clk`
- `devm_clk_get()`
- `devm_clk_get_optional()`
- clock bulk APIs
- `clk_prepare_enable()`
- `clk_disable_unprepare()`
- `clk_get_rate()`
- `clk_set_rate()`
- assigned clocks
- clock parent
- runtime PM interaction
- debugfs clock summary

## Mental Model

A driver consumes named clocks. Board and SoC descriptions decide which provider supplies each clock and what constraints apply.

```text
Device Tree:
  clocks = <&clkctrl 12>;
  clock-names = "core";

Driver:
  clk = devm_clk_get(dev, "core");
  clk_prepare_enable(clk);
```

The driver should not know clock-controller register details.

## Device Tree Example

```dts
demo@10000000 {
    compatible = "example,demo-mmio";
    reg = <0x0 0x10000000 0x0 0x1000>;
    clocks = <&clkctrl 12>, <&clkctrl 13>;
    clock-names = "core", "bus";
};
```

Driver:

```c
priv->core_clk = devm_clk_get(dev, "core");
if (IS_ERR(priv->core_clk))
    return dev_err_probe(dev, PTR_ERR(priv->core_clk),
                         "failed to get core clock\n");

priv->bus_clk = devm_clk_get(dev, "bus");
if (IS_ERR(priv->bus_clk))
    return dev_err_probe(dev, PTR_ERR(priv->bus_clk),
                         "failed to get bus clock\n");
```

Use names when a device has more than one clock.

## Enabling And Disabling

Enable:

```c
ret = clk_prepare_enable(priv->core_clk);
if (ret)
    return dev_err_probe(dev, ret, "failed to enable core clock\n");
```

Disable:

```c
clk_disable_unprepare(priv->core_clk);
```

`prepare` may sleep. Do not call `clk_prepare_enable()` in hard IRQ context.

## Managed Cleanup Action

Clock references from `devm_clk_get()` are managed, but enabling a clock is an active state change. You must disable it.

Use a cleanup action:

```c
static void demo_clk_disable(void *data)
{
    clk_disable_unprepare(data);
}

ret = clk_prepare_enable(priv->core_clk);
if (ret)
    return ret;

ret = devm_add_action_or_reset(dev, demo_clk_disable, priv->core_clk);
if (ret)
    return ret;
```

This disables the clock if a later probe step fails.

## Optional Clocks

Some hardware variants do not have a clock:

```c
priv->ref_clk = devm_clk_get_optional(dev, "ref");
if (IS_ERR(priv->ref_clk))
    return dev_err_probe(dev, PTR_ERR(priv->ref_clk),
                         "failed to get ref clock\n");

if (priv->ref_clk) {
    ret = clk_prepare_enable(priv->ref_clk);
    if (ret)
        return ret;
}
```

Use optional clocks only when the binding says the clock is optional.

## Bulk Clocks

For several clocks:

```c
static const char * const demo_clk_names[] = {
    "core",
    "bus",
    "iface",
};

priv->clks = devm_kcalloc(dev, ARRAY_SIZE(demo_clk_names),
                          sizeof(*priv->clks), GFP_KERNEL);
if (!priv->clks)
    return -ENOMEM;

for (i = 0; i < ARRAY_SIZE(demo_clk_names); i++)
    priv->clks[i].id = demo_clk_names[i];

ret = devm_clk_bulk_get(dev, ARRAY_SIZE(demo_clk_names), priv->clks);
if (ret)
    return dev_err_probe(dev, ret, "failed to get clocks\n");

ret = clk_bulk_prepare_enable(ARRAY_SIZE(demo_clk_names), priv->clks);
if (ret)
    return ret;
```

Disable:

```c
clk_bulk_disable_unprepare(ARRAY_SIZE(demo_clk_names), priv->clks);
```

Bulk APIs reduce error-prone repetitive code.

## Clock Rates

Read effective rate:

```c
rate = clk_get_rate(priv->core_clk);
dev_dbg(dev, "core clock rate %lu Hz\n", rate);
```

Set a rate only if the binding, hardware manual, and board policy allow it:

```c
ret = clk_set_rate(priv->core_clk, 24000000);
if (ret)
    return dev_err_probe(dev, ret, "failed to set clock rate\n");
```

Changing a shared clock can affect other consumers. Prefer board-level `assigned-clocks` for static setup when possible.

## Assigned Clocks

Device Tree may configure clocks before the driver probes:

```dts
assigned-clocks = <&clkctrl 12>;
assigned-clock-rates = <24000000>;
```

Use this for board/static policy where the driver only needs to consume an already configured clock.

Drivers should still validate rates when the device protocol requires a range.

## Runtime PM

Many drivers enable clocks in runtime resume and disable them in runtime suspend:

```c
static int demo_runtime_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    return clk_prepare_enable(priv->core_clk);
}

static int demo_runtime_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    clk_disable_unprepare(priv->core_clk);
    return 0;
}
```

This avoids keeping hardware clocked while idle.

Coordinate clocks with regulators, resets, and pinctrl states.

## Debugging Clocks

Debugfs, if enabled:

```sh
cat /sys/kernel/debug/clk/clk_summary
```

Search for the clock:

```sh
grep -i demo /sys/kernel/debug/clk/clk_summary
```

Runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'clocks|clock-names|assigned-clocks' /tmp/running.dts
```

Driver logs:

```sh
dmesg | grep -i clk
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `-EPROBE_DEFER` | clock provider not ready | provider node/driver |
| hardware registers read zero/stale | functional clock disabled | clk_summary |
| clock enable fails | missing clock, invalid parent, power domain off | dmesg |
| wrong data rate | unexpected clock rate | `clk_get_rate`, assigned clocks |
| other devices break | changed shared clock rate | clock tree consumers |
| suspend/resume failure | clock not restored | runtime/system PM callbacks |

## Common Mistakes

- Getting a clock with `devm_clk_get()` and assuming it is enabled.
- Enabling a clock without disabling it on failure/remove.
- Calling clock prepare APIs in hard IRQ context.
- Changing shared clock rates casually.
- Treating optional clocks as optional when the binding requires them.
- Debugging a device register before checking the functional clock.

## Practice Exercises

### Exercise 1: Add A Required Clock

Add `clocks` and `clock-names` to a dummy node and request the clock in probe.

### Exercise 2: Log Clock Rate

Call `clk_get_rate()` after enable and log the result with `dev_dbg()`.

### Exercise 3: Add Runtime PM Clock Gating

Move clock enable/disable into runtime resume/suspend for an idle-capable device.

## Debugging Checklist

- Does runtime Device Tree contain the expected clock names?
- Did the clock provider probe?
- Does `devm_clk_get()` return `-EPROBE_DEFER`?
- Is the clock enabled before register access?
- Is it disabled in matching cleanup paths?
- Is the rate correct?
- Is the clock shared with other consumers?
- Does runtime PM keep clock state coherent?

## Related Topics

- [Runtime PM](../power-management/runtime-pm.md)
- [Regulators](regulators.md)
- [Resets](resets.md)
- [Pinctrl](pinctrl.md)
- [Regulator And Clock Power Dependencies](../power-management/regulator-clock-power-dependencies.md)

## Official References

- [The Common Clk Framework](https://docs.kernel.org/driver-api/clk.html)
- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)
