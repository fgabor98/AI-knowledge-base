---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# CPUs, Topology, And Memory

`/cpus` describes processing elements; `/memory` describes physical RAM ranges. Both use `reg`, but their parent cell-count contracts and identifiers mean very different things.

## The `/cpus` Container

The `/cpus` node defines how CPU child unit addresses are encoded. The dual-core AM642 (`k3-am642.dtsi`, cache properties trimmed):

```dts
cpus {
        #address-cells = <1>;
        #size-cells = <0>;

        cpu0: cpu@0 {
                compatible = "arm,cortex-a53";
                reg = <0x000>;
                device_type = "cpu";
                enable-method = "psci";
                next-level-cache = <&L2_0>;
        };

        cpu1: cpu@1 {
                compatible = "arm,cortex-a53";
                reg = <0x001>;
                device_type = "cpu";
                enable-method = "psci";
                next-level-cache = <&L2_0>;
        };
};
```

`#size-cells = <0>` means CPU `reg` values carry identifiers but no size field. The identifier is architecture-defined: for Arm it is related to the hardware affinity value, while other architectures define their own meaning. It is not necessarily the Linux logical CPU number shown in `/proc/cpuinfo`.

Each CPU binding can add cache, clock, capacity, power, idle-state, and bring-up properties; AM62x CPUs, for example, also carry `operating-points-v2`, `clocks = <&k3_clks 135 0>`, and `#cooling-cells`. `enable-method` describes how secondary CPUs are started; it must agree with the architecture and firmware interface. On AM64x the `psci` method relies on the `arm,psci-1.0` node under `/firmware` in `k3-am64.dtsi`, implemented by Trusted Firmware-A. Copying `psci` into a tree does not create a working PSCI implementation.

## CPU Topology

Hardware ID and scheduler topology are separate. Where the architecture binding uses a `cpu-map`, it explicitly groups phandles into sockets, clusters, cores, and threads. AM642 places both cores in one cluster sharing `L2_0`:

```dts
cpu-map {
        cluster0: cluster0 {
                core0 {
                        cpu = <&cpu0>;
                };

                core1 {
                        cpu = <&cpu1>;
                };
        };
};
```

Do not infer topology from node ordering or consecutive `reg` values. A topology description must reflect sharing and packaging relevant to scheduling; a plausible-looking but false map can harm placement and energy decisions.

Review topology together with:

- CPU identifiers and enabled population
- cache hierarchy and sharing
- capacity/frequency differences on heterogeneous systems
- firmware tables or architecture discovery mechanisms
- runtime kernel topology under `/sys/devices/system/cpu/`

A CPU disabled by firmware, fused off, or absent from the final DTB may make a source-level map stale. Validate the delivered system.

## The `/memory` Node

A memory node uses the root node's `#address-cells` and `#size-cells`. The AM642 SK has one 2 GiB bank (`k3-am642-sk.dts`):

```dts
memory@80000000 {
        device_type = "memory";
        /* 2G RAM */
        reg = <0x00000000 0x80000000 0x00000000 0x80000000>;
};
```

Larger K3 parts need a second bank above 4 GiB. The J784S4 EVM (`k3-j784s4-evm.dts`):

```dts
memory@80000000 {
        /* 32G RAM */
        reg = <0x00000000 0x80000000 0x00000000 0x80000000>,
              <0x00000008 0x80000000 0x00000007 0x80000000>;
        device_type = "memory";
};
```

Each entry is two address cells followed by two size cells. The second entry is 30 GiB (`0x7_8000_0000`) at `0x8_8000_0000`. One memory node may contain several ranges, or a platform may use multiple memory nodes.

`device_type = "memory"` is required. The unit address is the first range's base address. The `reg` ranges should describe physical memory presented to the operating system; reserved regions are then excluded separately.

## Installed, Described, Reserved, And Usable RAM

These values are not synonyms:

```text
installed physical RAM
        ↓ firmware detects/trains memory
memory described to the OS
        ↓ reservations and unavailable ranges
early kernel usable memory
        ↓ kernel, initrd, CMA, crashkernel, drivers
runtime allocatable memory
```

A board DTS may contain a maximum or default RAM layout while the bootloader patches `/memory` for the fitted module. TI's AM64x U-Boot SPL does this in `spl_perform_board_fixups()` (`board/ti/am64x/evm.c`): with the K3 DDRSS driver and inline ECC enabled, `k3_ddrss_ddr_fdt_fixup()` shrinks the top bank by the ECC storage and rewrites `/memory` with `fdt_fixup_memory_banks()`; without the DDRSS driver, `fixup_memory_node()` writes the banks reported by `dram_init_banksize()`. That is legitimate only when ownership is documented and the final values are validated. Never diagnose a memory-size problem solely from the checked-in DTS.

Compare at least:

```sh
dmesg | grep -i -E 'memory|reserved'
cat /proc/iomem
ls /sys/firmware/devicetree/base
```

For exact cell decoding, use `fdtget` on the deployed DTB or decompile it. Remember that `/proc/meminfo` reports the kernel's managed view after overhead and reservations, not the raw `/memory/reg` total.

## Common Failure Modes

### Wrong cell width

A 64-bit address written as two cells is wrong if the parent declares one address cell, and a one-cell address is incomplete below a two-cell parent. Count according to the immediate parent, not habit.

### Truncation at 4 GiB

Addresses above 32 bits require sufficient address cells. Check every arithmetic conversion, bootloader fixup, and diagnostic tool for 64-bit handling.

### Overlap

Memory banks must not overlap each other. Reserved regions must lie in valid address space and agree with firmware ownership. An apparently harmless overlap can corrupt data long after boot.

### Confusing topology and numbering

CPU node order, hardware CPU identifier, Linux logical CPU number, and topology position are independent concepts. Record which identifier a log or register uses before comparing it with `reg`.

## Senior Review Checklist

- Are root and `/cpus` cell-count contracts explicit?
- Do CPU unit addresses match complete hardware identifiers?
- Is secondary CPU bring-up backed by real firmware support?
- Does topology describe physical sharing rather than cosmetic grouping?
- Can firmware patch CPU or memory nodes, and is that interface versioned?
- Are all RAM ranges representable, non-overlapping, and reconciled with reservations?
- Is runtime validation part of manufacturing coverage for RAM variants and fused CPU variants?

## Authoritative References

- [Devicetree Specification: `/cpus` and `/memory`](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html)
- [Devicetree Specification: `reg` and cell counts](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux arm64 booting requirements](https://docs.kernel.org/arch/arm64/booting.html)
- [Linux CPU topology through sysfs](https://docs.kernel.org/admin-guide/cputopology.html)

## Next Step

Continue with [Chosen And Boot Handoff](chosen-and-boot-handoff.md).
