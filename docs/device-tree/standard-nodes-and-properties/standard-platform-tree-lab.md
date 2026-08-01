---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Standard Platform Tree Lab

## Goal

Build and audit a small platform tree containing machine identity, aliases, CPUs, memory, boot handoff, both reservation mechanisms, and cross-cutting provider relationships. Then introduce failures that require reasoning from the final DTB.

The compatible strings and provider arguments are fictional. The exercise teaches encoding mechanics and is not a schema-valid description of real hardware.

## Lab Tree

Create `standard-platform.dts`:

```dts
/dts-v1/;

/memreserve/ 0x000000003e000000 0x0000000000100000;

/ {
        model = "Example Standard Platform Trainer";
        compatible = "example,standard-platform-v2",
                     "example,standard-platform";
        #address-cells = <2>;
        #size-cells = <2>;

        aliases {
                serial0 = &uart0;
        };

        chosen {
                stdout-path = "serial0:115200n8";
                bootargs = "console=ttyS0,115200 rootwait ro";
        };

        cpus {
                #address-cells = <1>;
                #size-cells = <0>;

                cpu0: cpu@0 {
                        device_type = "cpu";
                        compatible = "example,trainer-cpu";
                        reg = <0>;
                };

                cpu1: cpu@1 {
                        device_type = "cpu";
                        compatible = "example,trainer-cpu";
                        reg = <1>;
                        status = "disabled";
                        enable-method = "example,trainer-release";
                };
        };

        memory@0 {
                device_type = "memory";
                reg = <0x0 0x00000000 0x0 0x40000000>;
        };

        reserved-memory {
                #address-cells = <2>;
                #size-cells = <2>;
                ranges;

                dma_pool: buffer@3f000000 {
                        compatible = "shared-dma-pool";
                        reg = <0x0 0x3f000000 0x0 0x01000000>;
                        reusable;
                };
        };

        intc: interrupt-controller {
                compatible = "example,trainer-intc";
                interrupt-controller;
                #address-cells = <0>;
                #interrupt-cells = <2>;
        };

        iommu0: iommu {
                compatible = "example,trainer-iommu";
                #iommu-cells = <1>;
        };

        phy0: phy-provider {
                compatible = "example,trainer-phy";
                #phy-cells = <1>;
        };

        soc {
                compatible = "simple-bus";
                #address-cells = <1>;
                #size-cells = <1>;
                ranges = <0x0 0x0 0x10000000 0x00100000>;

                uart0: serial@1000 {
                        compatible = "example,trainer-uart";
                        reg = <0x1000 0x100>;
                        interrupts-extended = <&intc 17 4>;
                };

                accelerator@2000 {
                        compatible = "example,trainer-accelerator";
                        reg = <0x2000 0x100>;
                        interrupts-extended = <&intc 18 4>;
                        iommus = <&iommu0 0x42>;
                        phys = <&phy0 0>;
                        phy-names = "link";
                        memory-region = <&dma_pool>;
                        dma-coherent;
                };
        };
};
```

## Step 1: Build A Reproducible Artifact

```sh
dtc -@ -I dts -O dtb -o standard-platform.dtb standard-platform.dts
sha256sum standard-platform.dtb
dtc -I dtb -O dts -o standard-platform.final.dts standard-platform.dtb
```

Record the `dtc --version`, command, warnings, and hash. A DTB may compile while violating a real device binding, so successful compilation is only the first gate.

## Step 2: Audit Root And Alias Data

```sh
fdtget standard-platform.dtb / compatible
fdtget standard-platform.dtb / model
fdtget standard-platform.dtb /aliases serial0
fdtget standard-platform.dtb /chosen stdout-path
```

Answer:

1. Which compatible is most specific?
2. What absolute path is stored in `serial0`?
3. Does the path portion of `stdout-path` resolve?
4. Would changing only `model` affect compatible matching?

## Step 3: Decode CPU And Memory Cells

Complete the table without converting the source values by intuition:

| Property | Governing node | Address cells | Size cells | Meaning |
|---|---|---:|---:|---|
| `/cpus/cpu@0/reg` | | | | |
| `/memory@0/reg` | | | | |
| `/reserved-memory/buffer@3f000000/reg` | | | | |
| `/soc/serial@1000/reg` | | | | |

Expected reasoning: CPU `reg` has one identifier and no size; root children use two-plus-two cells; SoC children use one-plus-one cell before `ranges` translates the address.

Calculate all RAM and reservation intervals using `[base, base + size)`. Confirm that the 16 MiB DMA pool occupies the top of the 1 GiB bank and that the 1 MiB memory-reservation-block entry does not overlap it.

## Step 4: Decode Relationships

For each property below, resolve the phandle, find the provider's `#*-cells`, and interpret the remaining arguments only through that provider's fictional contract:

```sh
fdtget -tx standard-platform.dtb /soc/serial@1000 interrupts-extended
fdtget -tx standard-platform.dtb /soc/accelerator@2000 iommus
fdtget -tx standard-platform.dtb /soc/accelerator@2000 phys
fdtget -tx standard-platform.dtb /soc/accelerator@2000 memory-region
```

Explain separately what `dma-coherent` asserts and what it does **not** guarantee. Then explain why `memory-region` provides information that the top-level `/memreserve/` entry cannot.

## Step 5: Break Ancestor Availability

Add this to `soc`:

```dts
status = "disabled";
```

Leave `uart0` without `status`, rebuild, and answer:

- Is the UART locally available?
- Is it effectively available through its ancestors?
- Why can a valid `stdout-path` still lead to silence?

Restore the source afterward.

## Step 6: Break A Cell Contract

Change the memory property to:

```dts
reg = <0x00000000 0x40000000>;
```

Build and capture diagnostics. Regardless of whether a DTB is emitted, split the cells according to the root two-address/two-size contract and prove why the entry is incomplete. Restore it.

## Step 7: Create A Reservation Overlap

Move the `/memreserve/` entry to `0x3f800000` with size `0x01000000`. Calculate its half-open interval and show its overlap with `dma_pool`. Identify which evidence would reveal the conflict:

- `fdtdump` for the memory reservation block
- the logical `/reserved-memory` tree
- early kernel memory logs
- `/proc/iomem`

Restore the non-overlapping address.

## Step 8: Simulate Firmware Mutation

Copy the DTB and use `fdtput` if available:

```sh
cp standard-platform.dtb standard-platform.firmware.dtb
fdtput -t s standard-platform.firmware.dtb /chosen bootargs \
        "console=ttyS0,115200 rootwait ro debug"
```

Hash and decompile both blobs. Produce a focused diff and state which artifact is analogous to the bootloader handoff. This demonstrates why reviewing the original DTS cannot prove the runtime command line.

## Completion Criteria

The lab is complete when you can provide:

- tool version, commands, warnings, and DTB hashes
- a correct cell-decoding table
- calculated RAM and reservation intervals
- resolved alias and phandle targets
- an explanation of effective availability through ancestors
- evidence for each deliberate failure and its correction
- a diff showing a boot-time `/chosen` mutation

## Authoritative References

- [Devicetree Specification: standard nodes](https://devicetree-specification.readthedocs.io/en/stable/devicenodes.html)
- [Devicetree Specification: cells, phandles, and interrupts](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)

## Continue

Proceed to [Addressing And Bus Modeling](../addressing-and-bus-modeling.md).
