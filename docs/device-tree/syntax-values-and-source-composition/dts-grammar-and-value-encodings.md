---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# DTS Grammar And Value Encodings

## File Shape

A complete version 1 DTS starts with `/dts-v1/;`, may contain memory reservations, and defines a root node. Arm's FVP model (`arch/arm64/boot/dts/arm/fvp-base-revc.dts`) shows all three:

```dts
/dts-v1/;

/memreserve/ 0x80000000 0x00010000;

/ {
        model = "FVP Base RevC";
        /* ... */
};
```

The version directive is source-language metadata. `/memreserve/` creates an entry in the DTB memory reservation block; it is not a normal property under the root. TI K3 boards such as the AM642 SK use `/reserved-memory` nodes instead (see [Reserved Memory](../standard-nodes-and-properties/reserved-memory.md)).

Included `.dtsi` fragments do not normally repeat `/dts-v1/;`. The final compilation unit must produce one root tree.

## Nodes And Properties

The basic forms are:

```text
[label:] node-name[@unit-address] {
        property-name = value;
        boolean-property;
        child-node { };
};
```

Properties must precede child nodes within a node. Every property and node definition ends with a semicolon after its value or closing brace. The AM642 SK SD/SDIO slot shows properties, a boolean, and a child node together (`k3-am642-sk.dts`):

```dts
&sdhci0 {
        status = "okay";
        vmmc-supply = <&wlan_en>;
        bus-width = <4>;
        non-removable;
        cap-power-off-card;
        keep-power-in-suspend;
        ti,driver-strength-ohm = <50>;

        #address-cells = <1>;
        #size-cells = <0>;
        wlcore: wlcore@2 {
                compatible = "ti,wl1837";
                reg = <2>;
                /* ... */
        };
};
```

Syntax does not establish correctness. Only the MMC and SDHCI bindings can say whether `non-removable` and the `wlcore@2` SDIO function child are valid here.

## Strings

A DTS string is quoted and becomes a NUL-terminated byte sequence:

```dts
model = "Texas Instruments AM642 SK";
```

Escape embedded characters where required:

```dts
message = "line one\nline two";
```

The binding decides whether a property accepts a string. Numeric text is still a string:

```dts
/* These encode different types. */
frequency-text = "48000000";   /* invented: a string, not a number */
clock-frequency = <48000000>;  /* main_uart0 in k3-am64-main.dtsi */
```

## String Lists

Comma-separated quoted strings form one property containing consecutive NUL-terminated strings:

```dts
compatible = "ti,am64-uart", "ti,am654-uart";
clock-names = "clk_ahb", "clk_xin";      /* sdhci0 in k3-am64-main.dtsi */
```

Order is often semantic. Compatible strings run from most specific to more general. Named-resource lists correspond positionally to the associated resource entries.

## Cells And Cell Arrays

Angle brackets encode integers as cells. A normal cell is 32 bits and stored big-endian in the DTB:

```dts
clock-frequency = <48000000>;
reg = <0x00 0x02800000 0x00 0x100>;
```

Whitespace is not grouping. These are the same sequence of four cells:

```dts
values = <1 2 3 4>;
values = <1 2>, <3 4>;
```

Bindings give the group boundaries meaning. For a multi-entry `reg`, separate logical entries for readability, as `sdhci0` does for its host-controller and PHY register windows:

```dts
reg = <0x00 0xfa10000 0x00 0x260>, <0x00 0xfa18000 0x00 0x134>;
```

Integer expressions can use C-like operators supported by `dtc`. TI's pinmux macro produces them: after preprocessing, `AM64X_IOPAD(0x0260, PIN_INPUT_PULLUP, 0)` becomes

```dts
(((0x0260) & 0x1fff)) ((((1 << (18)) | (1 << (14)) | (1 << (17) | (0 << (16))))) | (0))
```

which `dtc` evaluates to the two cells `0x260 0x64000`. Prefer a named binding constant or a direct value when an expression obscures the hardware fact.

## Values Wider Than 32 Bits

A 64-bit semantic value is commonly represented by two cells, most-significant cell first. The AM64x OSPI controller's second `reg` entry is a 4 GiB flash data window at `0x5_0000_0000` (`k3-am64-main.dtsi`):

```dts
reg = <0x00 0x0fc40000 0x00 0x100>,
      <0x05 0x00000000 0x01 0x00000000>;
```

This is not interchangeable with one overflowing default cell. The binding or parent cell-count properties determine when multiple cells form one value.

Use `/bits/ 64` when a property specifically calls for an array of 64-bit integer elements, as the OPP binding does for `opp-hz` (`k3-am625.dtsi`):

```dts
opp-1250000000 {
        opp-hz = /bits/ 64 <1250000000>;
        /* ... */
};
```

Do not use `/bits/ 64` merely because an address happens to exceed 32 bits. Bus address encoding normally uses the cell counts defined by the parent.

## Explicitly Sized Integer Arrays

`/bits/` changes the width of each integer element in the following array. The AM642 EVM PCIe endpoint overlay (`k3-am642-evm-pcie0-ep.dtso`) uses an 8-bit element because the endpoint binding defines `max-functions` as `uint8`:

```dts
max-functions = /bits/ 8 <1>;
```

The binding must require or permit that width. An explicit width does not convert an arbitrary property into a valid one, and values must fit the selected element size.

## Byte Arrays

Square brackets encode raw bytes written as hexadecimal pairs. Both AM64x CPSW ports carry a placeholder MAC that the bootloader overwrites (`k3-am64-main.dtsi`):

```dts
mac-address = [00 00 00 00 00 00];
```

There is no implicit 32-bit cell alignment inside the property value. Use byte arrays for bindings that specify bytes, not as a visual alternative to cells.

## Empty And Boolean Properties

A property with no assigned value has zero length:

```dts
non-removable;
regulator-always-on;
```

Bindings often use presence as true and absence as false. These are not generally equivalent:

```dts
non-removable;
non-removable = <1>;      /* invented: wrong encoding */
non-removable = "true";   /* invented: wrong encoding */
```

Only the first matches a boolean binding unless that binding explicitly defines another representation. There is no generic `false;` spelling; remove the property to represent absence.

## Concatenated Value Components

DTS can combine compatible value components in one property, and labels can mark positions within values. This is mainly useful for specialized low-level cases:

```dts
mixed = <1 2>, [aa bb], "tail";
marked = start: <1 middle: 2 3> end:;
```

Do not invent mixed encodings. A consumer needs a binding that defines the exact byte layout.

## Comments

Both block and line comments are supported. Real comments in the AM64x sources explain ownership that the property alone cannot express:

```dts
rng: rng@40910000 {
        compatible = "inside-secure,safexcel-eip76";
        /* ... */
        status = "disabled"; /* Used by OP-TEE */
};
```

In kernel sources, follow local convention and use comments to explain non-obvious hardware facts, not syntax.

## Type-Reading Checklist

For every property:

1. Open the binding.
2. Identify the required semantic type.
3. Check element count and grouping.
4. Identify positional relationships to companion properties.
5. For cells, determine whether parent or provider context changes decoding.
6. Inspect the compiled value if preprocessing or references are involved.

## Common Errors

- Writing a numeric string where the binding expects a cell.
- Assuming commas create binary subarrays or tuple metadata.
- Overflowing a 32-bit cell with a 64-bit literal.
- Using `/bits/ 64` instead of the parent bus's address-cell encoding.
- Encoding bytes as cells, changing both width and content.
- Writing `<0>` to mean a boolean property is false.
- Reordering one named-resource list without its positional companion.

## Exercises

1. Encode `0x123456789abcdef0` as two normal cells.
2. Explain the binary difference between `<0x12 0x34>` and `[12 34]`.
3. Write a three-entry string list and describe its NUL bytes.
4. Explain why `<1 2>, <3 4>` needs a binding before you can call it two pairs.
5. Correct `wakeup-source = "yes";` for a boolean binding.

## References And Next Step

- [Devicetree source format](https://devicetree-specification.readthedocs.io/en/stable/source-language.html)
- [Devicetree value types](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

Continue with [References, Amendments, And Deletions](references-amendments-and-deletions.md).
