---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Resets

## What Problem Does This Solve?

Reset APIs let drivers control reset lines for hardware blocks without hard-coding reset controller details.

Reset lines are often required to:

- bring hardware into a known state
- release a peripheral after power is valid
- recover from errors
- coordinate bootloader and kernel ownership
- isolate a block during suspend or shutdown

## Core Concepts

- reset controller
- reset consumer
- `struct reset_control`
- assert
- deassert
- reset pulse
- shared reset
- exclusive reset
- optional reset
- bulk reset APIs
- reset sequencing
- startup delays

## Mental Model

Reset lines are part of the hardware lifecycle. A driver requests a named reset role and performs the sequence required by the hardware manual.

```text
enable regulator
prepare clock
assert reset
wait
deassert reset
wait startup delay
access registers
```

The exact order is device-specific.

## Device Tree Example

```dts
demo@10000000 {
    compatible = "example,demo-mmio";
    reg = <0x0 0x10000000 0x0 0x1000>;
    resets = <&resetctrl 5>;
    reset-names = "core";
};
```

Driver:

```c
priv->rst = devm_reset_control_get_exclusive(dev, "core");
if (IS_ERR(priv->rst))
    return dev_err_probe(dev, PTR_ERR(priv->rst),
                         "failed to get core reset\n");
```

## Assert And Deassert

```c
ret = reset_control_assert(priv->rst);
if (ret)
    return ret;

usleep_range(1000, 2000);

ret = reset_control_deassert(priv->rst);
if (ret)
    return ret;
```

Use delays from the hardware manual. Do not invent arbitrary sleeps unless you are isolating a lab issue.

## Reset Pulse

Some devices only need a pulse:

```c
ret = reset_control_reset(priv->rst);
if (ret)
    return dev_err_probe(dev, ret, "failed to reset device\n");
```

This asserts and deasserts according to reset-controller behavior.

## Optional Resets

```c
priv->rst = devm_reset_control_get_optional_exclusive(dev, "core");
if (IS_ERR(priv->rst))
    return dev_err_probe(dev, PTR_ERR(priv->rst),
                         "failed to get reset\n");

if (priv->rst) {
    ret = reset_control_deassert(priv->rst);
    if (ret)
        return ret;
}
```

Use optional only when the binding says the reset is optional.

## Exclusive Versus Shared

Exclusive reset:

```c
devm_reset_control_get_exclusive(dev, "core");
```

Use when the reset affects only this device or when this driver owns the line.

Shared reset:

```c
devm_reset_control_get_shared(dev, "bus");
```

Use only when multiple consumers legitimately share the reset and the hardware/reset-controller semantics support sharing.

Shared resets require careful coordination. A shared assert may affect other devices.

## Bulk Resets

For multiple resets:

```c
struct reset_control_bulk_data resets[] = {
    { .id = "core" },
    { .id = "bus" },
};

ret = devm_reset_control_bulk_get_exclusive(dev,
        ARRAY_SIZE(resets), resets);
if (ret)
    return dev_err_probe(dev, ret, "failed to get resets\n");

ret = reset_control_bulk_deassert(ARRAY_SIZE(resets), resets);
if (ret)
    return ret;
```

Use bulk APIs to keep related reset handling consistent.

## Cleanup Actions

If a driver deasserts reset in probe and wants to assert it on remove/failure:

```c
static void demo_reset_assert(void *data)
{
    reset_control_assert(data);
}

ret = reset_control_deassert(priv->rst);
if (ret)
    return ret;

ret = devm_add_action_or_reset(dev, demo_reset_assert, priv->rst);
if (ret)
    return ret;
```

This ensures later probe failures do not leave the device running unexpectedly.

## Sequencing With Power And Clocks

Common sequences:

```text
regulator enable
clock prepare/enable
reset deassert
wait
register access
```

or:

```text
assert reset
enable regulator
wait for rail
enable clock
deassert reset
wait startup
```

The correct sequence depends on the hardware. Follow the datasheet and board design.

## Bootloader State

Boot firmware may leave hardware:

- already out of reset
- clocked
- partially configured
- in a boot mode
- holding a reset line shared with another block

Linux drivers should not assume reset state unless the platform contract says so. A controlled reset sequence in probe is often safer, but not always possible for shared or boot-critical hardware.

## Runtime PM And Error Recovery

Resets may be used:

- during runtime resume
- during runtime suspend
- after transfer timeout
- after bus-off or fault state
- during system shutdown

Be careful: resetting active hardware can invalidate registers, DMA descriptors, FIFOs, and subsystem state. Stop I/O first.

## Debugging Resets

Runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'resets|reset-names' /tmp/running.dts
```

Logs:

```sh
dmesg | grep -i reset
```

If debugfs support exists for your reset provider, inspect it. Availability is platform-specific.

Use a scope for external reset pins when possible.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `-EPROBE_DEFER` | reset provider not ready | provider driver/node |
| register reads fail | reset still asserted | reset sequence |
| device works after bootloader but not cold boot | missing reset/power delay | hardware sequence |
| other block breaks | reset line is shared | reset topology |
| intermittent startup | delay too short after deassert | datasheet timing |
| resume fails | reset state not restored | PM callbacks |

## Common Mistakes

- Treating reset lines as optional without binding support.
- Deasserting reset before power or clocks are valid.
- Forgetting startup delays.
- Asserting a shared reset and breaking another device.
- Leaving reset deasserted after failed probe when policy expects safe off.
- Using GPIO reset manually when a reset controller binding/API exists.

## Practice Exercises

### Exercise 1: Add A Named Reset

Add `resets` and `reset-names` to a test node, request it, and deassert it in probe.

### Exercise 2: Add Cleanup

Use `devm_add_action_or_reset()` to assert reset if a later probe step fails.

### Exercise 3: Review Boot State

Compare warm boot and cold boot behavior. Determine whether firmware leaves the device out of reset.

## Debugging Checklist

- Does runtime Device Tree contain the reset property?
- Did the reset provider probe?
- Is the reset exclusive or shared?
- Is the sequence correct relative to power and clocks?
- Are required delays present?
- Is cleanup policy explicit?
- Does suspend/resume restore expected reset state?

## Related Topics

- [Clocks](clocks.md)
- [Regulators](regulators.md)
- [Runtime PM](../power-management/runtime-pm.md)
- [Power Domains](../power-management/power-domains.md)

## Official References

- [Reset controller API](https://docs.kernel.org/driver-api/reset.html)
