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

An empty `ranges` says the child and parent address spaces are identical for the relevant domain; the AM64x `fss` and `dmss` buses use it. An absent property does **not** generically mean identity. The AM64x ICSSG configuration block contains a `clocks` container with no `ranges` (`k3-am64-main.dtsi`):

```dts
icssg0_cfg: cfg@26000 {
        compatible = "ti,pruss-cfg", "syscon";
        reg = <0x26000 0x200>;
        ranges = <0x0 0x26000 0x2000>;

        clocks {
                #address-cells = <1>;
                #size-cells = <0>;

                icssg0_coreclk_mux: coreclk-mux@3c {
                        reg = <0x3c>;
                        #clock-cells = <0>;
                        /* ... */
                };
        };
};
```

`reg = <0x3c>` is a register offset inside the `syscon` that the PRUSS clock-mux driver reads through a regmap; it is deliberately not translatable to a CPU address. Some bus bindings or operating-system conventions define special behavior, but relying on an undocumented exception makes the tree non-portable and difficult to review.

## One Translation Boundary

The Raspberry Pi 4 peripheral bus keeps the legacy BCM283x “bus” addresses in child nodes and translates them to the BCM2711's CPU addresses (`arch/arm/boot/dts/broadcom/bcm2711.dtsi` and `bcm283x.dtsi`, trimmed):

```dts
/ {
        #address-cells = <2>;
        #size-cells = <1>;

        soc {
                compatible = "simple-bus";
                #address-cells = <1>;
                #size-cells = <1>;
                ranges = <0x7e000000  0x0 0xfe000000  0x01800000>,
                         <0x7c000000  0x0 0xfc000000  0x02000000>,
                         <0x40000000  0x0 0xff800000  0x00800000>;

                uart0: serial@7e201000 {
                        compatible = "arm,pl011", "arm,primecell";
                        reg = <0x7e201000 0x200>;
                };
        };
};
```

Each `ranges` entry has one child-address cell, two parent-address cells, and one length cell. The first maps child `[0x7e000000, 0x7f800000)` to parent `[0xfe000000, 0xff800000)`. The UART child address `0x7e201000` translates to CPU physical `0xfe201000`; its size remains `0x200`.

Translation formula inside one window:

```text
parent = parent_base + (child - child_base)
```

The entire child resource must fit the selected window. Checking only its first byte misses resources that cross a boundary.

## Nested Translation

AM64x PRU cores sit inside the ICSSG subsystem, which has its own local address space (`k3-am64.dtsi` and `k3-am64-main.dtsi`, trimmed):

```dts
cbass_main: bus@f4000 {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges = /* ... */
                 <0x00 0x30000000 0x00 0x30000000 0x00 0x000bc100>, /* ICSSG0/1 */
                 /* ... */;

        icssg0: icssg@30000000 {
                compatible = "ti,am642-icssg";
                reg = <0x00 0x30000000 0x00 0x80000>;
                #address-cells = <1>;
                #size-cells = <1>;
                ranges = <0x0 0x00 0x30000000 0x80000>;

                pru0_0: pru@34000 {
                        compatible = "ti,am642-pru";
                        reg = <0x34000 0x3000>,
                              <0x22000 0x100>,
                              <0x22400 0x100>;
                        reg-names = "iram", "control", "debug";
                };
        };
};
```

Walk outward one boundary at a time:

1. ICSSG-local `0x34000` falls in the window `[0x0, 0x80000)` and maps to bus address `0x30034000` (one child cell becomes two parent cells).
2. Bus address `0x30034000` falls in the `ICSSG0/1` window of `bus@f4000`, an identity mapping, so the CPU address is `0x30034000`.
3. The PRU instruction RAM interval is `[0x30034000, 0x30037000)`.

Never add every visible base address blindly. At each boundary, first identify the matching child window and subtract that window's child base. Non-zero child bases and disjoint windows make “sum the bases” reasoning fail.

## Multiple And Disjoint Windows

A bus can map several regions. `bus@f4000` on AM64x lists about thirty separate identity windows—pin control, GPIO, peripheral windows, ICSSG, DMSS, PCIe data, and so on—rather than one large range. Anything outside them has no defined parent mapping.

That is not only theoretical. In the v6.19-rc1 sources, `main_gpio_intr` declares `reg = <0x00 0x00a00000 0x00 0x800>`, which none of the `bus@f4000` windows covers (the nearest are GPIO at `0x600000` and the time-sync router at `0xa40000`). Linux's `of_address_count()` stops at the first untranslatable entry, so the platform device is created with no memory resources. It works only because the `ti,sci-intr` driver configures the router through TI-SCI firmware and never maps its registers.

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
