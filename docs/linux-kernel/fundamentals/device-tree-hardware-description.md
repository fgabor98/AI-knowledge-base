---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Device Tree Hardware Description

## What Problem Does This Solve?

Device Tree describes non-discoverable hardware so Linux can create device objects and pass board-specific wiring to reusable drivers.

Many embedded devices are not discoverable like PCI or USB devices. The kernel cannot ask an MMIO block, GPIO expander, regulator, clock tree, or board-specific sensor where it lives and how it is wired. Device Tree supplies that information as data.

The driver should not hard-code:

- physical register addresses
- IRQ numbers
- GPIO numbers
- regulator names
- clock names
- board-specific reset polarity
- bus addresses

Those facts belong in the hardware description and in binding-defined properties.

## Core Concepts

- Device Tree
- DTS source
- DTSI include
- DTB compiled blob
- node
- property
- `compatible`
- `reg`
- `interrupts`
- `interrupt-parent`
- GPIO specifier
- clock specifier
- regulator supply property
- reset specifier
- pinctrl state
- `status`
- address cells
- size cells
- binding document
- runtime tree under `/proc/device-tree`

## Mental Model

Device Tree is board data, not driver code.

```text
SoC .dtsi
  describes reusable SoC blocks

board .dts
  enables blocks and describes board wiring

driver
  supports compatible hardware and reads binding-defined properties
```

The flow:

```text
*.dts + *.dtsi
-> dtc compiles DTB
-> bootloader passes DTB to kernel
-> kernel creates devices
-> bus matching calls driver probe
```

Changing the source `.dts` is not enough. You must build the DTB, deploy the DTB the bootloader actually uses, boot it, and inspect the runtime tree.

## DTS, DTSI, And DTB

Common file roles:

| File Type | Purpose |
| --- | --- |
| `.dtsi` | Shared include file, usually SoC, family, carrier, or module-level description. |
| `.dts` | Board-level root source that includes `.dtsi` files and sets board-specific details. |
| `.dtb` | Compiled binary blob passed to the kernel. |
| `.dtbo` | Compiled overlay blob applied to a base tree. |

Common paths:

```text
arch/arm/boot/dts/
arch/arm64/boot/dts/
arch/riscv/boot/dts/
Documentation/devicetree/bindings/
```

Build:

```sh
make O=build-arm64 ARCH=arm64 dtbs
```

Inspect runtime:

```sh
tr -d '\0' < /proc/device-tree/model
find /proc/device-tree -maxdepth 3 -name compatible -print
```

## Node And Property Basics

Example:

```dts
temperature-sensor@48 {
    compatible = "example,tmp102", "ti,tmp102";
    reg = <0x48>;
    interrupt-parent = <&gpio1>;
    interrupts = <7 IRQ_TYPE_LEVEL_LOW>;
    vdd-supply = <&vdd_3v3>;
    status = "okay";
};
```

Meaning:

| Property | Meaning |
| --- | --- |
| node name | Human-readable hardware type plus unit address. |
| `compatible` | Ordered list of hardware identities. |
| `reg` | Address or address/size tuple on the parent bus. |
| `interrupt-parent` | Interrupt controller provider. |
| `interrupts` | Interrupt specifier interpreted by the parent. |
| `vdd-supply` | Regulator provider reference. |
| `status` | Whether the node is enabled. |

The exact meaning of `reg`, `interrupts`, GPIOs, clocks, and resets depends on the parent bus/provider and its cell counts.

## `compatible` Strings

`compatible` identifies hardware for driver matching.

Preferred form:

```dts
compatible = "vendor,specific-device", "vendor,fallback-family";
```

The first string is most specific. Later strings are fallbacks.

Example:

```dts
compatible = "acme,foo123-rev2", "acme,foo123";
```

A driver can match either string:

```c
static const struct of_device_id foo_of_match[] = {
    { .compatible = "acme,foo123" },
    { }
};
MODULE_DEVICE_TABLE(of, foo_of_match);
```

Do not invent generic strings like:

```dts
compatible = "my-device";
```

Use a vendor prefix and binding-defined names.

## `reg` And Address Cells

The `reg` property is interpreted using the parent node's:

```dts
#address-cells
#size-cells
```

For an MMIO bus:

```dts
soc {
    #address-cells = <2>;
    #size-cells = <2>;

    demo@10000000 {
        compatible = "example,demo-mmio";
        reg = <0x0 0x10000000 0x0 0x1000>;
    };
};
```

This means:

```text
address = 0x0000000010000000
size    = 0x0000000000001000
```

For an I2C bus:

```dts
i2c0 {
    #address-cells = <1>;
    #size-cells = <0>;

    eeprom@50 {
        compatible = "atmel,24c02";
        reg = <0x50>;
    };
};
```

There is no size cell because the parent bus defines addresses differently.

## Interrupts

Interrupt properties depend on the interrupt controller.

Example:

```dts
demo@10000000 {
    compatible = "example,demo-mmio";
    reg = <0x0 0x10000000 0x0 0x1000>;
    interrupts = <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>;
};
```

Driver side:

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return dev_err_probe(&pdev->dev, irq, "failed to get irq\n");
```

Named interrupts:

```dts
interrupt-names = "data-ready", "error";
interrupts = <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>,
             <GIC_SPI 43 IRQ_TYPE_LEVEL_HIGH>;
```

Driver side:

```c
irq = platform_get_irq_byname(pdev, "data-ready");
```

Use names when a device has multiple resources of the same type.

## GPIOs

GPIO consumer properties usually end in `-gpios`:

```dts
reset-gpios = <&gpio2 5 GPIO_ACTIVE_LOW>;
enable-gpios = <&gpio1 3 GPIO_ACTIVE_HIGH>;
```

Driver side:

```c
priv->reset_gpio = devm_gpiod_get(&pdev->dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(priv->reset_gpio))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->reset_gpio),
                         "failed to get reset gpio\n");
```

The driver requests `"reset"`, while Device Tree uses `reset-gpios`.

Do not use legacy global GPIO numbers in new drivers.

## Clocks, Resets, Regulators, And Pinctrl

Example:

```dts
demo@10000000 {
    compatible = "example,demo-mmio";
    reg = <0x0 0x10000000 0x0 0x1000>;
    clocks = <&clkctrl 12>;
    clock-names = "core";
    resets = <&resetctrl 5>;
    reset-names = "core";
    vdd-supply = <&vdd_3v3>;
    pinctrl-names = "default";
    pinctrl-0 = <&demo_pins>;
    status = "okay";
};
```

Driver side:

```c
priv->clk = devm_clk_get(&pdev->dev, "core");
if (IS_ERR(priv->clk))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->clk),
                         "failed to get core clock\n");

priv->vdd = devm_regulator_get(&pdev->dev, "vdd");
if (IS_ERR(priv->vdd))
    return dev_err_probe(&pdev->dev, PTR_ERR(priv->vdd),
                         "failed to get vdd regulator\n");
```

Provider dependencies may not be ready when your driver probes. In that case helpers often return `-EPROBE_DEFER`. Return it upward rather than converting it to another error.

## `status`

Common status values:

```dts
status = "okay";
status = "disabled";
```

SoC `.dtsi` files often define hardware blocks as disabled:

```dts
uart3: serial@2800000 {
    compatible = "vendor,soc-uart";
    reg = <0x0 0x02800000 0x0 0x1000>;
    status = "disabled";
};
```

Board `.dts` enables only the blocks that are wired and used:

```dts
&uart3 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart3_pins>;
    status = "okay";
};
```

If a node is disabled, the driver may never probe even if the source node exists.

## Binding Documentation

Bindings define the contract between Device Tree and driver.

Modern bindings live under:

```text
Documentation/devicetree/bindings/
```

They explain:

- valid `compatible` strings
- required properties
- optional properties
- child node layout
- allowed value ranges
- examples

Before inventing a property, check whether a binding already exists.

Bad:

```dts
magic-delay = <10>;
```

Better if the binding defines it:

```dts
reset-delay-us = <10000>;
```

Property names become ABI. Changing them later can break deployed Device Trees.

## Runtime Verification

The runtime tree is under `/proc/device-tree`.

Read model:

```sh
tr -d '\0' < /proc/device-tree/model
```

Find compatible strings:

```sh
find /proc/device-tree -name compatible -print
```

Read one property:

```sh
tr -d '\0' < /proc/device-tree/soc/demo@10000000/compatible
```

Some properties are binary cells. Use tools:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
```

or decompile a DTB:

```sh
dtc -I dtb -O dts board.dtb > board.dts
```

The decompiled output is useful for inspection, but the maintained source remains the `.dts` and `.dtsi` files.

## Example: Minimal Platform Device Node

```dts
demo@10000000 {
    compatible = "example,demo-mmio";
    reg = <0x0 0x10000000 0x0 0x1000>;
    interrupts = <GIC_SPI 42 IRQ_TYPE_LEVEL_HIGH>;
    status = "okay";
};
```

Driver resource lookup:

```c
base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(base))
    return PTR_ERR(base);

irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return irq;
```

## Example: I2C Device Node

```dts
&i2c2 {
    status = "okay";

    temperature-sensor@48 {
        compatible = "example,tmp102";
        reg = <0x48>;
        vdd-supply = <&vdd_3v3>;
        interrupt-parent = <&gpio1>;
        interrupts = <7 IRQ_TYPE_LEVEL_LOW>;
    };
};
```

This creates an I2C client device. It should be handled by an I2C client driver, not a platform driver.

The same Device Tree concepts apply, but the bus type changes the driver API.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Driver never probes | missing/disabled node, wrong compatible, driver not enabled | runtime DT, `.config`, `dmesg` |
| Probe gets no MMIO resource | wrong `reg` format or parent cell counts | binding, parent `#address-cells` |
| Probe gets no IRQ | missing interrupt parent or wrong specifier | `interrupts`, interrupt controller binding |
| GPIO lookup fails | wrong property name or provider not ready | `*-gpios`, provider driver |
| `-EPROBE_DEFER` repeats | provider missing or disabled | clocks/regulators/resets/pinctrl providers |
| DTS change has no effect | old DTB deployed or wrong boot path | checksum, bootloader env, `/proc/device-tree` |
| Binding check fails | property name/value violates schema | `dtbs_check`, binding docs |

## Common Mistakes

- Editing a `.dtsi` or `.dts` that is not used by the deployed DTB.
- Forgetting `status = "okay";`.
- Hard-coding board wiring in the driver.
- Inventing properties without binding documentation.
- Using the wrong bus driver type for the node.
- Confusing source DTS with runtime Device Tree.
- Ignoring parent `#address-cells` and `#size-cells`.
- Treating a compiled DTB as the source of truth.
- Forgetting that U-Boot and Linux may use different DTBs.

## Practice Exercises

### Exercise 1: Find The Runtime Node

On a board:

```sh
tr -d '\0' < /proc/device-tree/model
find /proc/device-tree -name compatible -print | head
```

Choose one device and find its source `.dts` or `.dtsi`.

### Exercise 2: Trace A `compatible` String

Pick a compatible string from runtime Device Tree:

```sh
tr -d '\0' < /proc/device-tree/path/to/node/compatible
```

Search the kernel source:

```sh
rg 'vendor,device' drivers Documentation/devicetree
```

Questions:

- Which driver matches it?
- Which binding documents it?

### Exercise 3: Add A Dummy Platform Node

Add a harmless node for a dummy driver:

```dts
demo {
    compatible = "example,demo-device";
    status = "okay";
};
```

Build/deploy the DTB and confirm the node appears in `/proc/device-tree`.

## Debugging Checklist

- Does the runtime tree contain the node?
- Is `status` enabled?
- Does the `compatible` string exactly match the driver table?
- Does the binding define every property you use?
- Are `reg` and `interrupts` encoded for the parent bus/controller?
- Are GPIO, clock, regulator, reset, and pinctrl providers enabled?
- Did you rebuild and deploy the correct DTB?
- Does the bootloader load the DTB you changed?

## Related Topics

- [Device Tree](../../device-tree/index.md)
- [Device Tree Matching From Drivers](device-tree-matching.md)
- [Device Tree Builds](../../build-systems/advanced/linux-kernel/device-tree-builds.md)
- [Device Tree Binding Validation](../../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)
- [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)

## Official References

- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
- [Devicetree Bindings](https://docs.kernel.org/devicetree/bindings/index.html)
- [Devicetree Specification](https://www.devicetree.org/specifications/)
