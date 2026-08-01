---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binman, Firmware Packaging, And DT-Based Image Layout

Binman uses a Devicetree-shaped description to assemble firmware components into final media images. It solves placement, alignment, padding, hierarchy, and format integration around U-Boot, SPL/TPL, trusted firmware, DTBs, and vendor blobs. It is not the Linux hardware description and is not interchangeable with FIT.

## FIT Versus Binman

| Concern | FIT | Binman |
|---|---|---|
| primary purpose | runtime payload bundle and configurations | build-time firmware/media layout |
| typical consumer | U-Boot image loader | build tooling, ROM layout, sometimes runtime FDT map |
| load/entry metadata | yes | component/image placement |
| hashes/signatures | FIT-native support | entry processing and integration, often wraps FIT |
| flash offsets/alignment | not its main role | core capability |
| kernel + DTB pairing | configurations | can package the FIT that contains them |

A common design uses binman to place an SPL and a signed FIT into a boot-media image. The FIT then selects and authenticates runtime payloads.

## Image Description

A simplified description might be:

```dts
binman {
        image {
                pad-byte = <0xff>;

                spl {
                        type = "u-boot-spl";
                };

                fit {
                        description = "Signed firmware FIT";
                };
        };
};
```

Real entry syntax is defined by current binman documentation and platform includes. Entry types can expand into nested sections and add headers, padding, hashes, compression, or vendor formats.

Keep layout policy in a reusable SoC U-Boot fragment when it is common, then narrow board/product differences deliberately. Do not hide product offsets in an unrelated hardware node.

## Account For Every Byte

For each final image, generate an interval map:

| Offset range | Entry | Alignment | Consumer |
|---|---|---|---|
| ROM header | platform wrapper | SoC requirement | boot ROM |
| SPL/TPL | early loader | load/execute constraint | ROM/CPU |
| padding | erased/pad value | next component | none |
| FIT | U-Boot + firmware payloads | storage/read constraint | SPL/U-Boot |
| environment | mutable region | erase block | U-Boot |

Check:

- entry offset and size
- parent-relative versus image-absolute offset
- padding byte and erased-media semantics
- alignment to ROM, DMA, and flash erase boundaries
- maximum growth
- redundancy and update slots
- authenticated extent
- writable versus read-only separation

Binman can make a layout internally consistent while still violating a ROM or update-system requirement. Validate external constraints.

## DTB Entries

Binman knows entry types for U-Boot, SPL, and TPL DTBs and for binaries without appended DTBs. Keeping binary and DTB separate can allow tooling to update layout data. Expanded entries can include BSS padding where appending data directly would otherwise collide with runtime zeroing.

Do not infer runtime consumption solely from an entry name. Trace:

```text
source entry
  -> expanded entry hierarchy
  -> output offset/size
  -> ROM or phase loader behavior
  -> runtime address
```

The binman map is evidence for media layout, not automatically for RAM layout.

## FDT Map And Symbols

Binman can place an FDT map describing image entries, allowing tools or firmware to locate contents. It can also fill symbols in executable images with entry positions.

These mechanisms create an ABI between packaging and firmware. Protect it with:

- stable entry paths/names where referenced
- bounds checks before use
- versioning when structure evolves
- authentication of the containing image
- tests that inspect values in the final packaged output

A correct linker symbol before binman runs can be a placeholder; inspect the post-processed binary.

## External And Proprietary Blobs

SoC boot chains can require trusted firmware, DDR training code, system-controller firmware, or vendor headers. Record:

- source and license
- exact hash/version
- expected load/entry address
- whether binman permits a missing placeholder
- who signs or wraps it
- security and rollback policy

Development builds that tolerate missing blobs must not produce an apparently successful production artifact. Gate final packaging on presence and hash policy.

## Multiple Board Images

Binman can build alternative images or include sets of DTBs. Decide whether distribution is:

- one image per board
- one universal media image with runtime selection
- one common early image plus board-specific OS bundle

Universal packaging reduces artifact count but increases selector complexity and image size. Separate images simplify selection but increase release-matrix and factory-programming risk.

Whichever model you choose, manifest every supported board identity to exact output hash.

## Inspection And Repacking

Useful host operations include:

```sh
binman ls -i image.bin
binman extract -i image.bin
binman replace -i image.bin -f replacement.bin path/to/entry
binman sign -i image.bin ...
```

Exact subcommands and options vary; use the current documentation. Repacking or replacement can change offsets, hashes, signatures, and outer vendor headers. Never treat a successful tool exit as sufficient production validation.

Archive:

- binman description closure
- entry arguments
- input hashes
- map file
- final image hash
- extracted-entry verification
- signing and vendor-tool logs

## Update And Failure Atomicity

The physical image layout must support the update mechanism:

- erase-block alignment
- A/B slots or recovery copy
- immutable first-stage or key material
- power-loss-safe metadata
- rollback selection
- environment isolation

DT-based layout does not itself provide atomic updates. Coordinate binman, storage geometry, ROM search rules, bootcount policy, and verified boot.

## Authoritative References

- [U-Boot package documentation](https://docs.u-boot.org/en/latest/develop/package/index.html)
- [U-Boot binman introduction](https://docs.u-boot.org/en/latest/develop/package/binman.html)
- [U-Boot binman entry documentation](https://docs.u-boot.org/en/latest/develop/package/entries.html)
- [U-Boot binman commands and arguments](https://docs.u-boot.org/en/latest/develop/package/binman.html#binman-commands-and-arguments)
- [U-Boot FIT documentation](https://docs.u-boot.org/en/latest/usage/fit/index.html)

## Continue

Proceed to [Bootloader DT Selection And Handoff Lab](bootloader-dt-selection-and-handoff-lab.md).
