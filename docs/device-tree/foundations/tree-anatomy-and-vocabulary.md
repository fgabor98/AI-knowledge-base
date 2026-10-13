---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# Tree Anatomy And Vocabulary

## Start With One Tree

This page uses a trimmed, self-contained excerpt of the TI AM642 SK description from mainline Linux (`arch/arm64/boot/dts/ti/k3-am64.dtsi`, `k3-am64-main.dtsi`, and `k3-am642-sk.dts`). Macros are expanded, and unrelated nodes, interrupts, power domains, and pins are omitted so the file compiles with standalone `dtc`:

```dts
/dts-v1/;

/ {
        model = "Texas Instruments AM642 SK";
        compatible = "ti,am642-sk", "ti,am642";
        #address-cells = <2>;
        #size-cells = <2>;

        aliases {
                serial2 = &main_uart0;
        };

        chosen {
                stdout-path = &main_uart0;
        };

        cbass_main: bus@f4000 {
                compatible = "simple-bus";
                #address-cells = <2>;
                #size-cells = <2>;
                ranges = <0x00 0x01000000 0x00 0x01000000 0x00 0x02330400>, /* First peripheral window */
                         <0x00 0x44043000 0x00 0x44043000 0x00 0x00000fe0>; /* TI SCI DEBUG */

                dmsc: system-controller@44043000 {
                        compatible = "ti,k2g-sci";
                        ti,host-id = <12>;
                        reg-names = "debug_messages";
                        reg = <0x00 0x44043000 0x00 0xfe0>;

                        k3_clks: clock-controller {
                                compatible = "ti,k2g-sci-clk";
                                #clock-cells = <2>;
                        };
                };

                main_uart0: serial@2800000 {
                        compatible = "ti,am64-uart", "ti,am654-uart";
                        reg = <0x00 0x02800000 0x00 0x100>;
                        clock-frequency = <48000000>;
                        clocks = <&k3_clks 146 0>;
                        clock-names = "fclk";
                        status = "okay";
                };
        };
};
```

This is source syntax. The binding for each node and property determines the semantic meaning of its values.

## Root And Child Nodes

The root node is written `/`. Every other node has exactly one structural parent:

```text
/
├── aliases
├── chosen
└── bus@f4000
    ├── system-controller@44043000
    │   └── clock-controller
    └── serial@2800000
```

This parent-child relationship often mirrors hardware containment or bus topology, but not every node is a physical device. `/chosen` communicates boot configuration; `/aliases` supplies shorthand paths; `clock-controller` is a software interface to the TI-SCI system firmware rather than a block of registers.

## Node Names And Unit Addresses

A node name has the general form:

```text
node-name@unit-address
```

For `serial@2800000`:

- `serial` is a generic device-class name
- `2800000` is the unit address
- the parent bus defines what that address means
- when a node has `reg`, its unit address normally matches the first address in `reg`; here the two address cells `0x00 0x02800000` form `0x2800000`

The unit address is not automatically a CPU physical address. Under an I2C controller, `tmp100@48` on the same board means target address `0x48`. Under the OSPI controller, `flash@0` means chip select 0. On K3 SoCs the `bus@f4000` window happens to be an identity mapping, so `serial@2800000` is also CPU physical address `0x2800000`, but that is a property of this `ranges`, not of unit addresses in general.

If a node has no address in its parent address space, omit `@unit-address`:

```dts
chosen { };
aliases { };
clock-controller { };
```

Use the generic node name defined by a binding when one exists. The compatible string identifies the programming model more precisely than a decorative node name would.

## Properties

A property has a name and zero or more bytes of value. DTS provides notation for common encodings, all taken from the AM642 sources:

```dts
model = "Texas Instruments AM642 SK";            /* string */
compatible = "ti,am64-uart", "ti,am654-uart";    /* string list */
reg = <0x00 0x02800000 0x00 0x100>;              /* cells */
mac-address = [00 00 00 00 00 00];               /* bytes (CPSW port, filled by firmware) */
non-removable;                                   /* empty/boolean (SK eMMC/SDIO slot) */
```

At binary level, all property values are bytes. The binding tells consumers whether those bytes encode a string, integer cells, a phandle and arguments, raw data, or another defined structure.

### Strings And String Lists

Strings are NUL-terminated in the DTB. A string list is multiple NUL-terminated strings concatenated in one property value. This matters when inspecting runtime files: a normal `cat` may hide separators or produce a run-together result.

```sh
tr '\0' '\n' < /sys/firmware/devicetree/base/compatible
```

On an AM642 SK this prints `ti,am642-sk` and `ti,am642` on separate lines.

### Cells

A cell is a 32-bit big-endian unit. Angle brackets express one or more cells:

```dts
clock-frequency = <48000000>;
reg = <0x00 0x02800000 0x00 0x100>;
```

The number of cells in a logical value is contextual. `bus@f4000` declares two address cells and two size cells, so the four cells of `reg` form one address/size pair: address `0x0000000002800000`, size `0x100`. On a bus with one address cell and one size cell, the same pair would be written `<0x02800000 0x100>`.

Do not count values by visual intuition. Read the consumer property binding and the relevant provider or parent-bus binding.

### Empty Properties

Presence can represent true:

```dts
non-removable;
```

Do not write an invented numeric value unless the binding requests one. Absence generally represents false, but the binding may define defaults or constraints.

## Paths

A full path identifies a node by walking from the root:

```text
/
/bus@f4000
/bus@f4000/serial@2800000
```

Paths are part of the logical tree. They can change if a node moves or its unit address changes, so source references usually prefer labels when composing a tree.

## Labels

A label is source syntax placed before a node or property:

```dts
main_uart0: serial@2800000 { };
```

It allows another source fragment—here `k3-am642-sk.dts`—to refer to that object:

```dts
&main_uart0 {
        status = "okay";
};
```

Important distinctions:

- `main_uart0` is not the node name
- `main_uart0` is not the runtime path
- renaming a label does not rename `serial@2800000`
- labels normally disappear as source constructs during ordinary DTB compilation
- compiling with symbol support can preserve a label-to-path symbol table for overlay resolution

Linux source style restricts labels to lowercase letters, digits, and underscores.

## Phandles

A phandle is a unique 32-bit identifier for a node in the compiled tree. It allows properties to form links outside the structural parent-child hierarchy.

Source usually uses a label reference:

```dts
clocks = <&k3_clks 146 0>;
```

The compiler resolves `&k3_clks` and emits a numeric phandle. A decompiled tree might therefore contain something like:

```dts
clock-controller {
        phandle = <0x01>;
};

serial@2800000 {
        clocks = <0x01 0x92 0x00>;
};
```

The number itself has no portable meaning. Never hard-code or compare phandle numbers across independently compiled blobs.

The two cells after the phandle are arguments required by the provider. This entry cannot be decoded as “node 146.” It means “reference `k3_clks`, followed by a provider-defined specifier.” The provider's `#clock-cells = <2>` says two argument cells follow, and the `ti,k2g-sci-clk` binding defines them as the TI-SCI device ID (146, UART0) and the clock ID within that device (0).

## Aliases

The `/aliases` node maps short names to paths:

```dts
aliases {
        serial2 = &main_uart0;
};
```

Outside angle brackets, the label reference is resolved to the target's full path string, `/bus@f4000/serial@2800000`. The same happens for `stdout-path = &main_uart0;` in `/chosen`. Aliases provide logical identifiers such as `serial2` when a consumer understands that alias class.

An alias is neither a label nor a general-purpose symbolic link:

| Mechanism | Defined where | Primary role |
|---|---|---|
| label | DTS source | source composition and references |
| phandle | compiled tree | node-to-node references |
| full path | logical tree | unique structural identity |
| alias | `/aliases` node | consumer-visible shorthand for a path |

## Comments And Style

DTS accepts C-style block comments, and current tooling commonly accepts C++-style line comments as well. Useful comments explain information that the binding and schematic do not make obvious. The SK sources contain good examples:

```dts
&main_uart1 {
        /* main_uart1 is reserved for firmware usage */
        status = "reserved";
};
```

Avoid comments that merely restate syntax, such as `status = "okay"; /* Enable the node. */`. Linux's naming, ordering, and layering rules are covered in [Hardware-Based Layering And Source Style](../syntax-values-and-source-composition/hardware-based-layering-and-source-style.md#linux-source-style).

## Read This Node Methodically

Given:

```dts
main_uart0: serial@2800000 {
        compatible = "ti,am64-uart", "ti,am654-uart";
        reg = <0x00 0x02800000 0x00 0x100>;
        clocks = <&k3_clks 146 0>;
        clock-names = "fclk";
        status = "okay";
};
```

Read it in this order:

1. Find the parent (`bus@f4000`) and identify its bus binding (`simple-bus`).
2. Check that `serial@2800000` follows the binding's generic node-name rule.
3. Use the parent cell counts to decode `reg`.
4. Open the binding for the compatible list (`Documentation/devicetree/bindings/serial/8250_omap.yaml`).
5. Confirm required and optional properties.
6. Resolve `&k3_clks`, then use the provider binding to decode `clocks`.
7. Check availability through `status` and all required parent/provider nodes.
8. Only then trace the compatible string into driver matching.

## Common Reading Errors

- Treating every number inside `<...>` as one complete value.
- Decoding a child's `reg` using the child's cell counts instead of its parent's.
- Assuming `&label` exists at runtime as a textual name.
- Assuming an alias selects a driver.
- Reading `status` as the only availability condition while a parent bus is disabled.
- Assuming a node name uniquely identifies the hardware programming model.
- Treating a phandle value as a physical address.

## Exercises

1. Write the full path for `main_uart0` in the example tree.
2. Explain why its unit address is `2800000` rather than `0x2800000` or `0000000002800000`.
3. State how many address/size pairs its `reg` contains.
4. Explain which source token causes the compiler to create a phandle on `clock-controller`.
5. Describe the different roles of `main_uart0`, `serial@2800000`, `/bus@f4000/serial@2800000`, and `serial2`.
6. Change `bus@f4000` to `#address-cells = <1>` and `#size-cells = <1>` and rewrite `reg` and `ranges` accordingly.

## References

- [Devicetree Specification: logical tree and standard properties](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
- [Mainline AM64x device trees](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/arch/arm64/boot/dts/ti)

## Next Step

Apply the vocabulary in [First Device Tree Lab](first-device-tree-lab.md).
