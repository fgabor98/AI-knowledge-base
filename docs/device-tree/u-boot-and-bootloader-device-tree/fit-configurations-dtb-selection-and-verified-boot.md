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

An Image Tree Source might contain:

```dts
/dts-v1/;

/ {
        description = "Product boot bundle";
        #address-cells = <1>;

        images {
                kernel {
                        description = "Linux kernel";
                        data = /incbin/("Image");
                        type = "kernel";
                        arch = "arm64";
                        os = "linux";
                        compression = "none";

                        hash {
                                algo = "sha256";
                        };
                };

                fdt-a {
                        description = "Board A DTB";
                        data = /incbin/("board-a.dtb");
                        type = "flat_dt";
                        arch = "arm64";
                        compression = "none";

                        hash {
                                algo = "sha256";
                        };
                };
        };

        configurations {
                default = "conf-a";

                conf-a {
                        description = "Board A";
                        kernel = "kernel";
                        fdt = "fdt-a";
                };
        };
};
```

This tree describes payload relationships. `fdt-a` contains the hardware tree that becomes the working FDT; the FIT's own root is not passed to Linux as its board description.

## Configuration Selection

A FIT can contain several DTBs and configurations. Selection can be:

- explicit in a `bootm` image/configuration expression
- the FIT's declared default
- chosen by board/platform matching logic
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

Matching only a broad compatible can choose a tree that boots but drives the wrong regulator, GPIO, or memory topology. Use the strongest stable identity available and define forward-compatible revision policy.

## Image Hashes Versus Signatures

A hash detects accidental corruption only if the expected hash is itself trusted. A digital signature authenticates signed content using a public key rooted in trusted storage or an authenticated earlier stage.

U-Boot supports signing images and configurations. Signed configurations protect the association among selected kernel, DTB, ramdisk, and other referenced images. This matters because every individual image could be legitimately signed yet form an unauthorized combination.

Review exactly which nodes and strings the signature covers. Do not say “the FIT is signed” without identifying:

- required public key
- required mode and configuration/image policy
- selected configuration
- referenced images
- algorithms
- rollback/version policy
- behavior for unsigned legacy formats and alternate boot commands

## Where Verification Keys Live

Verification public-key material can be inserted into U-Boot's control FDT. That tree must already be authenticated or immutable under the previous trust stage. Putting the key only inside the untrusted FIT would let an attacker replace both image and key.

Key insertion changes the control DTB after ordinary DTS compilation. Include the final key-bearing control tree in build provenance, size checks, and signing of the bootloader stage.

Plan:

- offline/private-key protection
- development versus production key separation
- key rotation
- revocation
- algorithm transition
- factory and recovery keys
- rollback counters or version fuses

DT nodes encode key parameters; they do not supply the operational key-management policy.

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

### Hash Passes, Security Still Fails

Hashes are unkeyed, signatures are optional, the required key is not marked as required, or an alternate unsigned boot path remains enabled.

### Signature Passes, Rollback Remains Possible

The image is authentic but older than product policy permits. Add an authenticated version/rollback mechanism outside mere signature validity.

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
