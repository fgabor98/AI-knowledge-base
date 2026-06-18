---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# FIT Images and Boot Artifacts

## What Problem Does This Solve?

U-Boot often loads more than one artifact: kernel image, DTB, initramfs, firmware, scripts, or another bootloader stage. FIT images provide a structured way to package artifacts with metadata, hashes, configurations, and signatures.

For embedded Linux work, FIT knowledge is essential when changing boot artifacts, signing policy, DTB selection, or kernel deployment layout.

## Core Concepts

- FIT image
- ITS source
- `mkimage`
- `dumpimage`
- image node
- configuration node
- hash
- signature
- load address
- entry address
- boot script
- `bootm`

## Mental Model

FIT source describes a package:

```text
.its source
-> mkimage
-> .itb FIT image
-> U-Boot selects configuration
-> U-Boot loads kernel, DTB, ramdisk, firmware
```

The FIT image is not just an archive. It encodes boot metadata and may encode trust policy.

## FIT Source Structure

Simplified ITS:

```dts
/dts-v1/;

/ {
    description = "Example FIT";

    images {
        kernel {
            description = "Linux kernel";
            data = /incbin/("Image");
            type = "kernel";
            arch = "arm64";
            os = "linux";
            compression = "none";
            load = <0x80200000>;
            entry = <0x80200000>;
        };

        fdt {
            description = "Board DTB";
            data = /incbin/("board.dtb");
            type = "flat_dt";
            arch = "arm64";
            compression = "none";
        };
    };

    configurations {
        default = "conf";
        conf {
            kernel = "kernel";
            fdt = "fdt";
        };
    };
};
```

Real systems may include multiple kernels, DTBs, ramdisks, firmware blobs, hashes, and signatures.

## Creating A FIT

```sh
mkimage -f image.its image.itb
```

Inspect:

```sh
dumpimage -l image.itb
mkimage -l image.itb
```

U-Boot's `mkimage` version matters. Use the one from the intended U-Boot build or SDK when possible.

## Configurations

Configurations select artifact combinations:

```text
conf-am62x-sk
  kernel = kernel
  fdt = k3-am625-sk
  ramdisk = initramfs

conf-custom-board
  kernel = kernel
  fdt = custom-board
```

If the wrong DTB is used, the FIT configuration may be wrong even if all individual images are present.

## Hashes And Signatures

Hashes detect corruption. Signatures support verified boot.

Changing any included artifact changes its hash. In signed flows, changing a kernel, DTB, ramdisk, or configuration requires resigning.

Practical rule:

- do not modify FIT contents manually
- regenerate from source inputs
- archive ITS source and signing inputs policy
- verify selected config on target

## Load And Entry Addresses

FIT metadata can include load and entry addresses. Wrong addresses can cause boot failures that look like kernel bugs.

Check:

- SoC memory map
- kernel expected load address
- U-Boot relocation behavior
- initramfs placement
- DTB placement
- overlap between loaded artifacts

## Boot Scripts And Environment

U-Boot may boot a FIT through environment variables or scripts:

```text
bootcmd
boot_targets
bootfile
loadaddr
fdt_addr_r
kernel_addr_r
ramdisk_addr_r
```

The build may produce the correct FIT while the board boots another file because environment variables point elsewhere.

## Board-Specific Boot Artifacts

Not every artifact is a Linux FIT. Some outputs are bootloader-stage packages. For TI platforms, common artifact names may include:

```text
tiboot3.bin
tispl.bin
u-boot.img
u-boot.itb
```

Know which artifacts are consumed by boot ROM, firmware, SPL, U-Boot proper, and Linux boot.

## Common Mistakes

- Updating kernel image but not regenerating FIT.
- Regenerating FIT but booting an old FIT file.
- Adding a DTB but not adding a configuration.
- Signing images but not configurations.
- Using wrong load addresses.
- Inspecting files on boot partition while U-Boot loads from network or eMMC.
- Mixing `mkimage` from a different U-Boot/SDK.

## Debugging Checklist

- Inspect FIT with `dumpimage -l`.
- Confirm default configuration.
- Confirm selected U-Boot boot command.
- Confirm environment points to expected FIT.
- Confirm hashes/signatures after changes.
- Confirm load addresses do not overlap.
- Confirm serial log shows selected config if available.
- Confirm Linux runtime DTB matches expected config.

## Related Topics

- [Device Tree in U-Boot](device-tree-in-u-boot.md)
- [Cross-Building and Flashing](cross-building-and-flashing.md)
- [Debugging U-Boot Builds](debugging-u-boot-builds.md)
- [Kernel Release Artifacts](../linux-kernel/kernel-release-artifacts.md)

## References

- U-Boot FIT image documentation
- U-Boot `mkimage` documentation
- U-Boot verified boot documentation
