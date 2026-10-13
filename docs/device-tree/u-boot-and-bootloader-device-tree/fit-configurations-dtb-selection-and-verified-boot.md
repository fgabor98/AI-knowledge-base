---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# FIT Configurations, DTB Selection, And Verified Boot

The Flat Image Tree format packages kernels, DTBs, ramdisks, firmware, and other payloads with named configurations. A configuration binds a compatible set of images. Selection and verification must cover the configuration relationship, not merely hash each file independently.

## FIT Is A Container Description

TI's `tispl.bin` for AM64x is a FIT built by binman. Its images and configurations, from `k3-binman.dtsi` and `k3-am64x-binman.dtsi` (trimmed):

```dts
fit {
        description = "Configuration to load ATF and SPL";
        #address-cells = <1>;

        images {
                atf {
                        description = "ARM Trusted Firmware";
                        type = "firmware";
                        arch = "arm64";
                        os = "arm-trusted-firmware";
                        load = <CONFIG_K3_ATF_LOAD_ADDR>;
                        entry = <CONFIG_K3_ATF_LOAD_ADDR>;
                };
                tee { /* OP-TEE at CONFIG_K3_OPTEE_LOAD_ADDR */ };
                dm { /* empty on AM64x */ };
                spl { /* A53 SPL at CONFIG_SPL_TEXT_BASE */ };

                fdt-0 {
                        description = "k3-am642-evm";
                        type = "flat_dt";
                        /* spl/dts/ti/k3-am642-evm.dtb */
                };

                fdt-1 {
                        description = "k3-am642-sk";
                        type = "flat_dt";
                        /* spl/dts/ti/k3-am642-sk.dtb */
                };
        };

        configurations {
                default = "conf-0";

                conf-0 {
                        description = "k3-am642-evm";
                        firmware = "atf";
                        loadables = "tee", "dm", "spl";
                        fdt = "fdt-0";
                };

                conf-1 {
                        description = "k3-am642-sk";
                        firmware = "atf";
                        loadables = "tee", "dm", "spl";
                        fdt = "fdt-1";
                };
        };
};
```

This tree describes payload relationships. `fdt-1` contains a hardware tree; the FIT's own root is not passed to anything as a board description. On HS devices, each image is wrapped by a `ti-secure` entry so the R5 SPL can authenticate it through system firmware.

## Configuration Selection

A FIT can contain several DTBs and configurations. Selection can be:

- explicit in a `bootm` image/configuration expression
- the FIT's declared default
- chosen by board/platform matching logic, as on AM64x, where the R5 SPL calls `board_fit_config_name_match()` for each configuration description and accepts `k3-am642-sk` only when the EEPROM says `AM64-SKEVM` or `AM64B-SKEVM`. `fit_find_config_node()` in `boot/common_fit.c` matches on each configuration's `description`, not its node name, and falls back to `default` (here the EVM) when nothing matches
- selected by SPL while loading U-Boot and other firmware
- integrated with bootstd or a product-specific policy

For every product, document:

```text
identity source
  -> normalization and validation
  -> configuration-name mapping
  -> unknown/revision fallback
  -> selected kernel, DTB, ramdisk, and loadables
```

TI's kernel-FIT path names configurations after the DTB file. `ti_common.env` builds the configuration from `fdtfile` and any overlays:

```text
get_fit_config=setexpr name_fit_config gsub / _ conf-${fdtfile}
run_fit=run get_fit_config; bootm ${addr_fit}#${name_fit_config}${overlaystring}
```

So `fdtfile=ti/k3-am642-sk.dtb` selects `conf-ti_k3-am642-sk.dtb`, and each entry in `name_overlays` appends `#conf-<overlay>`. The configuration name is now an interface between the board code, the environment, and the FIT build.

Matching only a broad compatible can choose a tree that boots but drives the wrong regulator, GPIO, or memory topology. Use the strongest stable identity available and define forward-compatible revision policy.

## Image Hashes Versus Signatures

A hash detects corruption only if the expected hash is itself trusted; a signature authenticates content against a public key anchored in an earlier trusted stage. Signed configurations matter because individually signed images can still form an unauthorized combination. Never say “the FIT is signed” without naming the required key and policy, the selected configuration, covered images, algorithms, rollback policy, and alternate boot paths.

Verification public keys are inserted into U-Boot's control FDT, which must itself be authenticated; a key carried only inside the FIT lets an attacker replace both. Include the final key-bearing control DTB in build provenance, size checks, and bootloader signing. [FIT Authenticated Selection And Key Policy](../security-and-production-lifecycle/fit-authenticated-selection-and-key-policy.md) covers signing flows, key policy, bypass testing, and anti-rollback in depth.

## DTB And Overlay Verification

If overlays are loaded separately, verify them under the same trust policy before applying them. A signed base DTB plus an unsigned overlay does not produce a trusted final description.

Options include:

- configurations referencing authenticated base and overlay images
- product-specific signed bundles
- a signed filesystem or partition whose policy authorizes artifacts

Prove that selection metadata is also protected. An attacker who can redirect `fdtfile` to another valid but unauthorized tree can change hardware access without modifying that tree.

## Load And Relocation Safety

FIT metadata can define load and entry addresses. U-Boot also relocates or decompresses images according to platform and commands. Build a memory map covering:

- FIT source buffer
- kernel compressed and decompressed ranges
- initrd
- selected DTB plus expansion space
- overlay blobs
- U-Boot relocated image and malloc arena
- secure firmware and reserved memory

Authentication before use does not prevent accidental overlap after verification. A decompressor can overwrite the working FDT if address planning is wrong.

## Inspect A FIT

Host-side:

```sh
mkimage -l product.itb
dumpimage -l product.itb
fdtdump product.itb
```

At the U-Boot prompt, `iminfo` and verbose `bootm` output can show selected configuration, subimages, hashes, signatures, load addresses, and FDT choice depending on configuration.

Archive the ITS or generated source, exact payload hashes, final FIT hash, signing log, and final key-bearing control DTB.

## Failure Patterns

### Valid Kernel, Wrong DTB

The default configuration or board match selected a valid but incorrect pair. Inspect selection input and the configuration references, not the DTB filename printed by the build.

### Hash Passes Or Signature Passes, Security Still Fails

Hashes are unkeyed, signatures are optional, the required key is not marked required, an unsigned boot path remains enabled, or an authentic but old release is accepted without [anti-rollback](../security-and-production-lifecycle/fit-authenticated-selection-and-key-policy.md#signature-is-not-freshness).

### Modified DTB After Verification

Normal fixups may be expected, but the final handed-off tree no longer hashes to the packaged image. Record authorized mutation provenance and protect the mutation code and inputs.

## Authoritative References

- [U-Boot FIT documentation](https://docs.u-boot.org/en/latest/usage/fit/index.html)
- [U-Boot FIT format](https://docs.u-boot.org/en/latest/usage/fit/source_file_format.html)
- [U-Boot FIT signature verification](https://docs.u-boot.org/en/latest/usage/fit/signature.html)
- [U-Boot verified boot](https://docs.u-boot.org/en/latest/usage/fit/verified-boot.html)
- [U-Boot `bootm` command](https://docs.u-boot.org/en/latest/usage/cmd/bootm.html)

## Continue

Proceed to [Environment, Bootstd, Extlinux, And OS DTB Loading](environment-bootstd-extlinux-and-os-dtb-loading.md).
