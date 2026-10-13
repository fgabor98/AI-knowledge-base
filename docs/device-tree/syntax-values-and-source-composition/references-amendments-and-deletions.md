---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# References, Amendments, And Deletions

## References Are Context-Sensitive

A label identifies source material. In `k3-am64-main.dtsi`:

```dts
k3_clks: clock-controller {
        compatible = "ti,k2g-sci-clk";
        #clock-cells = <2>;
};

main_uart0: serial@2800000 { /* ... */ };
```

Inside a cell array, a reference becomes a phandle cell:

```dts
clocks = <&k3_clks 146 0>;
```

Outside a cell array, a node reference becomes its full path string. The AM642 SK board uses this in `/chosen`:

```dts
chosen {
        stdout-path = &main_uart0;      /* "/bus@f4000/serial@2800000" */
};
```

The same token therefore produces a different binary representation based on context. Decompile or query the DTB when this distinction matters.

## Label And Path References

Labels are preferred for maintainable in-tree composition:

```dts
&main_uart0 {
        status = "okay";
};
```

A full-path reference can name an unlabeled node. NVIDIA's Tegra20 boards use one inside a cell array (`arch/arm/boot/dts/nvidia/tegra20-ventana.dts`):

```dts
i2c-parent = <&{/i2c@7000c400}>;
```

TI's overlays use the root path to amend `/`, which has no label (`k3-am642-evm-icssg1-dualemac.dtso`):

```dts
&{/} {
        aliases {
                ethernet1 = "/icssg1-eth/ethernet-ports/port@1";
        };
};
```

Path references are more fragile because renaming or moving any path component breaks the reference. They are useful when the target has no suitable label.

Labels can also mark properties and locations within values, but node labels cover most ordinary DTS composition.

## Extending A Previously Defined Node

`k3-am64-main.dtsi` defines `main_uart0` and `main_uart1` with `status = "disabled"`. Board files amend them. In `k3-am642-sk.dts`:

```dts
&main_uart0 {
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&main_uart0_pins_default>;
};

&main_uart1 {
        /* main_uart1 is reserved for firmware usage */
        status = "reserved";
        pinctrl-names = "default";
        pinctrl-0 = <&main_uart1_pins_default>;
};
```

The effective tree contains one `serial@2800000` node with the combined properties. Repeating an existing property replaces its earlier value: `status` changes from `"disabled"` to `"okay"` for UART0 and to `"reserved"` for UART1.

This is source merge behavior, not a runtime overlay and not object-oriented inheritance.

## Reopening By Structural Path

Source can also repeat a path structurally when composition order permits. This is legal but not how the TI sources do it:

```dts
/* Illustrative equivalent of &main_uart0 { status = "okay"; }; */
/ {
        bus@f4000 {
                serial@2800000 {
                        status = "okay";
                };
        };
};
```

Label amendments are usually clearer and less error-prone. Structural reopening can accidentally create a parallel node when a name or unit address differs by one character.

## Deleting A Property

Use `/delete-property/` inside the effective node. The SolidRun HummingBoard-T has a USB3 variant that includes the base board DTS, whose SoM include sets `ti,usb2-only` on `usbss0`, and removes it (`k3-am642-hummingboard-t-usb3.dts`):

```dts
#include "k3-am642-hummingboard-t.dts"

&usbss0 {
        /delete-property/ ti,usb2-only;
};
```

This removes the property from the composed source tree. It is different from assigning an empty value: writing `ti,usb2-only;` again would create or keep a zero-length property, which still means true.

Deletion is ordering-sensitive: the property must have been introduced before the deletion for the intended result, and a later include can add it again.

## Deleting A Node

Delete a child by name within its parent. The Seeed BeagleBone Green Eco reuses the shared BeagleBone include but has a different PMIC, so it removes the inherited TPS65217 node and adds its own (`arch/arm/boot/dts/ti/omap/am335x-bonegreen-eco.dts`):

```dts
&i2c0 {
        /delete-node/ pmic@24;

        tps65214: pmic@30 {
                compatible = "ti,tps65214";
                reg = <0x30>;
                /* ... */
        };
};
```

Or delete a labeled node by reference. The AM62D2 SoC reuses the AM62A description but lacks two accelerators (`k3-am62d2.dtsi`):

```dts
#include "k3-am62a7.dtsi"

/delete-node/ &vpu;   /* Video Codec is disabled in AM62D2 SoC */
/delete-node/ &e5010; /* JPEG Encoder is disabled in AM62D2 SoC */
```

Deleting is stronger than disabling:

| Action | Node remains in effective tree? | Typical intent |
|---|---:|---|
| `status = "disabled"` | yes | hardware block exists but is unavailable on this board |
| `/delete-node/` | no | inherited description is not part of this hardware at all |

Prefer disabling SoC-integrated hardware that exists in silicon but is unused on a board. Delete when the component truly is absent—a different PMIC on the board, or a block missing from the SoC variant as on AM62D2—and the layering cannot be corrected more cleanly.

## Composition Order

Think of source processing as ordered amendments:

```text
base definition
-> included amendments
-> later property replacements
-> deletions
-> still later amendments
-> final effective tree
```

This makes include order observable. Two files that both set the same property are not independent; the later effective definition wins. Excessive reliance on ordering makes a product tree hard to review.

## References Do Not Prove Availability

A syntactically resolved reference can still be semantically unusable:

- the provider node may be disabled
- its parent bus may be disabled
- the specifier may have the wrong number of cells
- the provider binding may assign a different meaning to the arguments
- the provider driver may be absent

Reference resolution proves that a target exists in the source graph, not that the hardware dependency works.

## Safe Review Method

When reviewing an amendment:

1. Locate the original labeled node.
2. Read every included amendment in effective order.
3. Construct the final property set.
4. Confirm that replacements preserve type and cardinality.
5. Check deletions against hardware reality.
6. Resolve all references and provider cell counts.
7. Decompile the built DTB when order is unclear.
8. Run binding validation on the effective result.

## Common Errors

- Treating `&label` outside `<...>` as a phandle.
- Using a path reference when a stable label is available.
- Believing two amendments create two runtime nodes.
- Using an empty property to delete an inherited property.
- Deleting an SoC block merely because a board does not use it.
- Depending on include order without documenting ownership.
- Fixing a bad common DTSI through many per-board deletions instead of correcting the layer.

## Exercises

1. Predict the final `status` after a base sets `disabled` and two later amendments set `okay`, then `disabled`.
2. Show how `stdout-path = &main_uart0;` and `clocks = <&k3_clks 146 0>;` encode their targets differently.
3. Remove an inherited boolean property without leaving a zero-length value.
4. Explain when disabling is more truthful than deleting.
5. Rewrite a structural path amendment using a label.

## References And Next Step

- [Devicetree source format](https://devicetree-specification.readthedocs.io/en/stable/source-language.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

Continue with [Includes, Preprocessing, And Macros](includes-preprocessing-and-macros.md).
