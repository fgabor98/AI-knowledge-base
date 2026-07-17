---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Hardware-Based Layering And Source Style

## Layer By Physical Reuse

Source boundaries should describe real reuse, not merely reduce file length:

```text
trainer-soc.dtsi
    silicon-integrated blocks and topology
            |
trainer-som.dtsi
    module RAM, PMIC, oscillator, fitted devices
            |
trainer-carrier-common.dtsi
    hardware genuinely shared by carrier variants
            |
trainer-carrier-a.dts / trainer-carrier-b.dts
    exact board model and variant-specific wiring
```

The ownership test is simple: if the physical component changes when this assembly is swapped, its description belongs at or below that assembly's layer.

## SoC Layer

An SoC DTSI describes blocks present in the silicon, their fixed addresses, internal interrupts, and stable provider topology. Controllers often default to disabled because a board must supply pins, clocks, PHY connections, or external devices:

```dts
uart0: serial@1000 {
        compatible = "example,trainer-uart";
        reg = <0x1000 0x100>;
        interrupts = <5>;

        status = "disabled";
};
```

Do not place a carrier-board sensor in the SoC DTSI just because all current products fit it.

## Module Layer

A system-on-module DTSI can add hardware physically owned by the module:

```dts
/ {
        osc: clock-24000000 {
                compatible = "fixed-clock";
                #clock-cells = <0>;
                clock-frequency = <24000000>;
        };
};

&uart0 {
        clocks = <&osc>;
};
```

If the module routes a controller to edge-connector pins but does not determine its final peripheral, keep the description incomplete or disabled until the carrier supplies the missing facts.

## Board Layer

The final `.dts` identifies the exact machine and describes fitted board hardware:

```dts
/dts-v1/;

#include "trainer-som.dtsi"

/ {
        model = "Example Trainer Carrier A";
        compatible = "example,trainer-carrier-a", "example,trainer-soc";
};

&uart0 {
        pinctrl-0 = <&uart0_pins>;
        pinctrl-names = "default";
        status = "okay";
};
```

The root compatible begins with the exact board. A reusable DTSI generally should not claim the final product identity.

## Variant Strategies

Choose the smallest structure that makes hardware differences explicit:

| Difference | Good starting strategy |
|---|---|
| one board, assembly option not auto-detected | separate final DTS or carefully selected overlay |
| several boards share a real carrier circuit | shared carrier DTSI plus final DTS files |
| same module on unrelated carriers | module DTSI included by each carrier DTS |
| silicon revisions with stable programming model | common SoC DTSI plus revision-specific compatible/data as bindings require |

Avoid a “common” DTSI that contains the union of all product hardware followed by many deletions. It obscures what each board actually contains and makes defaults dangerous.

## Override Versus Fix The Owner

An override is appropriate when a lower physical layer supplies a board-specific fact such as availability or pin selection. It is a smell when every board corrects the same mistaken SoC value.

Ask:

- Is this value genuinely variable at this hardware boundary?
- Do all consumers make the same replacement?
- Is the base incomplete by design, or simply wrong?
- Will a new board author discover the required amendment?

Repeated corrective overrides should move to the layer that owns the truth.

## Linux Source Style

Linux narrows the permissive DTS grammar to improve consistency:

- lowercase letters, digits, and dashes for node and property names
- lowercase letters, digits, and underscores for labels
- lowercase hexadecimal unit addresses without unnecessary leading zeros
- generic node names defined by bindings
- nodes with unit addresses ordered by address, subject to architecture convention
- nodes without unit addresses ordered alphanumerically
- amendments ordered alphabetically or in original DTSI order according to local convention

Preferred property order is:

1. `compatible`
2. `reg`
3. `ranges`
4. standard/common properties
5. vendor-specific properties
6. `status`
7. child nodes

Within a logical group, use natural property-name ordering. Separate child nodes and a final `status` visually where local style expects it.

## Formatting Multi-Entry Properties

Keep logical entries visible:

```dts
reg = <0x1000 0x100>,
      <0x2000 0x80>;

clocks = <&clock_controller 3>,
         <&clock_controller 7>;
clock-names = "bus", "core";
```

Align companion lists so reviewers can compare positions. A formatter cannot prove that the third clock name belongs to the third specifier, but consistent layout makes mistakes more visible.

## Comments And Documentation Debt

Useful comments capture facts that are difficult to infer:

- schematic net names or board-revision constraints
- why a non-obvious fallback compatible is safe
- why hardware must remain disabled
- which firmware component owns a reserved resource
- why a binding-approved default is intentionally overridden

Do not use comments to compensate for misleading layering. If a comment says “actually on the carrier,” move the node to the carrier layer when possible.

## Review Across Files

Review the effective hardware description, not each patch hunk in isolation:

1. start at the final DTS
2. follow includes in their effective order
3. find original node definitions
4. apply amendments and deletions mentally or through decompilation
5. verify physical ownership of every changed fact
6. compare sibling boards for accidental divergence
7. run schema validation on every affected DTB target

A small DTS diff can change a node defined across four files. Conversely, a large mechanical reorder may change no binary semantics but still carries merge risk.

## Common Architecture Smells

- board identity in a reusable module DTSI
- carrier peripherals in an SoC file
- all possible devices enabled in a common base
- variants selected through undocumented CPP macros
- long chains of includes with cyclic conceptual ownership
- repeated per-board deletion of the same bad common node
- labels named after Linux driver functions rather than hardware blocks
- a shared DTSI created only because two files currently contain similar text

## Exercises

1. Place a PMIC mounted on a system-on-module in the correct layer.
2. Decide where a carrier-selected UART pinctrl state belongs.
3. Identify when three identical overrides indicate a broken base layer.
4. Reorder an example node according to Linux property style.
5. Explain why textual similarity alone does not justify a shared DTSI.

## References And Next Step

- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Devicetree bindings guidance](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)

Apply the design in the [Source Composition Lab](source-composition-lab.md).
