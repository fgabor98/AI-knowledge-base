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

Compare these conceptual properties:

```dts
/* Hardware facts described by a binding. */
temperature-sensor@48 {
        compatible = "example,tmp123";
        reg = <0x48>;
        interrupt-parent = <&gpio0>;
        interrupts = <7 IRQ_TYPE_LEVEL_LOW>;
        vdd-supply = <&reg_3v3>;
};
```

```dts
/* Suspicious policy or implementation leakage. */
temperature-sensor@48 {
        linux,probe-first;
        driver-name = "tmp123_driver";
        userspace-path = "/dev/board-temperature";
};
```

The second form tries to control a particular software implementation. A reusable binding describes the component and its wiring; compatible software decides how to use it.

## Bindings Give Properties Meaning

Device Tree syntax only tells you that a node contains named byte sequences. A binding supplies semantics:

- allowed `compatible` values
- required and optional properties
- the type and number of values in each property
- child-node rules
- relationships to clocks, regulators, GPIOs, interrupts, and other providers

For example, this is syntactically plausible:

```dts
sensor@48 {
        compatible = "example,tmp123";
        reg = <0x48>;
};
```

Whether it is correct depends on at least two bindings:

1. The parent bus binding determines how to interpret `reg`.
2. The `example,tmp123` binding determines which other properties are required.

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

Assume a board contains a UART at SoC offset `0x1000`, connected to interrupt 5 and an oscillator:

```dts
/dts-v1/;

/ {
        compatible = "example,trainer-board";
        #address-cells = <1>;
        #size-cells = <1>;

        osc: clock-24000000 {
                compatible = "fixed-clock";
                #clock-cells = <0>;
                clock-frequency = <24000000>;
        };

        soc {
                compatible = "simple-bus";
                #address-cells = <1>;
                #size-cells = <1>;
                ranges;

                serial@1000 {
                        compatible = "example,trainer-uart";
                        reg = <0x1000 0x100>;
                        interrupts = <5>;
                        clocks = <&osc>;
                        status = "okay";
                };
        };
};
```

This does not say “load a serial driver.” It states that a compatible UART exists at a particular location with particular connections. Software that understands all referenced bindings can interpret those facts.

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
