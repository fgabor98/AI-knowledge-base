---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# `fdtdump` And `fdtget` Binary Inspection

`fdtdump` exposes whole-blob structure and low-level regions; `fdtget` answers focused node/property questions with an explicit output type. Use both on the exact artifact hash under investigation.

## Start With Identity

```bash
sha256sum board.dtb
stat board.dtb
```

Record origin:

```text
built output, extracted FIT component, boot-partition copy,
bootloader memory capture, or pre-handoff checkpoint
```

Two files named `board.dtb` are not the same evidence.

## `fdtdump`: Whole-Blob View

```bash
fdtdump board.dtb
```

Use it to inspect:

- header fields and layout clues
- memory reservation block
- flattened nodes and properties
- numeric phandles
- `__symbols__`, `__fixups__`, and overlay metadata
- unexpected duplicate-looking or stale content

`fdtdump` is diagnostic output, not maintainable DTS. Its formatting and low-level detail can differ from `dtc -I dtb -O dts` output.

Capture rather than truncate when investigating:

```bash
fdtdump board.dtb > board.fdtdump.txt
rg -n 'memreserve|reserved-memory|__symbols__|ethernet@' board.fdtdump.txt
```

Redirection here creates a diagnostic artifact; keep it outside source-controlled documentation unless intentionally recorded.

## `fdtget`: Query One Value

String:

```bash
fdtget board.dtb / model
fdtget board.dtb /chosen stdout-path
```

List child nodes or properties:

```bash
fdtget -l board.dtb /soc
fdtget -p board.dtb /soc/serial@1000
```

Cells as hexadecimal or unsigned integers:

```bash
fdtget -tx board.dtb /soc/serial@1000 reg
fdtget -tu board.dtb /clock@0 clock-frequency
```

Exact option/type syntax can vary across installed dtc utility versions. Consult `fdtget --help` from the same tool package used in the workflow.

## Choose Type From The Binding

The DTB stores bytes, not an intrinsic schema type for every property. `fdtget` needs the interpretation you intend.

| Binding value | Useful interpretation |
|---|---|
| string/string list | string output; account for NUL-separated entries |
| u32 cells | unsigned or hexadecimal 32-bit values |
| u64 encoded as cells | retrieve cells, then combine using binding/parent widths |
| phandle array | hexadecimal cells, then resolve first cell of each entry |
| byte array | byte-width hexadecimal output where supported |
| boolean | test property presence, not numeric value |

Never select decimal or hex based only on readability. Decode according to the binding and provider `#*-cells`.

## Decode `reg`

```bash
fdtget -tx board.dtb /soc/device@48000000 reg
fdtget -tu board.dtb /soc '#address-cells'
fdtget -tu board.dtb /soc '#size-cells'
```

Partition the `reg` cells using the parent bus widths. Then translate through `ranges` as necessary. `fdtget` prints cells; it does not infer the entire bus translation for you.

Example:

```text
parent #address-cells = 2
parent #size-cells = 2
reg cells = 0 48000000 0 1000
address = 0x0000000048000000
size    = 0x0000000000001000
```

## Decode Phandle Arrays

For:

```dts
clocks = <&ccu 12>, <&osc>;
```

Retrieve raw cells:

```bash
fdtget -tx board.dtb /soc/device@48000000 clocks
```

Then:

1. read the first phandle
2. locate its provider node
3. read provider `#clock-cells`
4. consume that many argument cells
5. repeat at the next phandle
6. pair entries with `clock-names`

Do not assume every tuple has the same width when providers differ.

## Resolve A Numeric Phandle

`fdtget` can enumerate nodes/properties but may not provide one universal “find node by phandle” operation across versions. Strategies:

- decode the DTB and search `phandle = <0x...>`
- dump and search the numeric value carefully
- use a short `libfdt` program with `fdt_node_offset_by_phandle()`
- use known `__symbols__` only when symbol metadata exists and the label is relevant

Numeric phandles are artifact-local. Never compare them across independently compiled trees as stable identities.

## Strings Are NUL-Terminated

A string list is stored as adjacent NUL-terminated strings. `fdtget` string output is safer than `cat` or line-oriented text tools. If inspecting filesystem-exported live properties later, use NUL-aware tools.

Do not use `strings` as proof of property boundaries or list order; it discards structure.

## Reservation Inspection

The FDT memory reservation block is distinct from `/reserved-memory`. `fdtdump` can show both. Account for:

- header reservation-map entries
- `/reserved-memory` child nodes
- ordinary `/memory` banks
- bootloader/firmware mutations

Use half-open intervals `[base, base + size)` and check every overlap.

## Error Discipline

Check command exit status and stderr. An empty property, missing node, missing property, type mismatch, and malformed DTB are different outcomes.

Example shell pattern:

```bash
if ! model=$(fdtget board.dtb / model); then
        printf '%s\n' 'failed to read /model' >&2
        exit 1
fi
printf '%s\n' "$model"
```

Do not add a default value in automation unless absence is an approved state. Defaults can hide wrong artifacts.

## Focused Inspection Record

```text
artifact hash:
node path:
property:
binding/provider used to decode:
raw cells/bytes:
decoded value:
expected value and source:
tool version/status:
```

## Authoritative References

- [Devicetree compiler utilities and source](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)

## Continue

Proceed to [`fdtput`, `fdtoverlay`, And Controlled Artifact Mutation](fdtput-fdtoverlay-and-controlled-artifact-mutation.md).
