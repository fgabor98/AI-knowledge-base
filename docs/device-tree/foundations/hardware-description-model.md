---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# The Hardware Description Model

## What Problem Does This Solve?

An operating system must know which hardware exists and how to reach it. Some buses provide enumeration: software can ask PCI or USB what devices are present and read standardized identifiers and resources. Much embedded hardware is different. A UART integrated into an SoC cannot usually announce its register address, interrupt line, input clock, reset, or board pin routing.

Historically, kernels carried board-specific C code containing these facts. Device Tree moves them into data so one kernel can support many boards without compiling their wiring into driver code.

The Devicetree Specification defines a tree of nodes and properties for describing hardware that a client program cannot necessarily discover. Linux uses that data primarily for platform identification, runtime configuration, and device population.

## Discoverable And Non-Discoverable Hardware

| Hardware | What software can often discover | What still needs external description |
|---|---|---|
| PCI endpoint | vendor/device ID, BARs, capabilities | host bridge, address windows, board power/reset details |
| USB device | class, vendor/product ID, interfaces | USB controller, PHY, connector and power topology |
| I2C sensor | sometimes a readable chip ID after access | bus number, slave address, interrupt, supplies, board mounting facts |
| SoC UART | little or nothing before correct access | register range, interrupt, clock, reset, pin routing |
| fixed regulator | nothing enumerable | voltage, enable GPIO, consumers, constraints |

“Discoverable” is not all-or-nothing. A bus may enumerate its children while the bus controller itself still needs Device Tree.

## Describe Hardware, Not A Driver Invocation

Good Device Tree data answers hardware questions:

- What component is present?
- Where is it in its parent's address space?
- Which signals and resources connect to it?
- Which hardware variant is it compatible with?
- Is it available on this board?

It should not normally answer Linux implementation questions:

- Which module should be loaded?
- In which order should probe functions run?
- Which internal kernel structure should be created?
- Which user-space pathname should an application use?
- Which workaround should a particular driver execute without a hardware distinction?

Compare a real node from the AM642 SK board (`arch/arm64/boot/dts/ti/k3-am642-sk.dts` in mainline Linux) with an invented anti-pattern:

```dts
/* Hardware facts: a TMP100 temperature sensor wired to I2C1 at address 0x48. */
&main_i2c1 {
        /* SoC power supply temperature */
        tmp100@48 {
                compatible = "ti,tmp100";
                reg = <0x48>;
        };
};
```

```dts
/* Invented for contrast: policy and implementation leakage. */
tmp100@48 {
        compatible = "ti,tmp100";
        reg = <0x48>;
        linux,probe-first;
        driver-name = "lm75";
        userspace-path = "/dev/soc-supply-temperature";
};
```

The second form tries to control a particular software implementation. The real node only states which part is fitted and where; Linux happens to drive it with the `lm75` hwmon driver, but nothing in the tree says so.

## Bindings Give Properties Meaning

Device Tree syntax only tells you that a node contains named byte sequences. A binding supplies semantics:

- allowed `compatible` values
- required and optional properties
- the type and number of values in each property
- child-node rules
- relationships to clocks, regulators, GPIOs, interrupts, and other providers

For the SK sensor above, correctness depends on two bindings:

1. The parent bus binding (`ti,omap4-i2c` via `i2c-controller.yaml`) says that `reg = <0x48>` is a 7-bit I2C target address, not a register offset.
2. The device binding (`Documentation/devicetree/bindings/hwmon/lm75.yaml`, which covers `ti,tmp100`) says that only `compatible` and `reg` are required, that a `vs-supply` regulator is optional, and that `interrupts` is allowed for `ti,tmp100` but forbidden for several sibling parts.

Never infer a property's complete meaning from its name. `reg`, `interrupts`, `clocks`, and GPIO specifiers are all decoded using context established elsewhere in the tree and in bindings.

## Bindings Are A Stable ABI

A deployed DTB may outlive the kernel version that first consumed it. Boot firmware, operating systems, hypervisors, and diagnostic tools may all interpret the same binding. Property names, compatible strings, value layouts, and defaults therefore form an ABI.

A compatible evolution usually follows these rules:

- keep the meaning of existing properties
- let new software accept older trees
- make new properties optional when an old representation remains sufficient
- preserve old behavior when a newly introduced property is absent
- introduce a new, specific `compatible` when hardware semantics change incompatibly
- allow a driver to support both old and new compatible strings during migration

The practical consequence is important: a property merged upstream is not merely an internal configuration option that can be renamed later for neatness.

## Device Tree, Driver, And Kernel Configuration

All three must agree:

```text
node exists and is available
        +
binding-compatible properties
        +
driver supports compatible string
        +
driver is enabled in kernel configuration
        +
providers and parent buses are available
        =
device has a chance to probe successfully
```

Device Tree cannot compensate for a missing driver. A driver cannot compensate for an absent device description when hardware is not discoverable. A match does not guarantee probe success because resource acquisition can still fail or defer.

## Device Tree And ACPI

Device Tree and ACPI can both convey platform information, but they come from different ecosystems and have different models.

| Question | Device Tree | ACPI |
|---|---|---|
| dominant use | embedded and SoC platforms | standardized PC/server and many Arm server platforms |
| basic representation | declarative tree of nodes and properties | tables plus namespace and AML-defined behavior |
| hardware description contract | Devicetree bindings | ACPI specifications and device-specific standards |
| common deployment | DTB selected or assembled by firmware/bootloader | firmware-provided ACPI tables |

The platform architecture and firmware ecosystem usually decide which model applies. A driver can support both through the kernel firmware-node abstraction, but a product should not invent parallel descriptions without a clear platform requirement.

## A Small End-To-End Example

The AM642 SK console UART is described in three layers. Trimmed excerpts from mainline Linux:

```dts
/* k3-am64.dtsi: the SoC and its main interconnect */
/ {
        compatible = "ti,am642";
        interrupt-parent = <&gic500>;
        #address-cells = <2>;
        #size-cells = <2>;

        cbass_main: bus@f4000 {
                compatible = "simple-bus";
                #address-cells = <2>;
                #size-cells = <2>;
                ranges = /* ... */
                         <0x00 0x01000000 0x00 0x01000000 0x00 0x02330400>; /* First peripheral window */
        };
};

/* k3-am64-main.dtsi: the UART block exists in silicon but is not yet wired */
&cbass_main {
        main_uart0: serial@2800000 {
                compatible = "ti,am64-uart", "ti,am654-uart";
                reg = <0x00 0x02800000 0x00 0x100>;
                interrupts = <GIC_SPI 178 IRQ_TYPE_LEVEL_HIGH>;
                clock-frequency = <48000000>;
                power-domains = <&k3_pds 146 TI_SCI_PD_EXCLUSIVE>;
                clocks = <&k3_clks 146 0>;
                clock-names = "fclk";
                status = "disabled";
        };
};

/* k3-am642-sk.dts: this board routes UART0 to pins and uses it as console */
&main_uart0 {
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&main_uart0_pins_default>;
};
```

None of this says “load a serial driver.” It states that a UART compatible with the AM654 programming model sits at physical address `0x2800000`, raises GIC SPI 178, takes its functional clock and power domain from the TI-SCI system firmware (device ID 146), and is pinned out on this board.

## Review Questions

1. Why does an I2C child still need a node even if its driver can read a chip ID?
2. Which is the ABI: the DTS filename, the property spelling, the driver function name, or the binding semantics?
3. Why is `probe-priority = <1>` usually suspicious?
4. What four independent conditions must hold before a described device can probe?
5. When would changing a `compatible` string be safer than changing an existing property's meaning?

## References

- [Devicetree Specification, The Devicetree](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
- [Devicetree ABI](https://docs.kernel.org/devicetree/bindings/ABI.html)
- [Devicetree bindings](https://docs.kernel.org/devicetree/bindings/index.html)

## Next Step

Continue with [Source And Binary Artifacts](source-and-binary-artifacts.md).
