---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binding Schema Authoring And Validation Lab

This lab converts a real binding that Linux v6.19 still documents as plain text, `Documentation/devicetree/bindings/media/ti-am437x-vpfe.txt`, into a DT schema. You will audit the old text against the driver and the shipped DTS, write the schema, prove that it selects and validates the three AM437x boards that use the VPFE, and prove that it rejects specific mistakes. Every expected message below was produced with `dt-schema` 2026.9 against mainline v6.19-rc1.

## Inputs

- `Documentation/devicetree/bindings/media/ti-am437x-vpfe.txt`
- `drivers/media/platform/ti/am437x/am437x-vpfe.c`, function `vpfe_get_pdata()`
- `arch/arm/boot/dts/ti/omap/am437x-l4.dtsi` (two SoC nodes) and the boards `am437x-sk-evm.dts`, `am437x-gp-evm.dts`, `am43x-epos-evm.dts`
- `dt-schema` installed (`pip install dtschema`)

## Task 1: Audit The Text Binding

Read the `.txt` file next to the driver and the DTS files, then list every disagreement. You should find at least these:

1. The text lists `ti,am437x-vpfe-interface` under the VPFE node's “Required properties”, but its own example, every board file, and the driver put it on the **endpoint** (`of_property_read_u32(endpoint, "ti,am437x-vpfe-interface", ...)`).
2. The driver treats the interface as optional: it starts at 0 (raw Bayer) and falls back to raw Bayer for out-of-range values.
3. The driver *does* require a parallel `bus-width` between 8 and 16 (`Invalid bus width.` otherwise), which the text never mentions.
4. The example's unit address `vpfe@f0034000` does not match its `reg = <0x48328000 0x2000>`.
5. The example's sensor uses `compatible = "ti,ov2659"` and `mclk-frequency`; real boards use `ovti,ov2659`.
6. It points to `video-interfaces.txt`, which is now `video-interfaces.yaml`.

Decide which disagreements the schema must resolve and which belong in the commit message.

## Task 2: Record The Baseline

Build the three DTBs (`make ARCH=arm ti/omap/am437x-sk-evm.dtb ...`, or the manual `cpp` + `dtc` pipeline) and validate them before any schema exists:

```sh
dt-validate -m -s processed-schema.json am437x-sk-evm.dtb | grep -i vpfe
```

Expected:

```text
.../target-module@26000/vpfe@0: failed to match any schema with compatible: ['ti,am437x-vpfe']
endpoint: ti,am437x-vpfe-interface: b'\x00\x00\x00\x00' is not of type 'object', 'integer', 'array', 'boolean', 'null'
.../target-module@28000/vpfe@0: failed to match any schema with compatible: ['ti,am437x-vpfe']
```

The second line shows what an untyped vendor property looks like: without a schema, the validator cannot decode its bytes.

## Task 3: Define The Contract

Fill this table from Task 1, the driver, and `video-interfaces.yaml`:

| Property | Node | Required? | Constraint | Evidence |
|---|---|---|---|---|
| `compatible` | VPFE | | | |
| `reg` | VPFE | | | |
| `interrupts` | VPFE | | | |
| `port` | VPFE | | | |
| `ti,am437x-vpfe-interface` | endpoint | | | |
| `bus-width` | endpoint | | | |
| `hsync-active`, `vsync-active` | endpoint | | | |
| `pinctrl-*`, `status` | VPFE | | | |

Note that the SoC nodes in `am437x-l4.dtsi` have no clocks: the functional clock sits on the parent `ti,sysc` target module, so the VPFE schema must not require one.

## Task 4: Write The Schema

Create `Documentation/devicetree/bindings/media/ti,am437x-vpfe.yaml` with `$id`, `$schema`, `title`, `maintainers` (take them from the `TI AM437X VPFE DRIVER` entry in `MAINTAINERS`), `properties`, `required`, closure, and one example. Reference `/schemas/graph.yaml#/$defs/port-base` for the port and `/schemas/media/video-interfaces.yaml#` for the endpoint, and close both with `unevaluatedProperties: false`. Write it before reading the reference solution at the end.

## Task 5: Run The Validation Ladder

```sh
make dt_binding_check DT_SCHEMA_FILES=ti,am437x-vpfe.yaml
make ARCH=arm CHECK_DTBS=y DT_SCHEMA_FILES=ti,am437x-vpfe.yaml \
        ti/omap/am437x-sk-evm.dtb ti/omap/am437x-gp-evm.dtb ti/omap/am43x-epos-evm.dtb
```

Without Kbuild, the same gates are `dt-doc-validate -u <bindings-dir> <file>`, `dt-mk-schema -j -o ps.json <bindings-dir>`, `dt-extract-example` plus `cpp` and `dtc -@`, and `dt-validate -m -s ps.json <dtb>`.

Expected: the schema and example pass; all three DTBs produce no VPFE output at all, including no `failed to match` line. That absence is your selection proof: `-m` would report the node if no schema matched.

## Task 6: Prove Negative Coverage

Apply each mutation to a copy, re-run the matching gate, and confirm the message:

| Mutation | Gate | Expected message (abridged) |
|---|---|---|
| `bus-width = <20>` on the SK endpoint | DTB | `port:endpoint:bus-width: 20 is greater than the maximum of 16` |
| `ti,am437x-vpfe-interface = <7>` | DTB | `port:endpoint:ti,am437x-vpfe-interface: 7 is not one of [0, 1, 2, 3, 4]` |
| interface property moved to the VPFE node | DTB | `'ti,am437x-vpfe-interface' does not match any of the regexes: '^pinctrl-[0-9]+$'` |
| `bus-width` deleted | DTB | `port:endpoint: 'bus-width' is a required property` |
| `$id` names `media/ti,vpfe.yaml` | schema | `$id: Cannot determine base path from $id, relative path/filename doesn't match actual path or filename` |
| interface `$ref` to `uint32` removed | processing | `ti,am437x-vpfe-interface: missing type definition` |

For the DTB mutations, `fdtput` on the SK DTB is enough, for example:

```sh
E=/ocp@44000000/interconnect@48000000/segment@300000/target-module@26000/vpfe@0/port/endpoint
cp am437x-sk-evm.dtb f1.dtb && fdtput -t u f1.dtb $E bus-width 20
```

The third row shows why the `.txt` placement was wrong: a schema that matched the text would reject every shipped board.

## Task 7: Find What The Schema Cannot Catch

Explain each of these, and say which tool or review step would catch it:

1. Both SoC nodes declare `reg = <0x0 0x2000>`, but their parent target modules declare `ranges = <0x0 0x26000 0x1000>` and `<0x0 0x28000 0x1000>`, so the child window is larger than the parent's (see [Window Containment](../addressing-and-bus-modeling/simple-bus-ranges-and-nested-translation.md)). `dtc` and `dt-validate` are both silent.
2. The old example's `vpfe@f0034000` with `reg = <0x48328000 ...>`. Compile it as a standalone DTS and note that `dtc` does not report the mismatch for a node outside a `simple-bus`.
3. Values the driver accepts but the schema now rejects (for example interface 7). Decide whether that stricter contract can break any shipped DTB.

## Task 8: Prepare The Submission

Write the commit for a single patch: subject `dt-bindings: media: ti,am437x-vpfe: Convert to DT schema`, deletion of the `.txt` file, and a message that records every deliberate difference from the text from Task 1 (for example, “Move ti,am437x-vpfe-interface to the endpoint, matching the driver and all in-tree users”). List the validation commands you ran and their results.

## Reference Solution

```yaml
# SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
%YAML 1.2
---
$id: http://devicetree.org/schemas/media/ti,am437x-vpfe.yaml#
$schema: http://devicetree.org/meta-schemas/core.yaml#

title: Texas Instruments AM437x Video Processing Front End (VPFE)

maintainers:
  - Lad Prabhakar <prabhakar.csengg@gmail.com>

description:
  The VPFE captures from raw image sensors and video decoders over a
  parallel bus.

properties:
  compatible:
    const: ti,am437x-vpfe

  reg:
    maxItems: 1

  interrupts:
    maxItems: 1

  port:
    $ref: /schemas/graph.yaml#/$defs/port-base
    description: Parallel input from an image sensor or video decoder.

    properties:
      endpoint:
        $ref: /schemas/media/video-interfaces.yaml#
        unevaluatedProperties: false

        properties:
          ti,am437x-vpfe-interface:
            $ref: /schemas/types.yaml#/definitions/uint32
            enum: [0, 1, 2, 3, 4]
            default: 0
            description: |
              Input interface type:
                0 - Raw Bayer
                1 - 8-bit BT.656
                2 - 10-bit BT.656
                3 - YCbCr 8-bit
                4 - YCbCr 16-bit

          bus-width:
            minimum: 8
            maximum: 16

          hsync-active: true
          vsync-active: true

        required:
          - bus-width

    unevaluatedProperties: false

required:
  - compatible
  - reg
  - interrupts
  - port

additionalProperties: false

examples:
  - |
    #include <dt-bindings/interrupt-controller/arm-gic.h>

    vpfe@0 {
        compatible = "ti,am437x-vpfe";
        reg = <0x0 0x2000>;
        interrupts = <GIC_SPI 48 IRQ_TYPE_LEVEL_HIGH>;

        port {
            endpoint {
                remote-endpoint = <&ov2659_ep>;
                ti,am437x-vpfe-interface = <0>;
                bus-width = <8>;
                hsync-active = <0>;
                vsync-active = <0>;
            };
        };
    };
```

Points to check against your version:

- `ti,am437x-vpfe-interface` lives under the endpoint, with a `uint32` type, an `enum`, and `default: 0`
- `bus-width` is required with `minimum: 8` and `maximum: 16`, matching the driver
- there are no `clocks`; `pinctrl-*` and `status` need no declaration because `dt-schema` admits them
- the example includes `arm-gic.h` and refers to a sensor endpoint it does not define, which works because `dt-extract-example` emits `/plugin/;`

## Completion Criteria

The lab is complete when you can show:

- the defect list from Task 1 with the action for each
- the baseline output and the clean output after the schema
- the contract table
- the schema passing meta-validation and example validation
- all six mutations producing their expected messages
- explanations for the three gaps in Task 7
- a commit message that would survive review

## Authoritative References

- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux annotated example schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/example-schema.yaml)
- [`dt-schema` project](https://github.com/devicetree-org/dt-schema)
- [Linux binding patch submission guide](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)

## Next Step

Continue with [Overlays In Depth](../overlays-in-depth.md), applying the same schema-selection and final-tree validation discipline to composed base DTBs and DTBOs.
