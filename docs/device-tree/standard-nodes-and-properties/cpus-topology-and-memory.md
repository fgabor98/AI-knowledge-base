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

The `/cpus` node defines how CPU child unit addresses are encoded:

```dts
cpus {
        #address-cells = <1>;
        #size-cells = <0>;

        cpu0: cpu@0 {
                device_type = "cpu";
                compatible = "arm,cortex-a53";
                reg = <0>;
                enable-method = "psci";
        };

        cpu1: cpu@1 {
                device_type = "cpu";
                compatible = "arm,cortex-a53";
                reg = <1>;
                enable-method = "psci";
        };
};
```

`#size-cells = <0>` means CPU `reg` values carry identifiers but no size field. The identifier is architecture-defined: for Arm it is related to the hardware affinity value, while other architectures define their own meaning. It is not necessarily the Linux logical CPU number shown in `/proc/cpuinfo`.

Each CPU binding can add cache, clock, capacity, power, idle-state, and bring-up properties. `enable-method` describes how secondary CPUs are started; it must agree with the architecture and firmware interface. Copying `psci` into a tree does not create a working PSCI implementation.

## CPU Topology

Hardware ID and scheduler topology are separate. Where the architecture binding uses a `cpu-map`, it explicitly groups phandles into sockets, clusters, cores, and threads:

```dts
cpu-map {
        cluster0 {
                core0 { cpu = <&cpu0>; };
                core1 { cpu = <&cpu1>; };
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

A memory node uses the root node's `#address-cells` and `#size-cells`:

```dts
/ {
        #address-cells = <2>;
        #size-cells = <2>;

        memory@80000000 {
                device_type = "memory";
                reg = <0x0 0x80000000 0x0 0x40000000>,
                      <0x8 0x00000000 0x0 0x80000000>;
        };
};
```

Each entry is two address cells followed by two size cells. The example describes 1 GiB at `0x80000000` and 2 GiB at `0x800000000`. One memory node may contain several ranges, or a platform may use multiple memory nodes.

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

A board DTS may contain a maximum or default RAM layout while the bootloader patches `/memory` for the fitted module. That is legitimate only when ownership is documented and the final values are validated. Never diagnose a memory-size problem solely from the checked-in DTS.

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
