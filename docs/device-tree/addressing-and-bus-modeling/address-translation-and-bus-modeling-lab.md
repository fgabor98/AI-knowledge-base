---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Address Translation And Bus Modeling Lab

## Goal

Decode one platform's CPU, DMA, and interrupt paths across nested buses, then inspect a PCI host window. Deliberate failures test cell grouping, window containment, inheritance, and namespace separation.

The compatible strings and interrupt encodings are fictional. This is a mechanics exercise, not a schema-valid hardware binding.

## Lab Tree

Create `addressing-lab.dts`:

```dts
/dts-v1/;

/ {
        compatible = "example,addressing-lab";
        #address-cells = <2>;
        #size-cells = <2>;

        memory@80000000 {
                device_type = "memory";
                reg = <0x0 0x80000000 0x0 0x20000000>;
        };

        intc: interrupt-controller {
                compatible = "example,lab-intc";
                interrupt-controller;
                #address-cells = <0>;
                #interrupt-cells = <2>;
        };

        soc {
                compatible = "simple-bus";
                #address-cells = <1>;
                #size-cells = <1>;
                interrupt-parent = <&intc>;
                ranges = <0x00000000 0x00000000 0x40000000 0x01000000>,
                         <0x10000000 0x00000008 0x00000000 0x01000000>;
                dma-ranges = <0x00000000 0x00000000 0x80000000 0x10000000>;

                uart@2000 {
                        compatible = "example,lab-uart";
                        reg = <0x2000 0x100>;
                        interrupts = <17 4>;
                };

                bridge@800000 {
                        compatible = "simple-bus";
                        reg = <0x800000 0x10000>;
                        #address-cells = <1>;
                        #size-cells = <1>;
                        ranges = <0x0000 0x800000 0x10000>;

                        engine@100 {
                                compatible = "example,lab-engine";
                                reg = <0x100 0x40>, <0x1000 0x20>;
                                reg-names = "control", "doorbell";
                                interrupts = <18 4>;
                        };
                };

                i2c@3000 {
                        compatible = "example,lab-i2c";
                        reg = <0x3000 0x100>;
                        #address-cells = <1>;
                        #size-cells = <0>;

                        sensor@48 {
                                compatible = "example,lab-sensor";
                                reg = <0x48>;
                        };
                };
        };

        pcie@4010000000 {
                compatible = "pci-host-ecam-generic";
                device_type = "pci";
                #address-cells = <3>;
                #size-cells = <2>;
                bus-range = <0x00 0x0f>;
                reg = <0x00000040 0x10000000 0x0 0x01000000>;
                ranges = <0x01000000 0x0 0x00000000
                          0x00000000 0x3f000000
                          0x00000000 0x00010000>,
                         <0x02000000 0x0 0x40000000
                          0x00000000 0x50000000
                          0x00000000 0x10000000>;
        };
};
```

## Step 1: Build And Preserve Evidence

```sh
dtc -@ -I dts -O dtb -o addressing-lab.dtb addressing-lab.dts
sha256sum addressing-lab.dtb
dtc -I dtb -O dts -o addressing-lab.final.dts addressing-lab.dtb
```

Record `dtc --version`, the exact command, every warning, and the hash. Fictional compatibles mean this lab is not expected to pass real binding-schema validation.

## Step 2: Build A Cell-Contract Table

Fill this from each node's immediate parent:

| Property | Address cells | Size cells | Entry width | Address meaning |
|---|---:|---:|---:|---|
| `/memory@80000000/reg` | | | | |
| `/soc/uart@2000/reg` | | | | |
| `/soc/bridge@800000/engine@100/reg` | | | | |
| `/soc/i2c@3000/sensor@48/reg` | | | | |
| `/pcie@4010000000/reg` | | | | |

The sensor entry has no size and identifies an I2C target. The PCI host's own `reg` is encoded by the root, not by the host's three-cell child contract.

## Step 3: Translate CPU Resources

Calculate half-open CPU physical intervals for:

1. UART registers
2. engine control registers
3. engine doorbell registers
4. I2C controller registers

Show every boundary. Expected bases are:

```text
UART:             0x40002000
engine control:   0x40800100
engine doorbell:  0x40801000
I2C controller:   0x40003000
```

Do **not** translate `sensor@48` through the SoC `ranges`. Its address belongs to the I2C namespace; only its controller's MMIO resource follows the outer translation.

## Step 4: Translate The DMA Aperture

Use `dma-ranges` to answer:

- Which parent memory interval can a SoC device reach directly?
- Which device DMA address corresponds to CPU physical `0x88000000`?
- Can it directly reach CPU physical `0x98000000` through this window?
- Does the MMIO `ranges` base participate in this calculation?

Expected direct DMA aperture: child `[0, 0x10000000)` to parent `[0x80000000, 0x90000000)`. Thus physical `0x88000000` appears as DMA `0x08000000`; `0x98000000` is outside this mapping.

## Step 5: Resolve Interrupt Inheritance

Neither `uart` nor `engine` has a local `interrupt-parent`. Walk ancestors in the final tree and identify the effective parent. Then decode each two-cell entry under the fictional controller contract.

Answer why:

- `18` is a controller-local hardware interrupt, not necessarily Linux IRQ 18
- the engine inherits through `bridge` even though the property is on `soc`
- changing `#interrupt-cells` without rewriting consumers corrupts entry boundaries

## Step 6: Decode PCI Windows

Split each PCI `ranges` entry into:

| Window | PCI space/type cell | PCI child base | CPU parent base | Length |
|---|---|---:|---:|---:|
| I/O | | | | |
| 32-bit memory | | | | |

Expected mappings:

- PCI I/O `[0, 0x10000)` → CPU `[0x3f000000, 0x3f010000)`
- PCI memory `[0x40000000, 0x50000000)` → CPU `[0x50000000, 0x60000000)`

Explain why the first PCI cell is decoded as flags and fields rather than concatenated as an ordinary high address cell. Check that 16 buses fit in the 16 MiB generic ECAM interval.

## Step 7: Break A `reg` Width

Change the engine control region to:

```dts
reg = <0x100>;
```

Build and capture diagnostics. The bridge requires one address plus one size cell, so the stream is incomplete. Restore the source.

## Step 8: Cross A Window Boundary

Change the engine doorbell entry to:

```dts
reg = <0xfff0 0x20>;
```

Its first byte lies inside bridge child `[0, 0x10000)`, but its last byte does not. Calculate the interval and explain why testing only the base gives a false success. Restore the source.

## Step 9: Remove Versus Empty `ranges`

Run two separate experiments on `bridge`:

1. Replace its mapping with `ranges;`.
2. Remove the property entirely.

For each, record compiler warnings and predicted Linux translation behavior. With identity mapping, `engine@100` enters the SoC namespace at `0x100`; with no generic mapping, translation is not defined. Restore the explicit mapping.

## Step 10: Break Interrupt Inheritance

Add a second fictional controller with `#interrupt-cells = <3>` and set only `bridge` to use it. Rebuild without changing the engine's two-cell `interrupts` value.

Explain why the UART remains under the original two-cell contract while the engine becomes malformed. This proves that the same-looking `interrupts` property can change meaning through ancestry.

## Step 11: Compare With Linux Runtime

On real hardware using an analogous tree, collect:

```sh
cat /proc/iomem
cat /proc/interrupts
find /sys/firmware/devicetree/base -name reg -o -name ranges -o -name dma-ranges
lspci -t
lspci -vv
```

Map each runtime resource back to its DT path. Keep raw child addresses, translated CPU resources, DMA addresses, hardware interrupts, and Linux IRQs in separate columns.

## Completion Criteria

The lab is complete when you can provide:

- tool version, commands, warnings, and DTB hash
- a correct cell-contract table
- step-by-step nested CPU translations
- the DMA aperture and inverse address calculation
- the effective interrupt-parent walk
- decoded PCI I/O and memory windows
- interval proofs for each deliberate failure
- an explanation of why the I2C child address is not CPU-translated

## Authoritative References

- [Devicetree Specification: address, DMA, and interrupt translation](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DeviceTree translation APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux DMA API HOWTO](https://docs.kernel.org/core-api/dma-api-howto.html)
- [Linux generic ECAM PCI host binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/pci/host-generic-pci.yaml)

## Continue

Proceed to [Driver Matching](../driver-matching.md).
