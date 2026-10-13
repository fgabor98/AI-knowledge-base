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

The AM64x OSPI controller sits on the `fss` bus, which declares two address and two size cells (`k3-am64-main.dtsi`):

```dts
fss: bus@fc00000 {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;

        ospi0: spi@fc40000 {
                reg = <0x00 0x0fc40000 0x00 0x100>,
                      <0x05 0x00000000 0x01 0x00000000>;
                /* ... */
        };
};
```

Each entry is four cells. The first is a 256-byte register window at `0xfc40000`; in the second, the address cells concatenate to `0x0000000500000000` and the size cells to `0x0000000100000000`—the 4 GiB flash data window. Cells are 32-bit big-endian quantities in the DTB. Multi-cell values are not separate fields unless the binding says they are.

`#address-cells` and `#size-cells` describe the encoding of direct children. They do not describe the bus node's own `reg`, which is encoded by its parent. They are also not inherited through arbitrary levels. The specification defines defaults when they are absent, but production trees should state them explicitly on addressable buses; implicit defaults hide integration mistakes and produce poor diagnostics.

## Zero Size Cells

Some buses identify children but do not encode a range size. The OSPI controller itself declares this for its flash children (`k3-am64-main.dtsi` and `k3-am642-sk.dts`):

```dts
ospi0: spi@fc40000 {
        #address-cells = <0x1>;
        #size-cells = <0x0>;

        flash@0 {
                compatible = "jedec,spi-nor";
                reg = <0x0>;            /* chip select, not address-plus-size */
                /* ... */
        };
};
```

With `#size-cells = <0>`, each entry contains only address cells. Do not append a zero size as a placeholder; that changes the cell stream and can make later entries unparsable.

## Multiple Register Regions

A device may expose several resource windows. The TI-SCI secure proxy mailbox (`k3-am64-main.dtsi`):

```dts
secure_proxy_main: mailbox@4d000000 {
        compatible = "ti,am654-secure-proxy";
        reg-names = "target_data", "rt", "scfg";
        reg = <0x00 0x4d000000 0x00 0x80000>,
              <0x00 0x4a600000 0x00 0x80000>,
              <0x00 0x4a400000 0x00 0x80000>;
};
```

Entries follow the binding's name order, not ascending address. `reg-names` follows the same positional rules as other [resource-name lists](../provider-consumer-relationships/specifier-decoding-and-resource-names.md#resource-names): reordering `reg` without the names changes which physical resource a driver maps.

Treat `[address, address + size)` as a half-open interval. A zero size, wraparound, overflow, overlap, or range outside the containing bus window is a defect even if `dtc` emits a blob.

## Unit Address Consistency

The text after `@` is the node's unit address. For a memory-mapped device it normally equals the first address in `reg`, written in lowercase hexadecimal without leading zeros: `serial@2800000` for `reg = <0x00 0x02800000 0x00 0x100>`, and `mailbox@4d000000` above even though its other two windows are at lower addresses.

The unit address is a node-name discriminator, not a second resource declaration. Software consumes `reg`; it does not derive a size or translated CPU address from the node name. Bus bindings can define non-numeric or multi-field unit-address formats, so use the bus rule rather than applying a generic formatting assumption.

Nodes without `reg` generally have no unit address. Exceptions exist only when a binding defines another property that supplies the unit address.

## Decode Cells Mechanically

For the OSPI raw cell stream printed by `fdtget -tx` from the AM642 SK DTB:

```text
0 fc40000 0 100 5 0 1 0
```

under a two-address/two-size parent:

| Entry | Address cells | Address | Size cells | Size |
|---|---|---:|---|---:|
| 0 | `0 fc40000` | `0xfc40000` | `0 100` | `0x100` |
| 1 | `5 0` | `0x500000000` | `1 0` | `0x100000000` |

Use unsigned arithmetic wide enough for all cells. A convenient reconstruction is:

```text
value = (cell[0] << 32 × (n - 1)) | ... | cell[n - 1]
```

In C, Python, shell utilities, or spreadsheets, verify that intermediate values do not truncate to 32 bits.

## Binding Semantics Come First

The generic shape does not tell you what the address means: it may be an MMIO offset plus length, an I2C target address, an SPI chip select, a CPU identifier, or a three-cell PCI address. [Bus-Specific Child Addressing](bus-specific-child-addressing.md) covers these formats.

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
