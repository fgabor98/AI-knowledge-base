---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Property Types, Encodings, And Cardinality

Schema validation does not operate on DTS tokens exactly as they appear in source. `dtc` and `dt-schema` expose a normalized data shape in which strings are arrays and numeric cell groups are matrices. Correct constraints require understanding both the source encoding and processed shape.

## Reuse Common Property Types

Standard properties such as `reg`, `interrupts`, `clocks`, `resets`, and `status` already have type definitions. A device binding usually adds only device-specific constraints:

```yaml
properties:
  reg:
    maxItems: 1

  interrupts:
    maxItems: 1
```

That is all the VPFE schema needs for its standard resources.

Do not repeat their generic type definitions. Redefinition can conflict with common schemas and hides which semantics are shared.

Vendor-specific properties normally reference a DT type from `/schemas/types.yaml` unless they are boolean:

```yaml
ti,am437x-vpfe-interface:
  $ref: /schemas/types.yaml#/definitions/uint32
  enum: [0, 1, 2, 3, 4]
  default: 0
```

Without the `$ref`, `dt-mk-schema` reports `ti,am437x-vpfe-interface: missing type definition`, and DTB validation cannot decode the value: today, with no VPFE schema at all, every AM437x DTB gets `ti,am437x-vpfe-interface: b'\x00\x00\x00\x00' is not of type 'object', 'integer', 'array', 'boolean', 'null'`.

## DTS And Validation Shapes

Conceptually:

```dts
clock-names = "core";
interrupts = <GIC_SPI 86 IRQ_TYPE_LEVEL_HIGH>;
clocks = <&k3_clks 187 0>, <&k3_clks 187 1>;
```

is normalized into array-oriented data roughly like:

```text
clock-names -> ["core"]
interrupts  -> [[0, 86, 4]]
clocks      -> [[2, 187, 0], [2, 187, 1]]
```

The last line is what `dt-validate` prints for a PowerVR node with one clock too many: `clocks: [[2, 187, 0], [2, 187, 1]] is too long`. Each inner list is one phandle-plus-specifier entry, with the phandle shown as its number.

DT schema transformations let authors use compact idioms for common single values and fixed tuples, but diagnostics often reveal the processed array/matrix model. When an error says an array is too short or an item is itself an array, trace it back to cell grouping in DTS.

## Boolean Properties

From `usb/ti,am62-usb.yaml`:

```yaml
ti,vbus-divider:
  description:
    Should be present if USB VBUS line is connected to the
    VBUS pin of the SoC via a 1/3 voltage divider.
  type: boolean
```

In DTS, presence means true, as on the AM62x SK:

```dts
&usbss0 {
        ti,vbus-divider;
};
```

There is no `= <0>` false form for a boolean binding. Absence must have a defined meaning. Do not add a boolean to select a software implementation path.

## Strings And String Arrays

One exact string, from `ti,am625-oldi.yaml`:

```yaml
clock-names:
  const: serial
```

A finite vocabulary, from `usb/usb-drd.yaml`:

```yaml
dr_mode:
  $ref: /schemas/types.yaml#/definitions/string
  enum: [host, peripheral, otg]
```

An ordered string list, from `media/cdns,csi2rx.yaml`:

```yaml
clock-names:
  items:
    - const: sys_clk
    - const: p_clk
    - const: pixel_if0_clk
    - const: pixel_if1_clk
    - const: pixel_if2_clk
    - const: pixel_if3_clk
```

The explicit `items` list fixes order and, through DT schema processing, normally fixes cardinality. Use `minItems`/`maxItems` where a range is intentional. Avoid a free-form `type: string` when the hardware vocabulary is closed.

## Scalars And Ranges

From `mmc/sdhci-am654.yaml`, one of the per-mode tap delays:

```yaml
ti,otap-del-sel-legacy:
  description: Output tap delay for SD/MMC legacy timing
  $ref: /schemas/types.yaml#/definitions/uint32
  minimum: 0
  maximum: 0xf
```

Use `enum` for sparse valid values and `minimum`/`maximum` for a continuous range. Add `multipleOf` only when the hardware truly requires a step. State inclusive boundaries and unit semantics in the property description.

For properties with recognized unit suffixes, common schemas may already provide type and unit expectations. Constrain the valid device-specific range without inventing another scale.

## Fixed Tuples

Use positional items when each cell has a different meaning:

From `net/can/bosch,m_can.yaml` (descriptions trimmed):

```yaml
bosch,mram-cfg:
  $ref: /schemas/types.yaml#/definitions/int32-array
  items:
    - description: Message RAM offset
      default: 0
    - description: 11-bit Filter 0-128 elements
      minimum: 0
      maximum: 128
    - description: 29-bit Filter 0-64 elements
      minimum: 0
      maximum: 64
    # Rx FIFO 0, Rx FIFO 1, Rx Buffers (0-64 each)
    # Tx Event FIFO, Tx Buffers (0-32 each)
  minItems: 1
```

The AM64x node sets `bosch,mram-cfg = <0x0 128 64 64 64 64 32 32>;`, every element at its maximum. Each position has its own meaning and range.

Before defining such a tuple, ask whether it should be a phandle-array, `reg`, `dma-ranges`, or another existing structure. An opaque vendor cell tuple is hard to extend.

## Resource Lists And Parallel Names

Keep resource and name cardinality aligned:

```yaml
properties:
  interrupts:
    maxItems: 2

  interrupt-names:
    items:
      - const: error_irq
      - const: irq
```

This is `cdns,csi2rx.yaml` as merged, and its names show what current review guidance discourages: names should describe roles without suffixes such as `-irq` or `_clk`. Once DTBs ship, such names are ABI and cannot be cleaned up. If variants have different list lengths, give broad limits at the top level and narrow both lists in the same compatible-specific condition.

## Phandles And Phandle Arrays

A single reference and a provider specifier are different shapes:

- phandle: reference to one node
- phandle-array: one or more entries, each containing a provider phandle plus provider-defined argument cells

Prefer existing property definitions. For custom relationships, define why a standard provider class does not apply and reference the correct type:

```yaml
ti,companion-oldi:
  $ref: /schemas/types.yaml#/definitions/phandle
  description:
    phandle to companion OLDI transmitter. This property is required for both
    the OLDI TXes if they are expected to work either in dual-lvds mode or in
    clone mode. This property should point to the other OLDI TX's phandle.
```

This is from `ti,am625-oldi.yaml`.

The consumer schema usually constrains entry count; the provider binding defines `#*-cells` and specifier meaning.

## Byte Arrays

Use byte-array definitions for actual byte sequences such as hardware identifiers or calibration blobs, not for numeric values that happen to fit in bytes. Constrain exact or bounded length. From `net/network-class.yaml` (description trimmed):

```yaml
mac-address:
  description:
    Specifies the MAC address that was last used by the boot program.
  $ref: /schemas/types.yaml#/definitions/uint8-array
  minItems: 6
  maxItems: 6
```

The AM64x `cpsw_port1` writes it as `mac-address = [00 00 00 00 00 00];`, the byte-string syntax.

Do not place large opaque configuration blobs in DT merely because a schema can bound their size.

## Compatible Lists

One identity, as in the VPFE schema:

```yaml
compatible:
  const: ti,am437x-vpfe
```

Several independent identities, from `ti,omap-mailbox.yaml` (trimmed):

```yaml
compatible:
  enum:
    - ti,omap2-mailbox # for OMAP2420, OMAP2430 SoCs
    - ti,omap3-mailbox # for OMAP3430, OMAP3630 SoCs
    # ...
    - ti,am64-mailbox
```

Specific fallback sequences, from `img,powervr-rogue.yaml` (trimmed):

```yaml
compatible:
  oneOf:
    - items:
        - enum:
            - ti,am62-gpu
        - const: img,img-axe-1-16m
        - const: img,img-axe
        - const: img,img-rogue
    - items:
        - enum:
            - ti,am62-gpu
        - const: img,img-axe
      deprecated: true
```

This accepts exactly the documented order rather than any permutation of known strings.

## Cardinality Audit

For every array-like property, answer:

- exact, minimum, and maximum number of entries
- semantic order
- whether duplicates are meaningful
- relationship to a parallel `*-names` property
- variant-specific narrowing
- meaning of absence

Unbounded arrays and unconstrained strings are usually incomplete ABI definitions.

## Authoritative References

- [Linux schema-writing guide: property schemas](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [`dt-schema` type definitions](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/types.yaml)
- [`dt-schema` property unit schemas](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/property-units.yaml)

## Continue

Proceed to [References, Composition, Conditionals, And Closure](references-composition-conditionals-and-closure.md).
