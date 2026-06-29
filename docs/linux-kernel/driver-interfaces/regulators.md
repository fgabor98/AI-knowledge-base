---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Regulators

## What Problem Does This Solve?

Regulator APIs let drivers request and control power rails needed by devices.

Drivers should ask for supplies by role:

```text
vdd
vddio
avdd
dvdd
vref
```

Board data defines which regulator provider supplies each rail, allowed voltages, startup delays, and whether the rail is always on.

## Core Concepts

- regulator provider
- regulator consumer
- fixed regulator
- PMIC regulator
- supply property
- voltage constraints
- current/load
- enable count
- optional supplies
- bulk regulators
- always-on rails
- boot-on rails
- startup delay
- runtime PM interaction

## Mental Model

A driver consumes named supplies. The board decides where the supply comes from and what voltage range is safe.

```dts
vdd-supply = <&vdd_3v3>;
vref-supply = <&vref_2v5>;
```

Driver:

```c
vdd = devm_regulator_get(dev, "vdd");
vref = devm_regulator_get(dev, "vref");
```

The driver should not hard-code PMIC register addresses or board rail topology.

## Required Supply

```c
priv->vdd = devm_regulator_get(dev, "vdd");
if (IS_ERR(priv->vdd))
    return dev_err_probe(dev, PTR_ERR(priv->vdd),
                         "failed to get vdd supply\n");
```

Enable:

```c
ret = regulator_enable(priv->vdd);
if (ret)
    return dev_err_probe(dev, ret, "failed to enable vdd\n");
```

Disable:

```c
regulator_disable(priv->vdd);
```

Getting a regulator does not necessarily enable it.

## Cleanup Action

```c
static void demo_regulator_disable(void *data)
{
    regulator_disable(data);
}

ret = regulator_enable(priv->vdd);
if (ret)
    return ret;

ret = devm_add_action_or_reset(dev, demo_regulator_disable, priv->vdd);
if (ret)
    return ret;
```

This handles later probe failures safely.

## Optional Supply

Use optional only when the binding says the supply is optional:

```c
priv->vref = devm_regulator_get_optional(dev, "vref");
if (IS_ERR(priv->vref)) {
    ret = PTR_ERR(priv->vref);
    if (ret == -ENODEV)
        priv->vref = NULL;
    else
        return dev_err_probe(dev, ret, "failed to get vref\n");
}
```

If the device cannot function without the rail, do not use optional lookup.

## Bulk Regulators

```c
static const char * const supply_names[] = {
    "vdd",
    "vddio",
};

for (i = 0; i < ARRAY_SIZE(supply_names); i++)
    priv->supplies[i].supply = supply_names[i];

ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(supply_names),
                              priv->supplies);
if (ret)
    return dev_err_probe(dev, ret, "failed to get supplies\n");

ret = regulator_bulk_enable(ARRAY_SIZE(supply_names), priv->supplies);
if (ret)
    return ret;
```

Disable:

```c
regulator_bulk_disable(ARRAY_SIZE(supply_names), priv->supplies);
```

Bulk APIs reduce ordering and cleanup mistakes.

## Voltage Constraints

Drivers may query voltage:

```c
uv = regulator_get_voltage(priv->vdd);
if (uv > 0)
    dev_dbg(dev, "vdd=%duV\n", uv);
```

Set voltage only when the hardware manual, binding, and board policy allow it:

```c
ret = regulator_set_voltage(priv->vdd, 1800000, 1800000);
if (ret)
    return ret;
```

Most device drivers should rely on board constraints rather than changing shared rails casually.

## Loads And Modes

Some regulators support load hints:

```c
ret = regulator_set_load(priv->vdd, 10000);
```

This can help PMICs choose operating modes, but only if board and regulator constraints support it. Use when the subsystem or hardware design calls for it.

## Device Tree Example

Fixed regulator:

```dts
vdd_3v3: regulator-3v3 {
    compatible = "regulator-fixed";
    regulator-name = "vdd_3v3";
    regulator-min-microvolt = <3300000>;
    regulator-max-microvolt = <3300000>;
    regulator-always-on;
};
```

Consumer:

```dts
sensor@48 {
    compatible = "example,tmp102";
    reg = <0x48>;
    vdd-supply = <&vdd_3v3>;
};
```

The property name before `-supply` becomes the consumer supply name.

## Startup Delays

Regulators or devices may need delays after enable:

```text
enable regulator
wait for rail to settle
release reset
wait for device startup
```

Some regulator providers encode enable ramp delay. Some drivers still need device-specific delays after power is valid.

Use datasheet timing and `usleep_range()` where appropriate.

## Runtime PM

Drivers often enable regulators during runtime resume and disable during runtime suspend:

```c
static int demo_runtime_resume(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    return regulator_enable(priv->vdd);
}

static int demo_runtime_suspend(struct device *dev)
{
    struct demo_priv *priv = dev_get_drvdata(dev);

    regulator_disable(priv->vdd);
    return 0;
}
```

Coordinate with clocks, resets, pinctrl sleep state, and register cache.

## Debugging Regulators

Debugfs, if enabled:

```sh
cat /sys/kernel/debug/regulator/regulator_summary
```

Runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'supply|regulator-' /tmp/running.dts
```

Logs:

```sh
dmesg | grep -i regulator
```

Measure rails with appropriate tools when hardware behavior is suspect.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `-EPROBE_DEFER` | regulator provider not ready | provider node/driver |
| device NACKs or reads wrong ID | supply disabled or wrong voltage | regulator summary, measurement |
| enable fails | constraints invalid or rail unavailable | DT constraints |
| other hardware affected | shared rail changed/disabled | regulator consumers |
| resume fails | supply not re-enabled or registers lost | PM flow |
| optional supply hides bug | required rail declared optional | binding review |

## Common Mistakes

- Assuming `devm_regulator_get()` enables the rail.
- Disabling a shared rail without understanding consumers.
- Using optional regulators for required supplies.
- Changing voltage without board-level validation.
- Forgetting startup delays.
- Not coordinating regulator state with reset and clock sequencing.
- Failing to disable an enabled regulator on later probe failure.

## Practice Exercises

### Exercise 1: Add A Required Supply

Add `vdd-supply` to a test node and request it with `devm_regulator_get()`.

### Exercise 2: Add Enable Cleanup

Enable the regulator and use `devm_add_action_or_reset()` to disable it on failure/remove.

### Exercise 3: Inspect Regulator Consumers

Use regulator debugfs to find a rail and list its consumers.

## Debugging Checklist

- Does the runtime Device Tree contain the expected `*-supply` property?
- Did the regulator provider probe?
- Is the supply required or optional by binding?
- Is the rail enabled before device access?
- Are voltage constraints valid?
- Are startup delays satisfied?
- Is cleanup symmetric?
- Does runtime PM preserve the power sequence?

## Related Topics

- [Power Management](../power-management/index.md)
- [Clocks](clocks.md)
- [Resets](resets.md)
- [Regulator And Clock Power Dependencies](../power-management/regulator-clock-power-dependencies.md)

## Official References

- [Voltage and current regulator API](https://docs.kernel.org/driver-api/regulator.html)
- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)
