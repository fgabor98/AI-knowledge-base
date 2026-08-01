---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Overlay Source, Fragments, And Target Selection

Overlay source describes changes relative to another tree. `/plugin/` tells `dtc` that unresolved references to the base are expected and must be emitted as relocation metadata instead of treated as ordinary unresolved-label errors.

## Minimal Label-Targeted Overlay

```dts
/dts-v1/;
/plugin/;

#include <dt-bindings/gpio/gpio.h>

&spi2 {
        status = "okay";

        sensor@0 {
                compatible = "acme,temp100";
                reg = <0>;
                spi-max-frequency = <1000000>;
                reset-gpios = <&gpio1 12 GPIO_ACTIVE_LOW>;
        };
};
```

`spi2` and `gpio1` are external symbols expected from the base. The overlay adds or updates content beneath the base node labeled `spi2`; it also creates an external phandle reference to the base node labeled `gpio1`.

The source is concise, but its contract includes both labels and the provider meaning of `gpio1`'s specifier cells.

## Explicit Fragment Form

The same target can be written explicitly:

```dts
/dts-v1/;
/plugin/;

/ {
        fragment@0 {
                target = <&spi2>;

                __overlay__ {
                        status = "okay";

                        sensor@0 {
                                compatible = "acme,temp100";
                                reg = <0>;
                                spi-max-frequency = <1000000>;
                        };
                };
        };
};
```

`target` identifies the existing node. `__overlay__` contains the subtree merged into that target. Shorthand references are normally clearer; explicit fragments are useful when inspecting generated structures, teaching resolution, or working with tooling that expects the form.

Fragment names and unit numbers identify overlay records, not hardware addresses. Keep them unique and stable enough for debugging, but do not expose them as a product ABI.

## Label Targets

```dts
&i2c2 {
        status = "okay";
};
```

Advantages:

- independent of the target node's full path
- survives path changes when the label remains exported
- expresses a named base integration point
- preferred by upstream Linux overlay documentation

Requirements:

- the base source has the label
- the deployed base DTB contains a matching `__symbols__` entry
- the label continues to mean the same hardware and binding contract
- any providers referenced by the overlay also export required symbols

A source label normally disappears from an ordinary DTB. It becomes an externally usable symbol only when the base is built with symbol generation.

## Path Targets

Path-reference shorthand:

```dts
&{/soc/i2c@2000000} {
        status = "okay";
};
```

Explicit equivalent:

```dts
fragment@0 {
        target-path = "/soc/i2c@2000000";

        __overlay__ {
                status = "okay";
        };
};
```

Path targeting can work without an exported base label for that target. It is more tightly coupled to hierarchy, bus naming, unit addresses, and source refactoring. A stable path is still a compatibility promise.

Path targeting does not eliminate all symbol requirements. An overlay targeted by path can still refer to `&gpio1`, `&clk`, or another base label inside its properties.

## Target The Owning Node

Choose the node whose binding owns the change:

- add an SPI peripheral under the SPI controller
- add a pin group under the pin controller
- add a fixed regulator under the binding-defined parent or root
- attach an endpoint under the correct `port`
- set a board-specific property on the actual consumer

Do not target `/` merely because it is easy, then recreate paths or duplicate existing hardware. A new child with a slightly different name may coexist with the original and cause duplicate devices rather than replacing it.

## Multiple Targets

A module can require several fragments:

```dts
&pinctrl {
        module_pins: module-pins {
                pins = "gpio12", "gpio13";
                function = "spi2";
        };
};

&spi2 {
        pinctrl-names = "default";
        pinctrl-0 = <&module_pins>;
        status = "okay";

        sensor@0 {
                compatible = "acme,temp100";
                reg = <0>;
        };
};
```

`module_pins` is local to the overlay. `pinctrl` and `spi2` are external. The compiler must preserve both categories so the resolver can relocate local references and resolve base references.

Source order can matter when several fragments write the same target/property, but relying on intra-overlay last-writer behavior obscures ownership. Prefer one fragment per target or non-overlapping changes.

## Properties Are Replaced, Nodes Are Merged

When an overlay supplies a property already present on the target, the merged value replaces the prior value for that applied changeset. When it supplies a child node with the same full name, content merges into that node; a differently named child is an additional node.

Before writing an overlay, record the expected prior state:

```text
target: /soc/spi@2000000
prior status: disabled
prior child sensor@0: absent
overlay postcondition: status okay, exactly one sensor@0
```

This turns accidental double application or a changed base into a detectable precondition failure.

## Deletion Is Not A Portable Overlay Primitive

`/delete-node/` and `/delete-property/` are source-composition directives. A normal flattened DTB does not contain a generic “delete this base item later” tombstone. Do not assume that placing source deletion syntax in an overlay creates a portable runtime deletion operation.

Some ecosystems add tool-specific mechanisms. Treat those as explicit platform formats, not generic DT overlay behavior. For portable designs, prefer positive descriptions, separate base DTBs, or disabling a node when the binding and lifecycle make that safe.

Removing an entire applied live overlay is different: the overlay framework reverts the changeset it recorded. That is not an arbitrary delete operation against the base.

## Source Review Checklist

- Is `/plugin/` present?
- Does each target identify the true owner of the change?
- Are all base labels/path targets declared compatibility interfaces?
- Are local labels distinguished from external labels?
- Are provider specifiers valid for the selected base providers?
- Does each expected prior state have a postcondition?
- Could a child name create a duplicate rather than update the intended node?
- Are multiple fragments non-conflicting?
- Is any supposed deletion actually supported by the selected toolchain?
- Does the overlay describe optional physical hardware rather than product policy?

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to [Symbols, Fixups, Local Fixups, And Compilation](symbols-fixups-local-fixups-and-compilation.md).
