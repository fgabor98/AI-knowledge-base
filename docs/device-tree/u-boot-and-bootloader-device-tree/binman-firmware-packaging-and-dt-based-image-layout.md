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

TI's AM64x boot chain is three binman images, described in `arch/arm/dts/k3-am64x-binman.dtsi` and the shared `k3-binman.dtsi`:

| Image | Built by | Contents | First consumer |
|---|---|---|---|
| `tiboot3.bin` | R5 build | R5 SPL, TIFS system firmware, its board configuration, wrapped in an X.509 certificate | boot ROM |
| `tispl.bin` | A53 build | FIT: TF-A, OP-TEE, A53 SPL, one SPL DTB per board | R5 SPL |
| `u-boot.img` | A53 build | FIT: U-Boot proper, one DTB per board | A53 SPL |

The first, for HS silicon (trimmed):

```dts
&binman {
        tiboot3-am64x_sr2-hs-evm.bin {
                filename = "tiboot3-am64x_sr2-hs-evm.bin";
                ti-secure-rom {
                        content = <&u_boot_spl>, <&ti_sci_enc>,
                                <&combined_sysfw_cfg>, <&sysfw_inner_cert>;
                        combined;
                        keyfile = "custMpk.pem";
                        load = <0x70000000>;
                        load-sysfw = <0x44000>;
                        load-sysfw-data = <0x7b000>;
                };
                u_boot_spl: u-boot-spl {
                        no-expanded;
                };
                ti_sci_enc: ti-fs-enc.bin {
                        filename = "ti-sysfw/ti-sci-firmware-am64x_sr2-hs-enc.bin";
                        type = "blob-ext";
                };
                /* combined-sysfw-cfg.bin, sysfw inner certificate */
        };
};
```

`ti-secure-rom` is a TI-specific entry type that builds the certificate the ROM checks; `load` places the R5 SPL in on-chip SRAM at `0x70000000`. The `tispl.bin` FIT, from the shared template, gives each payload its load address from Kconfig (TF-A at `CONFIG_K3_ATF_LOAD_ADDR`, OP-TEE at `CONFIG_K3_OPTEE_LOAD_ADDR`). It also has a `dm` image whose `blob-ext` is `/dev/null` on AM64x, because this SoC has no separate device-manager firmware.

Keep layout policy in a reusable SoC U-Boot fragment when it is common, then narrow board/product differences deliberately. Do not hide product offsets in an unrelated hardware node.

## Account For Every Byte

For each final image and the medium it lives on, generate an interval map. For the AM642 EVM's OSPI flash, the Linux DTS partitions and TI U-Boot's defconfigs agree on:

| Offset range | Entry | Consumer |
|---|---|---|
| `0x000000`–`0x0fffff` | `tiboot3.bin` | boot ROM |
| `0x100000`–`0x2fffff` | `tispl.bin` (`CONFIG_SYS_SPI_U_BOOT_OFFS=0x100000` in the R5 defconfig) | R5 SPL |
| `0x300000`–`0x6fffff` | `u-boot.img` (`CONFIG_SYS_SPI_U_BOOT_OFFS=0x300000` in the A53 defconfig) | A53 SPL |
| `0x700000`–`0x77ffff` | environment and its backup | U-Boot |

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
