---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Cell Counts, `reg`, And Unit Addresses

`reg` is an array of bus-defined address and size entries. Its shape comes from the node's immediate parent, while the meaning of its address portion comes from that parent's bus binding.

## The Immediate-Parent Rule

For an ordinary addressable child, each `reg` entry contains:

```text
parent #address-cells cells + parent #size-cells cells
```

```dts
/ {
        #address-cells = <2>;
        #size-cells = <2>;

        device@120000000 {
                reg = <0x00000001 0x20000000
                       0x00000000 0x00001000>;
        };
};
```

The address cells concatenate to `0x0000000120000000`; the size cells concatenate to `0x1000`. Cells are 32-bit big-endian quantities in the DTB. Multi-cell values are not separate fields unless the binding says they are.

`#address-cells` and `#size-cells` describe the encoding of direct children. They do not describe the bus node's own `reg`, which is encoded by its parent. They are also not inherited through arbitrary levels. The specification defines defaults when they are absent, but production trees should state them explicitly on addressable buses; implicit defaults hide integration mistakes and produce poor diagnostics.

## Zero Size Cells

Some buses identify children but do not encode a range size:

```dts
spi@1000 {
        #address-cells = <1>;
        #size-cells = <0>;

        flash@0 {
                reg = <0>; /* chip select, not address-plus-size */
        };
};
```

With `#size-cells = <0>`, each entry contains only address cells. Do not append a zero size as a placeholder; that changes the cell stream and can make later entries unparsable.

## Multiple Register Regions

A device may expose several resource windows:

```dts
mailbox@2000 {
        reg = <0x2000 0x100>,
              <0x3000 0x20>;
        reg-names = "control", "doorbell";
};
```

Here the parent has one address and one size cell. `reg-names` labels entries positionally. The consumer binding defines the accepted names, their order, and whether they are required. Linux drivers commonly request these resources by index or name; reordering `reg` without the names—or names without `reg`—changes which physical resource a driver maps.

Treat `[address, address + size)` as a half-open interval. A zero size, wraparound, overflow, overlap, or range outside the containing bus window is a defect even if `dtc` emits a blob.

## Unit Address Consistency

The text after `@` is the node's unit address. For a simple memory-mapped device it normally equals the first address in `reg`, written without leading zeros:

```dts
uart@2000 {
        reg = <0x2000 0x100>;
};
```

The unit address is a node-name discriminator, not a second resource declaration. Software consumes `reg`; it does not derive a size or translated CPU address from the node name. Bus bindings can define non-numeric or multi-field unit-address formats, so use the bus rule rather than applying a generic formatting assumption.

Nodes without `reg` generally have no unit address. Exceptions exist only when a binding defines another property that supplies the unit address.

## Decode Cells Mechanically

For a raw cell stream:

```text
00000001 20000000 00000000 00001000
00000002 00000000 00000000 00002000
```

under a two-address/two-size parent:

| Entry | Address cells | Address | Size cells | Size |
|---|---|---:|---|---:|
| 0 | `1 20000000` | `0x120000000` | `0 1000` | `0x1000` |
| 1 | `2 00000000` | `0x200000000` | `0 2000` | `0x2000` |

Use unsigned arithmetic wide enough for all cells. A convenient reconstruction is:

```text
value = (cell[0] << 32 × (n - 1)) | ... | cell[n - 1]
```

In C, Python, shell utilities, or spreadsheets, verify that intermediate values do not truncate to 32 bits.

## Binding Semantics Come First

The generic shape does not tell you what the address means:

- MMIO bus: offset or bus address plus length
- I2C: target address with no size
- SPI: chip-select index with no size
- CPU container: hardware CPU identifier with no size
- PCI: flags, bus/device/function, and register fields spread over three cells

The same property name therefore cannot be decoded correctly without the parent bus binding.

## Linux Resource View

Linux OF helpers translate a `reg` entry to a resource after walking bus mappings. A driver using `platform_get_resource()` or `devm_platform_ioremap_resource()` normally sees the translated resource, not the raw child-bus number.

Useful evidence includes:

```sh
cat /proc/iomem
readlink /sys/bus/platform/devices/*/of_node
hexdump -Cv /sys/firmware/devicetree/base/path/to/device/reg
```

Raw properties are big-endian binary data. Prefer `fdtget`, `fdtdump`, or a small endian-aware decoder over visually reading `hexdump` output.

## Review Checklist

- Which immediate parent supplies both cell counts?
- Does the bus binding redefine the address fields?
- Is the cell count an exact multiple of the entry width?
- Does every `reg-names` entry correspond positionally?
- Does the unit address match the binding-defined first address?
- Do reconstructed values fit the platform's address type?
- Are intervals non-overlapping and contained by the reachable bus window?

## Authoritative References

- [Devicetree Specification: `#address-cells`, `#size-cells`, and `reg`](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux DeviceTree address and resource APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Linux platform-device driver model](https://docs.kernel.org/driver-api/driver-model/platform.html)

## Next Step

Continue with [`simple-bus`, `ranges`, And Nested Translation](simple-bus-ranges-and-nested-translation.md).
