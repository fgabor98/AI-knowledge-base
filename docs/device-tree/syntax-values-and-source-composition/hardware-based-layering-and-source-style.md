---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Hardware-Based Layering And Source Style

## Layer By Physical Reuse

Source boundaries should describe real reuse, not merely reduce file length. The PHYTEC AM64x boards in mainline Linux follow the physical assemblies exactly:

```text
k3-am64.dtsi + k3-am64-main.dtsi + k3-am64-mcu.dtsi
    silicon-integrated blocks and interconnect topology (TI)
            |
k3-am642.dtsi
    exact SoC variant: two A53 cores and their L2 cache (TI)
            |
k3-am64-phycore-som.dtsi
    module RAM, PMIC, eMMC, OSPI flash, Ethernet PHY, RTC (PHYTEC)
            |
k3-am642-phyboard-electra-rdk.dts
    exact carrier: keys, LEDs, CAN, SPI, USB, SD card, ICSSG Ethernet (PHYTEC)
```

The ownership test is simple: if the physical component changes when this assembly is swapped, its description belongs at or below that assembly's layer.

## SoC Layer

An SoC DTSI describes blocks present in the silicon, their fixed addresses, internal interrupts, and stable provider topology. Controllers default to disabled because a board must supply pins, clocks, PHY connections, or external devices. `k3-am64-main.dtsi`:

```dts
main_i2c0: i2c@20000000 {
        compatible = "ti,am64-i2c", "ti,omap4-i2c";
        reg = <0x00 0x20000000 0x00 0x100>;
        interrupts = <GIC_SPI 161 IRQ_TYPE_LEVEL_HIGH>;
        #address-cells = <1>;
        #size-cells = <0>;
        power-domains = <&k3_pds 102 TI_SCI_PD_EXCLUSIVE>;
        clocks = <&k3_clks 102 2>;
        clock-names = "fck";
        status = "disabled";
};
```

Do not place a carrier-board sensor in the SoC DTSI just because all current products fit it.

## Module Layer

A system-on-module DTSI adds hardware physically owned by the module. `k3-am64-phycore-som.dtsi` describes the module's EEPROM, RTC, and PMIC on I2C0 (trimmed):

```dts
&main_i2c0 {
        /* ... */
        eeprom@50 {
                compatible = "atmel,24c32";
                /* ... */
        };

        i2c_som_rtc: rtc@52 {
                compatible = "microcrystal,rv3028";
                /* ... */
        };

        pmic@61 {
                compatible = "ti,lp8733";
                /* ... */
        };
};
```

The module also sets a module-level identity, `compatible = "phytec,am64-phycore-som", "ti,am642";`, which every carrier overrides. If the module routes a controller to edge-connector pins but does not determine its final peripheral, keep the description incomplete or disabled until the carrier supplies the missing facts.

## Board Layer

The final `.dts` identifies the exact machine and describes fitted carrier hardware (`k3-am642-phyboard-electra-rdk.dts`):

```dts
/dts-v1/;

#include "k3-am642.dtsi"
#include "k3-am64-phycore-som.dtsi"

/ {
        compatible = "phytec,am642-phyboard-electra-rdk",
                     "phytec,am64-phycore-som", "ti,am642";
        model = "PHYTEC phyBOARD-Electra-AM64x RDK";

        chosen {
                stdout-path = &main_uart0;
        };
};
```

The root compatible begins with the exact board, then the module, then the SoC. The carrier, not a reusable DTSI, owns the final product identity.

## Variant Strategies

Choose the smallest structure that makes hardware differences explicit:

| Difference | Real example in `arch/arm64/boot/dts/ti/` |
|---|---|
| same module on unrelated carriers | `k3-am64-phycore-som.dtsi`, `k3-am642-tqma64xxl.dtsi`, and `k3-am642-sr-som.dtsi` each included by their carrier DTS |
| one carrier, SERDES lane used for PCIe or USB3 | `k3-am642-hummingboard-t-pcie.dts` and `-usb3.dts` each `#include "k3-am642-hummingboard-t.dts"` and change only the lane |
| optional add-on hardware | `k3-am642-evm-icssg1-dualemac.dtso`, `k3-am642-phyboard-electra-x27-gpio1-spi1-uart3.dtso` overlays |
| SoC variant missing blocks | `k3-am62d2.dtsi` includes `k3-am62a7.dtsi` and deletes the absent accelerators |

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

Keep logical entries visible, as `k3-am64-main.dtsi` does for SDHCI0 and ICSSG0:

```dts
reg = <0x00 0xfa10000 0x00 0x260>, <0x00 0xfa18000 0x00 0x134>;

clocks = <&k3_clks 81 0>,  /* icssg0_core_clk */
         <&k3_clks 81 3>,  /* icssg0_iep_clk */
         <&k3_clks 81 16>, /* icssg0_rgmii_mhz_250_clk */
         /* ... */
         <&k3_clks 81 20>; /* icssg0_iclk */
```

Per-entry comments name the provider clock behind each opaque ID. Align companion lists such as `clocks`/`clock-names` so reviewers can compare positions. A formatter cannot prove that the third name belongs to the third specifier, but consistent layout makes mistakes more visible.

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
