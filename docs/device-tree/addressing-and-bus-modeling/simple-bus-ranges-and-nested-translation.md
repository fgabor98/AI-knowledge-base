---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# `simple-bus`, `ranges`, And Nested Translation

`ranges` maps child-bus addresses into the parent bus. A nested device reaches the CPU only by satisfying every required mapping boundary on its path.

## `ranges` Entry Shape

Each ordinary `ranges` entry contains:

```text
child-bus address + parent-bus address + length
```

The widths come from two levels:

| Field | Width source |
|---|---|
| child-bus address | bus node's `#address-cells` |
| parent-bus address | bus node's parent's `#address-cells` |
| length | bus node's `#size-cells` |

The bus binding may refine the child-address format, as PCI does.

## Empty, Populated, And Absent

These forms are not equivalent:

```dts
ranges;                 /* Explicit identity mapping. */
ranges = <...>;         /* Explicit translated windows. */
/* no ranges property */ /* No mapping is defined by the generic rule. */
```

An empty `ranges` says the child and parent address spaces are identical for the relevant domain. An absent property does **not** generically mean identity. Some bus bindings or operating-system conventions define special behavior, but relying on an undocumented exception makes the tree non-portable and difficult to review.

## One Translation Boundary

```dts
/ {
        #address-cells = <2>;
        #size-cells = <2>;

        soc {
                compatible = "simple-bus";
                #address-cells = <1>;
                #size-cells = <1>;
                ranges = <0x00000000 0x00000000 0x40000000 0x01000000>;

                uart@2000 {
                        reg = <0x2000 0x100>;
                };
        };
};
```

The `ranges` entry has one child-address cell, two parent-address cells, and one length cell. It maps child `[0, 0x01000000)` to parent `[0x40000000, 0x41000000)`. The UART child address `0x2000` translates to CPU physical `0x40002000`; its size remains `0x100`.

Translation formula inside one window:

```text
parent = parent_base + (child - child_base)
```

The entire child resource must fit the selected window. Checking only its first byte misses resources that cross a boundary.

## Nested Translation

Add a bridge inside the SoC bus:

```dts
bridge@800000 {
        compatible = "simple-bus";
        reg = <0x800000 0x10000>;
        #address-cells = <1>;
        #size-cells = <1>;
        ranges = <0x0000 0x800000 0x10000>;

        timer@100 {
                reg = <0x100 0x40>;
        };
};
```

Walk outward one boundary at a time:

1. Bridge child `0x100` maps to SoC-bus `0x800100`.
2. SoC-bus `0x800100` maps to root/CPU `0x40800100`.
3. The timer interval is `[0x40800100, 0x40800140)`.

Never add every visible base address blindly. At each boundary, first identify the matching child window and subtract that window's child base. Non-zero child bases and disjoint windows make “sum the bases” reasoning fail.

## Multiple And Disjoint Windows

A bus can map several regions:

```dts
ranges = <0x00000000 0x00000000 0x40000000 0x00100000>,
         <0x10000000 0x00000008 0x00000000 0x01000000>;
```

Under one-child/two-parent/one-size cell counts, the first child MiB maps at `0x40000000`; child `[0x10000000, 0x11000000)` maps at `0x800000000`. A child in the gap has no defined parent mapping.

Check for:

- overlapping child windows that make selection ambiguous
- overlapping parent windows that alias unrelated resources
- arithmetic overflow at `base + length`
- a resource straddling two windows
- a mapping wider than the parent can represent

## What `simple-bus` Means

`compatible = "simple-bus"` identifies a transparent or simply translated memory-mapped bus whose children can be enumerated directly. Such a node normally supplies explicit child cell counts and `ranges`.

It does not mean:

- every child is automatically functional
- the bus has no clocks, power, resets, or access controls
- missing `ranges` should be treated as identity
- all child devices are platform devices under every operating system
- the bus node needs no SoC-specific compatible when hardware control registers exist

If a block has configuration registers or a driver-managed programming model, follow its binding. Do not add `simple-bus` merely to force child creation.

## Linux Translation Path

Linux functions such as `of_address_to_resource()` and `of_iomap()` walk the applicable OF bus translators and `ranges` properties. That traversal may be bus-specific. The final resource can be compared with `/proc/iomem`, platform-device resource files, and driver logs.

When translation fails, inspect:

1. raw child `reg`
2. immediate-parent cell counts
3. each `ranges` property and matching interval
4. bus-compatible-specific translation rules
5. the final DTB rather than only included source fragments
6. kernel logs for resource or population errors

## Senior Review Checklist

- Is every bus boundary modeled, or has a physical interconnect been flattened without justification?
- Are identity mappings explicit?
- Do windows reflect hardware decoder programming and boot firmware state?
- Are secure or privilege-filtered regions mistakenly exposed as ordinary children?
- Can firmware reprogram windows after the DTB is built?
- Do tests cover the first and last byte of every window, gaps, and 64-bit ranges?
- Does runtime evidence agree with the calculated final resources?

## Authoritative References

- [Devicetree Specification: `ranges` and address translation](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DeviceTree address translation APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux Device Tree bindings](https://docs.kernel.org/devicetree/bindings/index.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)

## Next Step

Continue with [DMA Address Spaces And `dma-ranges`](dma-address-spaces-and-dma-ranges.md).
