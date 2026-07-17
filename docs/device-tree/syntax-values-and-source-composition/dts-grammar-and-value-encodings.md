---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# DTS Grammar And Value Encodings

## File Shape

A complete version 1 DTS starts with `/dts-v1/;`, may contain memory reservations, and defines a root node:

```dts
/dts-v1/;

/memreserve/ 0x80000000 0x00100000;

/ {
        model = "Example Trainer Board";
};
```

The version directive is source-language metadata. `/memreserve/` creates an entry in the DTB memory reservation block; it is not a normal property under the root.

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

Properties must precede child nodes within a node. Every property and node definition ends with a semicolon after its value or closing brace.

```dts
uart0: serial@1000 {
        compatible = "example,trainer-uart";
        reg = <0x1000 0x100>;
        wakeup-source;

        diagnostic-channel {
                status = "disabled";
        };
};
```

Syntax does not establish correctness. Only bindings can say whether `diagnostic-channel` or `wakeup-source` is valid for this device.

## Strings

A DTS string is quoted and becomes a NUL-terminated byte sequence:

```dts
model = "Example Trainer Board";
```

Escape embedded characters where required:

```dts
message = "line one\nline two";
```

The binding decides whether a property accepts a string. Numeric text is still a string:

```dts
/* These encode different types. */
frequency-text = "24000000";
frequency-cell = <24000000>;
```

## String Lists

Comma-separated quoted strings form one property containing consecutive NUL-terminated strings:

```dts
compatible = "example,trainer-uart-v2", "example,trainer-uart";
clock-names = "bus", "core";
```

Order is often semantic. Compatible strings run from most specific to more general. Named-resource lists correspond positionally to the associated resource entries.

## Cells And Cell Arrays

Angle brackets encode integers as cells. A normal cell is 32 bits and stored big-endian in the DTB:

```dts
clock-frequency = <24000000>;
reg = <0x1000 0x100>;
```

Whitespace is not grouping. These are the same sequence of four cells:

```dts
values = <1 2 3 4>;
values = <1 2>, <3 4>;
```

Bindings give the group boundaries meaning. For a multi-entry `reg`, separate logical entries for readability:

```dts
reg = <0x1000 0x100>,
      <0x2000 0x80>;
```

Integer expressions can use C-like operators supported by `dtc`, but clarity matters:

```dts
buffer-size = <(4 * 1024)>;
```

Prefer a named binding constant or a direct value when an expression obscures the hardware fact.

## Values Wider Than 32 Bits

A 64-bit semantic value is commonly represented by two cells, most-significant cell first:

```dts
/* 0x0000000123456789 */
address = <0x00000001 0x23456789>;
```

This is not interchangeable with one overflowing default cell. The binding or parent cell-count properties determine when multiple cells form one value.

Use `/bits/ 64` when a property specifically calls for an array of 64-bit integer elements:

```dts
values-64 = /bits/ 64 <0x0000000123456789 0xabcdef0123456789>;
```

Do not use `/bits/ 64` merely because an address happens to exceed 32 bits. Bus address encoding normally uses the cell counts defined by the parent.

## Explicitly Sized Integer Arrays

`/bits/` changes the width of each integer element in the following array:

```dts
samples-8 = /bits/ 8 <0x01 0x7f 0xff>;
samples-16 = /bits/ 16 <0x1234 0xabcd>;
samples-64 = /bits/ 64 <0x0000000100000002>;
```

The binding must require or permit that width. An explicit width does not convert an arbitrary property into a valid one, and values must fit the selected element size.

## Byte Arrays

Square brackets encode raw bytes written as hexadecimal pairs:

```dts
local-mac-address = [02 00 00 00 00 01];
calibration-data = [12 34 ab cd];
```

There is no implicit 32-bit cell alignment inside the property value. Use byte arrays for bindings that specify bytes, not as a visual alternative to cells.

## Empty And Boolean Properties

A property with no assigned value has zero length:

```dts
wakeup-source;
dma-coherent;
```

Bindings often use presence as true and absence as false. These are not generally equivalent:

```dts
wakeup-source;
wakeup-source = <1>;
wakeup-source = "true";
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

Both block and line comments are supported:

```dts
/* Hardware erratum: register window is only 0x80 bytes. */
reg = <0x1000 0x80>;

status = "disabled"; // Connector is not fitted on this variant.
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
