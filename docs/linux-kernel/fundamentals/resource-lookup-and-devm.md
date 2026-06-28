---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Resource Lookup And Managed Allocation

## What Problem Does This Solve?

Drivers need to acquire memory, MMIO regions, IRQs, clocks, GPIOs, regulators, resets, DMA channels, firmware, and other resources without leaking them across probe failures or device removal.

The failure path matters as much as the success path:

```text
allocate private data
map registers
request irq
enable clock
register userspace interface
failure here
-> unwind everything already acquired
```

Device-managed helpers (`devm_*`) tie many resources to the device lifecycle, making common failure paths safer and shorter.

## Core Concepts

- `devm_*`
- managed resource
- device lifetime
- probe failure cleanup
- remove cleanup
- `devm_kzalloc()`
- `devm_platform_ioremap_resource()`
- `devm_request_threaded_irq()`
- `devm_gpiod_get()`
- `devm_clk_get()`
- `devm_regulator_get()`
- `devm_reset_control_get_optional_exclusive()`
- `devm_add_action_or_reset()`
- provider dependencies
- `-EPROBE_DEFER`
- cleanup ordering

## Mental Model

Managed resources are tied to `struct device`.

```text
probe starts
-> devm allocation/resource acquisition succeeds
-> if later probe fails, device core releases managed resources
-> if device is removed, device core releases managed resources
```

Managed cleanup reduces boilerplate. It does not remove the need to stop hardware and asynchronous activity in the correct order.

## Why Manual Cleanup Is Error-Prone

Manual pattern:

```c
priv = kzalloc(sizeof(*priv), GFP_KERNEL);
if (!priv)
    return -ENOMEM;

base = ioremap(...);
if (!base) {
    ret = -ENOMEM;
    goto err_free_priv;
}

ret = request_threaded_irq(...);
if (ret)
    goto err_iounmap;

return 0;

err_iounmap:
    iounmap(base);
err_free_priv:
    kfree(priv);
    return ret;
```

This is valid, but grows quickly.

Managed version:

```c
priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
    return -ENOMEM;

base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(base))
    return PTR_ERR(base);

ret = devm_request_threaded_irq(&pdev->dev, irq, NULL, demo_irq_thread,
                                IRQF_ONESHOT, dev_name(&pdev->dev), priv);
if (ret)
    return ret;
```

The error path is clearer and less likely to leak basic resources.

## Managed Memory Allocation

Allocate per-device state:

```c
struct demo_priv {
    struct device *dev;
    void __iomem *base;
    int irq;
};

priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
if (!priv)
    return -ENOMEM;

priv->dev = &pdev->dev;
platform_set_drvdata(pdev, priv);
```

Use `devm_kcalloc()` for arrays:

```c
priv->channels = devm_kcalloc(&pdev->dev, count,
                              sizeof(*priv->channels), GFP_KERNEL);
if (!priv->channels)
    return -ENOMEM;
```

Use normal `kmalloc()` only when the allocation lifetime is not the same as the device or when another subsystem explicitly takes ownership and defines cleanup.

## MMIO Resource Lookup

Device Tree:

```dts
demo@10000000 {
    compatible = "example,demo-mmio";
    reg = <0x0 0x10000000 0x0 0x1000>;
};
```

Driver:

```c
priv->base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(priv->base))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->base),
                         "failed to map registers\n");
```

Named resource:

```c
priv->ctrl = devm_platform_ioremap_resource_byname(pdev, "control");
if (IS_ERR(priv->ctrl))
    return PTR_ERR(priv->ctrl);
```

This replaces the common manual sequence:

```c
platform_get_resource()
devm_ioremap_resource()
```

## IRQ Lookup And Request

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return dev_err_probe(&pdev->dev, irq, "failed to get irq\n");

ret = devm_request_threaded_irq(&pdev->dev, irq,
                                NULL, demo_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(&pdev->dev), priv);
if (ret)
    return dev_err_probe(&pdev->dev, ret, "failed to request irq\n");
```

Named:

```c
irq = platform_get_irq_byname(pdev, "data-ready");
```

Managed IRQ freeing happens at device cleanup. You still must ensure hardware cannot continue generating interrupts after your state is invalid. Often this means disabling the device interrupt source in remove or before unregistering dependent interfaces.

## GPIO Lookup

Device Tree:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
```

Driver:

```c
priv->reset = devm_gpiod_get(&pdev->dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(priv->reset))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->reset),
                         "failed to get reset gpio\n");
```

Optional GPIO:

```c
priv->enable = devm_gpiod_get_optional(&pdev->dev, "enable", GPIOD_OUT_LOW);
if (IS_ERR(priv->enable))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->enable),
                         "failed to get enable gpio\n");
```

The consumer name `"reset"` maps to `reset-gpios`.

## Clock Lookup

Device Tree:

```dts
clocks = <&clkctrl 12>;
clock-names = "core";
```

Driver:

```c
priv->clk = devm_clk_get(&pdev->dev, "core");
if (IS_ERR(priv->clk))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->clk),
                         "failed to get core clock\n");

ret = clk_prepare_enable(priv->clk);
if (ret)
    return dev_err_probe(&pdev->dev, ret,
                         "failed to enable core clock\n");
```

If you manually enable a clock, you must disable it. Add a managed cleanup action:

```c
static void demo_clk_disable(void *data)
{
    clk_disable_unprepare(data);
}

ret = devm_add_action_or_reset(&pdev->dev, demo_clk_disable, priv->clk);
if (ret)
    return ret;
```

Some kernels provide convenience helpers for common clock-enable lifetimes. Use the helper available in your kernel tree and subsystem style.

## Regulator Lookup

Device Tree:

```dts
vdd-supply = <&vdd_3v3>;
```

Driver:

```c
priv->vdd = devm_regulator_get(&pdev->dev, "vdd");
if (IS_ERR(priv->vdd))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->vdd),
                         "failed to get vdd regulator\n");

ret = regulator_enable(priv->vdd);
if (ret)
    return dev_err_probe(&pdev->dev, ret,
                         "failed to enable vdd\n");
```

Add cleanup:

```c
static void demo_regulator_disable(void *data)
{
    regulator_disable(data);
}

ret = devm_add_action_or_reset(&pdev->dev,
                               demo_regulator_disable, priv->vdd);
if (ret)
    return ret;
```

## Reset Lookup

Device Tree:

```dts
resets = <&resetctrl 5>;
reset-names = "core";
```

Driver:

```c
priv->rst = devm_reset_control_get_optional_exclusive(&pdev->dev, "core");
if (IS_ERR(priv->rst))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->rst),
                         "failed to get reset\n");

ret = reset_control_deassert(priv->rst);
if (ret)
    return dev_err_probe(&pdev->dev, ret,
                         "failed to deassert reset\n");
```

Add cleanup if the hardware should return to reset:

```c
static void demo_reset_assert(void *data)
{
    reset_control_assert(data);
}

ret = devm_add_action_or_reset(&pdev->dev, demo_reset_assert, priv->rst);
if (ret)
    return ret;
```

## Provider Dependencies And `-EPROBE_DEFER`

Resource providers may not be ready yet.

Examples:

- regulator provider not probed
- clock controller not probed
- GPIO controller not probed
- reset controller missing
- pinctrl provider disabled
- power domain provider unavailable

Helpers often return:

```text
-EPROBE_DEFER
```

Use:

```c
return dev_err_probe(dev, ret, "failed to get vdd\n");
```

This preserves the error and avoids noisy logs for normal deferral.

Do not:

```c
if (ret == -EPROBE_DEFER)
    return -ENODEV; /* bad */
```

That prevents the device model from retrying.

## Managed Actions For Custom Cleanup

Use `devm_add_action_or_reset()` when you need custom cleanup tied to the device.

Example:

```c
static void demo_hw_disable(void *data)
{
    struct demo_priv *priv = data;

    writel(0, priv->base + DEMO_CTRL);
}

ret = demo_hw_enable(priv);
if (ret)
    return ret;

ret = devm_add_action_or_reset(&pdev->dev, demo_hw_disable, priv);
if (ret)
    return ret;
```

If adding the action fails, `devm_add_action_or_reset()` runs the action immediately, preventing a leak of the enabled hardware state.

Use this for:

- enabled clocks
- enabled regulators
- deasserted resets
- hardware modes
- temporary registrations without a direct devm helper

## Cleanup Ordering

Managed resources are released in reverse order of registration. This matters.

If you do:

```c
enable clock
add cleanup action to disable clock
request irq
register userspace device
```

then release ordering may not match your intended runtime shutdown.

For complex drivers, explicitly stop the device in `remove()`:

```c
static void demo_remove(struct platform_device *pdev)
{
    struct demo_priv *priv = platform_get_drvdata(pdev);

    demo_stop_io(priv);
    demo_disable_interrupts(priv);
}
```

Then let devm free passive resources after active paths are stopped.

## What `devm_*` Does Not Solve

Managed resources do not automatically:

- stop hardware from generating interrupts
- cancel delayed work
- flush workqueues
- stop timers
- close userspace file descriptors
- serialize callbacks
- unregister every subsystem object in the right semantic order
- make remove safe while an operation is active
- define a stable userspace ABI

Use explicit cleanup for active behavior:

```c
cancel_work_sync(&priv->work);
del_timer_sync(&priv->timer);
```

Use locking and reference counting when userspace or asynchronous callbacks can outlive parts of teardown.

## Full Probe Example

```c
struct demo_priv {
    struct device *dev;
    void __iomem *base;
    struct gpio_desc *reset;
    struct clk *clk;
    int irq;
};

static void demo_clk_disable(void *data)
{
    clk_disable_unprepare(data);
}

static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_priv *priv = data;

    dev_dbg(priv->dev, "interrupt\n");
    return IRQ_HANDLED;
}

static int demo_probe(struct platform_device *pdev)
{
    struct demo_priv *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &pdev->dev;
    platform_set_drvdata(pdev, priv);

    priv->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->base))
        return dev_err_probe(&pdev->dev, PTR_ERR(priv->base),
                             "failed to map registers\n");

    priv->reset = devm_gpiod_get_optional(&pdev->dev, "reset",
                                          GPIOD_OUT_HIGH);
    if (IS_ERR(priv->reset))
        return dev_err_probe(&pdev->dev, PTR_ERR(priv->reset),
                             "failed to get reset gpio\n");

    priv->clk = devm_clk_get(&pdev->dev, "core");
    if (IS_ERR(priv->clk))
        return dev_err_probe(&pdev->dev, PTR_ERR(priv->clk),
                             "failed to get core clock\n");

    ret = clk_prepare_enable(priv->clk);
    if (ret)
        return dev_err_probe(&pdev->dev, ret,
                             "failed to enable core clock\n");

    ret = devm_add_action_or_reset(&pdev->dev,
                                   demo_clk_disable, priv->clk);
    if (ret)
        return ret;

    priv->irq = platform_get_irq(pdev, 0);
    if (priv->irq < 0)
        return dev_err_probe(&pdev->dev, priv->irq,
                             "failed to get irq\n");

    ret = devm_request_threaded_irq(&pdev->dev, priv->irq,
                                    NULL, demo_irq_thread,
                                    IRQF_ONESHOT,
                                    dev_name(&pdev->dev), priv);
    if (ret)
        return dev_err_probe(&pdev->dev, ret,
                             "failed to request irq\n");

    dev_info(&pdev->dev, "resources acquired\n");
    return 0;
}
```

This is still not a complete production driver, but it demonstrates the acquisition shape.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Probe leaks resource on failure | manual cleanup path incomplete | failure labels, devm conversion |
| Probe defers forever | provider disabled/missing | provider DT nodes and configs |
| Crash after remove | active IRQ/work/timer used freed state | synchronous cleanup |
| IRQ fires before state ready | request IRQ too early | initialization order |
| Clock remains enabled after failed probe | enabled without cleanup action | `devm_add_action_or_reset()` |
| Resource lookup fails | DT property wrong or resource named differently | binding, runtime DT |
| Works once but fails after rebind | hardware not returned to safe state | remove path |

## Common Mistakes

- Treating `devm_*` as a replacement for understanding lifetime.
- Mixing managed and unmanaged cleanup without documenting ownership.
- Enabling hardware before all required state is initialized.
- Requesting an IRQ before the device can handle it.
- Dropping `-EPROBE_DEFER`.
- Not checking `IS_ERR()`/`PTR_ERR()` for resource helpers.
- Using optional resources but failing when they are absent.
- Forgetting custom cleanup for enabled clocks/regulators/resets.

## Practice Exercises

### Exercise 1: Convert Manual Allocation

Replace:

```c
priv = kzalloc(sizeof(*priv), GFP_KERNEL);
```

with:

```c
priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
```

Remove the matching `kfree()` and verify probe failure cleanup remains correct.

### Exercise 2: Add A Managed Cleanup Action

Enable a clock or mock hardware state, then register a cleanup action with `devm_add_action_or_reset()`.

Force a later probe failure and confirm cleanup runs.

### Exercise 3: Preserve Probe Deferral

Add a regulator dependency in DT, disable the provider, and observe the returned error. Use `dev_err_probe()` and compare logs to plain `dev_err()`.

## Debugging Checklist

- Is each resource tied to the correct lifetime?
- Are managed helpers used for per-device passive resources?
- Are active hardware paths stopped explicitly?
- Are cleanup actions registered immediately after enabling resources?
- Are resources requested by name when multiple resources exist?
- Does the code preserve `-EPROBE_DEFER`?
- Are optional resources handled as optional?
- Is error handling reverse-ordered for unmanaged resources?
- Can probe fail at each step without leaking state?
- Can unbind/rebind run repeatedly?

## Related Topics

- [Driver Binding, Probe, And Remove](driver-binding-probe-remove.md)
- [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
- [Kernel Memory Allocation](../memory-and-io/kernel-memory-allocation.md)
- [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)
- [IRQ Handling](../driver-interfaces/irq-handling.md)
- [Clocks](../driver-interfaces/clocks.md)
- [Regulators](../driver-interfaces/regulators.md)
- [Resets](../driver-interfaces/resets.md)

## Official References

- [Driver Basics](https://docs.kernel.org/driver-api/basics.html)
- [Platform Devices and Drivers](https://docs.kernel.org/driver-api/driver-model/platform.html)
- [GPIO Descriptor Driver Interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
