---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# Tree Anatomy And Vocabulary

## Start With One Tree

The same small tree is used throughout this page:

```dts
/dts-v1/;

/ {
        model = "Example Trainer Board";
        compatible = "example,trainer-board";
        #address-cells = <1>;
        #size-cells = <1>;

        aliases {
                serial0 = &uart0;
        };

        chosen {
                stdout-path = "serial0:115200n8";
        };

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

                uart0: serial@1000 {
                        compatible = "example,trainer-uart";
                        reg = <0x1000 0x100>;
                        clocks = <&osc>;
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
├── clock-24000000
└── soc
    └── serial@1000
```

This parent-child relationship often mirrors hardware containment or bus topology, but not every node is a physical device. `/chosen` communicates boot configuration; `/aliases` supplies shorthand paths; a sound or graph node may describe relationships among components.

## Node Names And Unit Addresses

A node name has the general form:

```text
node-name@unit-address
```

For `serial@1000`:

- `serial` is a generic device-class name
- `1000` is the unit address
- the parent bus defines what that address means
- when a node has `reg`, its unit address normally matches the first address in `reg`

The unit address is not automatically a CPU physical address. Under an I2C controller, `sensor@48` commonly means slave address `0x48`. Under a chip-select-based SPI controller, `flash@0` may mean chip select 0. Under a translated memory bus, `serial@1000` can map to a different CPU address through `ranges`.

If a node has no address in its parent address space, omit `@unit-address`:

```dts
chosen { };
aliases { };
gpio-keys { };
```

Use the generic node name defined by a binding when one exists. The compatible string identifies the programming model more precisely than a decorative node name would.

## Properties

A property has a name and zero or more bytes of value. DTS provides notation for common encodings:

```dts
model = "Example Trainer Board";             /* string */
compatible = "example,uart-v2",              /* string list */
             "example,uart";
reg = <0x1000 0x100>;                         /* cells */
mac-address = [00 11 22 33 44 55];            /* bytes */
wakeup-source;                                /* empty/boolean */
```

At binary level, all property values are bytes. The binding tells consumers whether those bytes encode a string, integer cells, a phandle and arguments, raw data, or another defined structure.

### Strings And String Lists

Strings are NUL-terminated in the DTB. A string list is multiple NUL-terminated strings concatenated in one property value. This matters when inspecting runtime files: a normal `cat` may hide separators or produce a run-together result.

```sh
tr '\0' '\n' < /sys/firmware/devicetree/base/compatible
```

### Cells

A cell is a 32-bit big-endian unit. Angle brackets express one or more cells:

```dts
clock-frequency = <24000000>;
reg = <0x1000 0x100>;
```

The number of cells in a logical value is contextual. In the example, the parent bus declares one address cell and one size cell, so `reg` contains one address/size pair. On a bus with two address cells and two size cells, the same conceptual pair uses four cells.

Do not count values by visual intuition. Read the consumer property binding and the relevant provider or parent-bus binding.

### Empty Properties

Presence can represent true:

```dts
wakeup-source;
```

Do not write an invented numeric value unless the binding requests one. Absence generally represents false, but the binding may define defaults or constraints.

## Paths

A full path identifies a node by walking from the root:

```text
/
/soc
/soc/serial@1000
```

Paths are part of the logical tree. They can change if a node moves or its unit address changes, so source references usually prefer labels when composing a tree.

## Labels

A label is source syntax placed before a node or property:

```dts
uart0: serial@1000 { };
```

It allows another source fragment to refer to that object:

```dts
&uart0 {
        status = "okay";
};
```

Important distinctions:

- `uart0` is not the node name
- `uart0` is not the runtime path
- renaming a label does not rename `serial@1000`
- labels normally disappear as source constructs during ordinary DTB compilation
- compiling with symbol support can preserve a label-to-path symbol table for overlay resolution

Linux source style restricts labels more tightly than the broad source grammar: lowercase letters, digits, and underscores are the normal set.

## Phandles

A phandle is a unique 32-bit identifier for a node in the compiled tree. It allows properties to form links outside the structural parent-child hierarchy.

Source usually uses a label reference:

```dts
clocks = <&osc>;
```

The compiler resolves `&osc` and emits a numeric phandle. A decompiled tree might therefore contain something like:

```dts
clock-24000000 {
        phandle = <0x01>;
};

serial@1000 {
        clocks = <0x01>;
};
```

The number itself has no portable meaning. Never hard-code or compare phandle numbers across independently compiled blobs.

Provider bindings can require arguments after the phandle:

```dts
clocks = <&clock_controller 7>;
```

This cannot be decoded as “node 7.” It means “reference `clock_controller`, followed by a provider-defined specifier.” The provider's `#clock-cells` and binding explain the number and meaning of those argument cells.

## Aliases

The `/aliases` node maps short names to paths:

```dts
aliases {
        serial0 = &uart0;
};
```

The source reference is resolved so the property value represents the target path. Aliases can provide stable logical identifiers such as `serial0` when a consumer understands that alias class.

An alias is neither a label nor a general-purpose symbolic link:

| Mechanism | Defined where | Primary role |
|---|---|---|
| label | DTS source | source composition and references |
| phandle | compiled tree | node-to-node references |
| full path | logical tree | unique structural identity |
| alias | `/aliases` node | consumer-visible shorthand for a path |

## Comments And Style

DTS accepts C-style block comments, and current tooling commonly accepts C++-style line comments as well. Kernel DTS should follow the project's published coding style and the local architecture convention.

Useful comments explain information that the binding and schematic do not make obvious:

```dts
/* R42 is not fitted on board revisions A0 and A1. */
status = "disabled";
```

Avoid comments that merely restate syntax:

```dts
status = "okay"; /* Enable the node. */
```

For Linux DTS, prefer:

- lowercase node and property names
- lowercase hexadecimal unit addresses without unnecessary leading zeroes
- generic node names
- stable node and property ordering
- `compatible`, then `reg`, common properties, vendor properties, `status`, and child nodes
- hardware-based DTSI/DTS layering

Style is not cosmetic at product scale. Predictable ordering reduces merge conflicts and makes review omissions easier to spot.

## Read This Node Methodically

Given:

```dts
uart0: serial@1000 {
        compatible = "example,trainer-uart";
        reg = <0x1000 0x100>;
        clocks = <&osc>;
        status = "okay";
};
```

Read it in this order:

1. Find the parent and identify its bus binding.
2. Check that `serial@1000` follows the binding's generic node-name rule.
3. Use the parent cell counts to decode `reg`.
4. Open the binding for `example,trainer-uart`.
5. Confirm required and optional properties.
6. Resolve `&osc`, then use the provider binding to decode `clocks`.
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

1. Write the full path for `uart0` in the example tree.
2. Explain why its unit address is `1000` rather than `0x1000`.
3. State how many address/size pairs its `reg` contains.
4. Explain which source token causes the compiler to create a phandle.
5. Describe the different roles of `uart0`, `serial@1000`, `/soc/serial@1000`, and `serial0`.
6. Change the parent to `#address-cells = <2>` and `#size-cells = <1>`. Rewrite `reg` for address `0x1000`, size `0x100`.

## References

- [Devicetree Specification: logical tree and standard properties](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)

## Next Step

Apply the vocabulary in [First Device Tree Lab](first-device-tree-lab.md).
