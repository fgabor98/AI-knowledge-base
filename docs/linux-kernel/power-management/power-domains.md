---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Power Domains

## What Problem Does This Solve?

Many devices cannot be powered independently. They sit inside a power island
that is shared with other devices, controlled by firmware, or controlled by a
SoC power controller.

Example:

```text
display controller
GPU
video decoder
  -> all inside multimedia power domain
```

If the domain is off, register access to any device inside it may fail even if
the driver's own clock and regulator handles look correct.

Power domains answer:

- Which group of devices shares power control?
- Who turns that group on and off?
- How are parent and child domains ordered?
- How does runtime PM know when the domain can power down?
- Can a wake source work if its domain is off?

## Core Concepts

### Power Domain

A power domain is a hardware or firmware-controlled power island. Devices inside
the domain depend on it being on before they can operate.

Example topology:

```text
always-on domain
  -> peripheral domain
     -> i2c controller
     -> spi controller
  -> multimedia domain
     -> display controller
     -> video decoder
```

Power domains may be:

- controlled by SoC registers
- controlled by firmware
- always on
- hierarchical
- shared by unrelated-looking devices
- associated with clocks, resets, isolation, retention, or SRAM state

### Generic PM Domains

The kernel generic PM domain framework is commonly called genpd. It connects
domain providers, device consumers, runtime PM, and system sleep ordering.

Conceptually:

```text
consumer device runtime resumes
  -> genpd powers domain on if needed
  -> device runtime resume callback runs

consumer device runtime suspends
  -> device runtime suspend callback runs
  -> genpd may power domain off when all consumers are idle
```

A normal leaf driver usually does not call genpd internals. It describes its
domain in firmware and uses runtime PM correctly.

### Provider And Consumer

The provider owns the domain control.

Example provider:

```dts
power: power-controller@12340000 {
    compatible = "example,soc-power-controller";
    reg = <0x12340000 0x1000>;
    #power-domain-cells = <1>;
};
```

The consumer references the domain:

```dts
uart3: serial@10030000 {
    compatible = "example,uart";
    reg = <0x10030000 0x1000>;
    clocks = <&clk UART3_CLK>;
    power-domains = <&power 3>;
};
```

The exact provider binding is SoC-specific. The consumer should describe the
relationship, not manually control provider registers.

### Domain Hierarchy

Domains can have parents:

```text
top domain
  -> display domain
     -> display-subsystem domain
        -> display controller
        -> HDMI PHY
```

The parent must be powered before the child. When the last child is idle, the
framework may power down child and parent domains according to constraints.

This matters because a leaf device might appear independent in the driver tree
but still depend on a parent domain that also serves other blocks.

### Always-On Domains

Some domains are described as always on because turning them off would break
boot, firmware, debug, wake, or shared infrastructure.

Always-on does not mean drivers can ignore runtime PM. The device's local clock,
reset, and register state may still need correct handling even when the domain
never powers down.

### Firmware-Controlled Domains

On some platforms, Linux asks firmware to change power state instead of writing
SoC registers directly.

Examples:

- ARM SCMI power domains
- PSCI-related CPU or cluster states
- ACPI power resources
- vendor secure firmware services

Driver implication:

```text
leaf driver uses runtime PM
  -> PM domain provider talks to firmware
  -> firmware enforces platform policy
```

The leaf driver should not bypass firmware ownership.

## Runtime PM Interaction

Power domains are most useful when device drivers use runtime PM.

Normal flow:

```text
driver operation
  -> pm_runtime_resume_and_get(dev)
     -> domain powers on
     -> clocks/resources become available
     -> driver's runtime_resume runs
     -> hardware access
  -> pm_runtime_put_autosuspend(dev)
     -> driver's runtime_suspend runs later
     -> domain may power off
```

If a driver never drops its runtime PM reference, the domain may never power
down. If a driver accesses registers without resuming the device, it may touch a
powered-off domain.

## Device Links And Supplier Ordering

Power domains are one kind of supplier dependency. Clocks, regulators, interconnects,
IOMMUs, and buses may also be suppliers.

The device core can use device links to order suppliers and consumers:

```text
power domain provider
  -> consumer device
```

This helps with:

- probe ordering
- suspend/resume ordering
- runtime PM ordering
- avoiding supplier removal while consumers are active

As a driver author, prefer standard firmware properties and framework APIs so
the core can infer or create the right relationships.

## Multiple Power Domains

Some devices depend on more than one domain. Bindings may name them:

```dts
video@20000000 {
    compatible = "example,video";
    reg = <0x20000000 0x10000>;
    power-domains = <&power 1>, <&power 2>;
    power-domain-names = "core", "bus";
};
```

Many simple drivers do not manually attach to domains. The bus or PM core may
handle the default domain automatically. Multiple domains are more complex and
often require subsystem or driver-specific support.

Before writing manual attach code, check:

- Does the binding define multiple power domains?
- Does the subsystem already handle them?
- Are there existing drivers for the same SoC family?
- Are runtime PM callbacks aware of all domains?
- What happens if one domain can wake but another cannot?

## Sequencing With Clocks, Resets, And Regulators

Power domain on/off is only part of the sequence.

Common power-up shape:

```text
power domain on
regulators enabled
clocks prepared/enabled
reset deasserted
pinctrl default state selected
register access
```

Common power-down shape:

```text
stop I/O
save state
mask interrupts
assert reset if required
disable clocks
disable regulators
power domain off
```

The exact order is hardware-specific. Some domains require clocks during power
transitions; some resets must be asserted before power is removed; some SRAM
retention must be configured before domain off.

The driver should encode the sequence in helper functions with one clear owner:

```c
static int demo_hw_power_on(struct demo_priv *priv)
{
    int ret;

    ret = regulator_bulk_enable(DEMO_NUM_SUPPLIES, priv->supplies);
    if (ret)
        return ret;

    ret = clk_bulk_prepare_enable(DEMO_NUM_CLKS, priv->clks);
    if (ret)
        goto err_disable_regulators;

    ret = reset_control_deassert(priv->rst);
    if (ret)
        goto err_disable_clks;

    return 0;

err_disable_clks:
    clk_bulk_disable_unprepare(DEMO_NUM_CLKS, priv->clks);
err_disable_regulators:
    regulator_bulk_disable(DEMO_NUM_SUPPLIES, priv->supplies);
    return ret;
}
```

The PM domain itself is usually entered before the driver callback and left
after the callback through runtime PM ordering, but provider behavior can vary.
Design from the platform's documented PM model.

## Wake Sources Inside Domains

Wakeup is where power-domain mistakes become visible.

A device may be wake-capable only if the domain or wake island remains powered:

```text
touch controller wake IRQ
  -> touch controller internal wake logic
  -> GPIO controller
  -> interrupt controller
  -> always-on wake logic
```

If any required piece is in a domain that powers off, wake may fail.

Questions:

- Is the device itself powered during suspend?
- Is the GPIO or interrupt controller powered?
- Is the pinctrl sleep state configured for wake?
- Does the domain provider support wakeup constraints?
- Does firmware need to be told that this domain contains a wake source?
- Does the wake path use an always-on companion block instead of the main device
  domain?

Do not solve this only in the leaf driver. The correct fix may be in the power
domain provider, interrupt controller, pinctrl state, or firmware description.

## Debugging Power Domains

If debugfs is available, inspect genpd state:

```sh
sudo cat /sys/kernel/debug/pm_genpd/pm_genpd_summary
```

Useful questions:

- Is the expected device attached to the expected domain?
- Is the domain on when the device is active?
- Does the domain remain on when all consumers are idle?
- Are there unexpected active consumers?
- Are child domains blocking parent power-off?

Inspect runtime PM state for the consumer:

```sh
cat /sys/devices/.../power/runtime_status
cat /sys/devices/.../power/runtime_usage
cat /sys/devices/.../power/control
```

Check firmware description:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'power-domains|power-domain-names|#power-domain-cells' /tmp/running.dts
```

Trace runtime PM and power events:

```sh
sudo trace-cmd record -e power sleep 5
sudo trace-cmd report
```

Use platform logs too. Firmware-controlled domains may report failures through
firmware-specific tracing or error codes.

## `-EPROBE_DEFER` And Provider Ordering

If a consumer probes before its power-domain provider is ready, probe may defer:

```text
demo 10030000.serial: supplier power-controller not ready
demo 10030000.serial: probe defer
```

Driver code should use `dev_err_probe()` for resource acquisition:

```c
priv->clk = devm_clk_get(dev, NULL);
if (IS_ERR(priv->clk))
    return dev_err_probe(dev, PTR_ERR(priv->clk),
                         "failed to get clock\n");
```

For domains described in firmware and handled by the core, the driver may not
explicitly request the domain. Still, a missing or late provider can affect when
the consumer becomes usable.

## Common Power Domain Bugs

| Bug | Symptom | Fix |
| --- | --- | --- |
| Missing `power-domains` property | register access fails when firmware leaves domain off | fix firmware/Device Tree binding data |
| Driver bypasses runtime PM | access while domain is off | bracket hardware access with runtime PM |
| Usage count leak | domain never powers down | match gets and puts |
| Wake source in powered-off domain | missed wake | keep wake path powered or configure provider wake support |
| Provider probe missing | consumer probe defers forever | enable provider driver and firmware node |
| Wrong domain index | unrelated block powers on/off | fix provider cell value |
| Parent domain not modeled | child appears on but access still fails | describe hierarchy correctly |
| Firmware ownership ignored | secure monitor errors or no effect | use firmware-backed provider |
| Always-on assumed for local resources | driver works until clock/reset state changes | still manage clocks, resets, and state |

## Practice Exercises

1. Pick a Device Tree node with `power-domains`. Find the provider node and
   identify the domain cell meaning from the binding.
2. Inspect `pm_genpd_summary` and verify whether the device appears under the
   expected domain.
3. Toggle the device's runtime PM policy and watch whether the domain state
   changes.
4. Identify a wake-capable device and map every powered block in its wake path.
5. Review a driver that accesses registers in a sysfs or ioctl path and verify
   that it resumes the device first.

## Review Checklist

- Is the device's power domain described by firmware?
- Does the driver rely on runtime PM instead of direct provider control?
- Are provider and consumer probe-order failures handled cleanly?
- Are parent/child domains represented?
- Are wake-capable paths compatible with domain power-off?
- Are clocks, regulators, resets, and pinctrl states sequenced with domain state?
- Can debugfs or tracing confirm that the domain changes state as expected?

## Related Topics

- [Runtime PM](runtime-pm.md)
- [Device Tree](../../device-tree/index.md)
- [Regulator And Clock Power Dependencies](regulator-clock-power-dependencies.md)

## Official References

- [Device Power Management Basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)
