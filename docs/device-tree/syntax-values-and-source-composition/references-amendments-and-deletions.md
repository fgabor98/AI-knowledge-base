---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# References, Amendments, And Deletions

## References Are Context-Sensitive

A label identifies source material:

```dts
osc: clock-24000000 {
        compatible = "fixed-clock";
        #clock-cells = <0>;
        clock-frequency = <24000000>;
};
```

Inside a cell array, `&osc` becomes a phandle cell:

```dts
clocks = <&osc>;
```

Outside a cell array, a node reference becomes its full path string:

```dts
aliases {
        clock0 = &osc;
};
```

The same token therefore produces a different binary representation based on context. Decompile or query the DTB when this distinction matters.

## Label And Path References

Labels are preferred for maintainable in-tree composition:

```dts
&uart0 {
        status = "okay";
};
```

A full-path reference can target an unlabeled node:

```dts
&{/soc/serial@1000} {
        status = "okay";
};
```

Path references are more fragile because renaming or moving any path component breaks the reference. They can still be useful when modifying imported source that does not expose a suitable label.

Labels can also mark properties and locations within values, but node labels cover most ordinary DTS composition.

## Extending A Previously Defined Node

An included SoC description can define a disabled controller:

```dts
uart0: serial@1000 {
        compatible = "example,trainer-uart";
        reg = <0x1000 0x100>;
        status = "disabled";
};
```

The board amends the same node:

```dts
&uart0 {
        clocks = <&osc>;
        pinctrl-0 = <&uart0_pins>;
        pinctrl-names = "default";
        status = "okay";
};
```

The effective tree contains one `serial@1000` node with the combined properties. Repeating an existing property replaces its earlier value:

```dts
&uart0 {
        status = "okay";
};
```

This is source merge behavior, not a runtime overlay and not object-oriented inheritance.

## Reopening By Structural Path

Source can also repeat a path structurally when composition order permits:

```dts
/ {
        soc {
                serial@1000 {
                        status = "okay";
                };
        };
};
```

Label amendments are usually clearer and less error-prone. Structural reopening can accidentally create a parallel node when a name or unit address differs by one character.

## Deleting A Property

Use `/delete-property/` inside the effective node:

```dts
&uart0 {
        /delete-property/ wakeup-source;
};
```

This removes the property from the composed source tree. It is different from assigning an empty value:

```dts
/* This creates or replaces a zero-length property. */
wakeup-source;
```

Deletion is ordering-sensitive: the property must have been introduced before the deletion for the intended result, and a later include can add it again.

## Deleting A Node

Delete a child by name within its parent:

```dts
&i2c0 {
        /delete-node/ sensor@48;
};
```

Or delete a labeled node by reference where supported by the source layout:

```dts
/delete-node/ &unused_sensor;
```

Deleting is stronger than disabling:

| Action | Node remains in effective tree? | Typical intent |
|---|---:|---|
| `status = "disabled"` | yes | hardware block exists but is unavailable on this board |
| `/delete-node/` | no | inherited description is not part of the effective hardware model or must be removed structurally |

Prefer disabling SoC-integrated hardware that exists in silicon. Delete inherited board-level nodes when the underlying component truly is absent and the layering cannot be corrected more cleanly.

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
2. Show how `serial0 = &uart0;` and `clocks = <&osc>;` encode their targets differently.
3. Remove an inherited boolean property without leaving a zero-length value.
4. Explain when disabling is more truthful than deleting.
5. Rewrite a structural path amendment using a label.

## References And Next Step

- [Devicetree source format](https://devicetree-specification.readthedocs.io/en/stable/source-language.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

Continue with [Includes, Preprocessing, And Macros](includes-preprocessing-and-macros.md).
