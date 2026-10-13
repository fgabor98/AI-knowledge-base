---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Hardware Description Contracts And Policy Boundaries

A binding should describe what hardware exists, how it is connected, and which fixed constraints govern it. It should not serialize the control flow, object model, or preferences of one driver. The most important design step is therefore deciding which interface owns each fact.

## Start With The Physical Contract

Before writing DTS or YAML, collect evidence from:

- the IP and SoC integration manuals
- board schematics, BOM variants, and strap tables
- clock, reset, power, pin, interrupt, and DMA topology
- discoverable identification and capability registers
- boot-firmware and secure-firmware interface specifications
- electrical limits and sequencing requirements

Describe the device without mentioning Linux. A useful inventory looks like:

```text
device: dual-channel capture engine
register windows: control, DMA
interrupt outputs: completion, error
clock inputs: bus, sample
reset inputs: core
DMA masters: one per channel
external connections: two fixed sensor links
discoverable facts: revision, FIFO depth
board-specific facts: sensor endpoint and ref clock rate
required sequence: supply -> ref clock -> reset release
```

Only after this inventory should you map facts to standard DT concepts.

## Classify Every Candidate Fact

| Class | Typical home | Example |
|---|---|---|
| fixed, non-discoverable integration | DT property or relationship | interrupt wiring, regulator supply |
| implementation identity | `compatible` plus driver match data | register layout revision |
| safely discoverable capability | hardware register | FIFO depth reported by hardware |
| boot-time discovery/handoff | firmware protocol or owned fixup | trained RAM size |
| build-time software choice | Kconfig/build configuration | debugging implementation |
| runtime mechanism | kernel or firmware subsystem | DMA allocation strategy |
| user/product policy | userspace or managed configuration | preferred operating profile |

The same numeric value can belong in different places depending on its meaning. A maximum clock rate imposed by board wiring is hardware description; a preferred clock selected to save power is policy.

## The Counterfactual Driver Test

Ask whether an independently written operating system could understand the property from its name and binding alone. If the description says “set this so driver function X takes branch Y,” the abstraction is wrong.

Take the Cadence CSI-2 receiver on AM62x. A driver-shaped proposal for it might be (hypothetical):

```dts
cdns_csi2rx0: csi-bridge@30101000 {
        compatible = "ti,j721e-csi2rx", "cdns,csi2rx";
        ti,use-polling;
        ti,allocate-large-buffer;
        ti,skip-reset-workaround;
};
```

These properties select implementation techniques. The real node in `k3-am62-main.dtsi` models the hardware instead:

```dts
cdns_csi2rx0: csi-bridge@30101000 {
        compatible = "ti,j721e-csi2rx", "cdns,csi2rx";
        reg = <0x00 0x30101000 0x00 0x1000>;
        interrupts = <GIC_SPI 143 IRQ_TYPE_LEVEL_HIGH>,
                     <GIC_SPI 141 IRQ_TYPE_LEVEL_HIGH>;
        interrupt-names = "error_irq", "irq";
        clocks = <&k3_clks 182 0>, <&k3_clks 182 3>, /* ... */;
        clock-names = "sys_clk", "p_clk", /* pixel_if0..3_clk */;
        phys = <&dphy0>;
        phy-names = "dphy";
};
```

Whether to poll or use the interrupts, and how large a buffer to allocate, are driver decisions made from these facts.

A silicon erratum belongs in driver data keyed by something the hardware identifies, not in a property named after the workaround. `sdhci_am654` disables HS400 on AM62P SR1.0 and SR1.1 through `soc_device_match()` on the SoC family and revision; no board DT says “skip HS400.”

## Nodes Are Hardware, Not Driver Instances

Do not create a node only to cause Linux to instantiate a platform device. A node normally represents a discoverable or non-discoverable hardware entity, a defined logical hardware function, or a standard firmware interface.

Warning signs include:

- a node name ending in `-driver`, `-helper`, or `-manager`
- `compatible = "vendor,my-linux-module"`
- a child node for each internal C structure
- a node whose only purpose is to pass arbitrary configuration to a driver
- duplicate representations of one hardware block so two drivers can probe

Multifunction hardware can have child nodes when the binding models real independently addressable or functionally distinct hardware. The boundary must follow the hardware and established subsystem model, not the desired module split.

## Mechanism Versus Policy

DT may describe that a device has two clock parents and the constraints on them. Choosing a clock dynamically for current workload is policy. DT may describe thermal sensors, trip capabilities, and cooling-device relationships when defined by the binding; fleet-specific performance policy should not be smuggled into vendor properties.

Use this test:

```text
Would two products with identical assembled hardware but different customer policy
need different values?
```

If yes, prefer a runtime policy interface unless the relevant standard binding explicitly defines the value as platform configuration.

## Firmware Interfaces Are Still Contracts

A node representing PSCI, SCMI, OP-TEE, or another standardized firmware interface describes an interface the platform provides. Do not mirror firmware implementation internals or secure resources that normal-world software cannot own.

Separate:

- the existence and transport of the interface
- identifiers defined by that interface
- permissions and ownership controlled by firmware
- requests or policies negotiated at runtime

A normal-world DT cannot grant itself access by describing a secure device.

## Review Exercise

Classify each proposal before looking at a driver:

1. `ti,dma-buffer-size = <1048576>` on the CSI-2 shim, chosen to reduce dropped frames (hypothetical).
2. `max-frequency = <25000000>` on an SD host because of a level shifter on this board.
3. `ti,silicon-revision = <2>` on the AM62P SD host, although the SoC revision is readable through socinfo (hypothetical).
4. `reset-gpios` on BeaglePlay's Ethernet PHY, wired to `main_gpio0` line 15.
5. `ti,use-new-api` added during a driver rewrite (hypothetical).

Expected reasoning:

1. likely runtime/resource policy, not a hardware fact
2. hardware integration constraint, using the subsystem's standard property if defined
3. discover from hardware; do not duplicate it
4. physical board wiring, represented by the binding's standard reset relationship
5. implementation selector; reject it

## Design Review Checklist

- Can the device be described accurately without naming Linux internals?
- Does each node correspond to a defensible hardware or standardized firmware entity?
- Is every value fixed for this physical instance?
- Have discoverable and compatible-implied facts been removed?
- Are runtime negotiation and user policy outside the binding?
- Is ownership consistent across normal world, secure world, and auxiliary processors?
- Would another operating system infer the same semantics?

## Authoritative References

- [Linux binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Devicetree Specification: purpose and terminology](https://devicetree-specification.readthedocs.io/en/stable/chapter2-devicetree-basics.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)

## Continue

Proceed to [Complete Bindings And Extensible Hardware Models](complete-bindings-and-extensible-hardware-models.md).
