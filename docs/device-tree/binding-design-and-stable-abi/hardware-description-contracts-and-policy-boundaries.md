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

Bad interface:

```dts
capture@40000000 {
        compatible = "acme,capture-v2";
        acme,use-polling;
        acme,allocate-large-buffer;
        acme,skip-reset-workaround;
};
```

These properties select implementation techniques. Model the underlying truth instead:

```dts
capture@40000000 {
        compatible = "acme,capture-v2";
        reg = <0x40000000 0x1000>;
        interrupts = <42>, <43>;
        interrupt-names = "completion", "error";
        resets = <&resetc 7>;
        reset-names = "core";
};
```

If a particular revision has a reset erratum, encode it in compatible-specific match data rather than a property named after the workaround.

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

1. `vendor,dma-buffer-size = <1048576>` chosen to reduce dropped frames.
2. `max-frequency = <25000000>` due to a level shifter on this board.
3. `vendor,revision = <3>` even though a reliable revision register exists.
4. `reset-gpios` for a peripheral reset wired only on one carrier.
5. `vendor,use-new-api` added during a driver rewrite.

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
