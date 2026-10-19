---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Boot Artifact Pipeline

## Goal

Map every file that boots a TI K3 device (AM62x is the running example) to the core that executes it, the recipe that builds it, and the inputs it is assembled from. Then you can answer the two questions behind most bring-up failures: *which file is actually running?* and *which build produced it?*

The metadata quoted here is from meta-ti `wrynose` at the revision pinned in `examples/build-systems/yocto/kb-wrynose.conf.json` (the TI Processor SDK 12.x baseline). See [Release Baseline](index.md#release-baseline) for what differs on scarthgap (SDK 11.x).

## The K3 Boot Chain

A K3 SoC is several computers in one package. The boot chain hands control between them:

```mermaid
flowchart TD
    ROM["Boot ROM (security subsystem)"] -->|loads tiboot3.bin into SRAM| R5["R5 SPL on the wakeup R5F core"]
    TIFS["TIFS: TI Foundational Security firmware"] -.packaged in tiboot3.bin.-> R5
    R5 -->|initializes DDR, loads tispl.bin| TFA["TF-A BL31 on A53"]
    TFA --> OPTEE["OP-TEE BL32 on A53"]
    OPTEE --> A53SPL["A53 SPL"]
    R5 -->|after handoff, the R5 runs| DM["DM firmware (TISCI server)"]
    A53SPL -->|loads u-boot.img| UBOOT["U-Boot proper on A53"]
    UBOOT -->|loads Image, DTB, initramfs or FIT| LINUX[Linux]
    LINUX -->|remoteproc, /lib/firmware| REMOTE["M4F / R5F / PRU firmware"]
```

| Stage | Runs on | Comes from | Purpose |
| --- | --- | --- | --- |
| Boot ROM | security subsystem, then wakeup R5F | silicon | authenticates and loads `tiboot3.bin` |
| `tiboot3.bin` | wakeup R5F | U-Boot built for the R5 (32-bit) + TIFS + board configuration | DDR init, load `tispl.bin` |
| TIFS | security enclave | `ti-linux-firmware` (`ti-sysfw/`) | security, clocks, power, and resource management services |
| DM firmware | wakeup R5F, after R5 SPL finishes | `ti-linux-firmware` (`ti-dm/`) | device-management half of the TISCI services ("split firmware") |
| `tispl.bin` | A53 | FIT image: TF-A BL31 + OP-TEE + DM + A53 SPL | secure world, then A53 SPL |
| `u-boot.img` | A53 | U-Boot built for the A53 (64-bit) | boot environment, loads Linux |
| kernel, DTB(s) | A53 | `linux-ti-staging` | Linux |
| remote core firmware | M4F, R5F, PRU | firmware recipes into `/lib/firmware` | loaded by Linux remoteproc |

Upstream U-Boot classifies K3 devices by how the firmware is split. AM62x is "combined binary, split firmware": ROM loads one `tiboot3.bin` that carries TIFS, while the DM part of the system firmware runs later on the R5 core that ran SPL. Other K3 families (AM65x, J721E) use a separate `sysfw.itb`. Check which class your SoC is before reusing another family's instructions.

### Device Types: GP, HS-FS, HS-SE

The boot ROM behaves differently depending on the device's security state, so `tiboot3.bin` exists in variants:

| Device type | Meaning | `tiboot3` file built for it |
| --- | --- | --- |
| GP | general purpose, no authentication | `tiboot3-am62x-gp-evm.bin` |
| HS-FS | high security, *field securable*: not yet fused with customer keys; ROM checks signatures but accepts any key | `tiboot3-am62x-hs-fs-evm.bin` |
| HS-SE | high security, *security enforced*: fused, only images signed with the customer key boot | `tiboot3-am62x-hs-evm.bin` |

Production silicon and current EVMs are usually HS-FS. A GP image on an HS device, or an unsigned image on an HS-SE device, stops in ROM with no console output at all. The deploy directory contains all variants plus a `tiboot3.bin` symlink. Know which one the symlink points to and which one your board needs.

## How Yocto Builds Two Architectures In One Build: The `k3r5` Multiconfig

The R5 and A53 halves need different compilers (Armv7 bare metal vs. AArch64 Linux), so they are two BitBake configurations in one build. Every AM62x machine pulls this in from meta-ti:

```text
# meta-ti-bsp/conf/machine/include/mc_k3r5.inc
BBMULTICONFIG += "k3r5"

do_image[mcdepends] += "mc::k3r5:virtual/bootloader:do_deploy"
```

```text
# meta-ti-bsp/conf/multiconfig/k3r5.conf
MAINMACHINE := "${MACHINE}"

DEPLOY_DIR_IMAGE:k3r5 = "${TI_COMMON_DEPLOY}/images/${MAINMACHINE}"

MACHINE:append = "-k3r5"
TCLIBC = "baremetal"
TI_TMPDIR_APPEND ?= "-k3r5"
TMPDIR:append = "${TI_TMPDIR_APPEND}"
```

Read these four lines carefully, because they explain most of the TI-specific build behavior:

- The `k3r5` configuration builds for `MACHINE` + `-k3r5` (`am62xx-evm-k3r5`), with a bare-metal C library and its own `TMPDIR` (`tmp-k3r5`). Expect two tmp directories and two toolchains.
- Both configurations deploy into the same images directory, so `tiboot3*.bin` from the R5 build sits next to `tispl.bin` and `u-boot.img` from the A53 build.
- `do_image[mcdepends]` makes every image wait for the R5 bootloader's `do_deploy`. You never build `tiboot3.bin` by hand.
- **A custom board needs two machine files**: `myboard.conf` and `myboard-k3r5.conf`. Forgetting the second one is the classic first error when copying an EVM machine.

The R5 machine file selects the R5 defconfig and names its output:

```text
# meta-ti-bsp/conf/machine/am62xx-evm-k3r5.conf (excerpt)
require conf/machine/include/k3r5.inc
UBOOT_MACHINE = "am62x_evm_r5_defconfig"

# meta-ti-bsp/conf/machine/include/k3r5.inc (excerpt)
PREFERRED_PROVIDER_virtual/kernel = "linux-dummy"
UBOOT_SUFFIX = "bin"
UBOOT_BINARY = "tiboot3.${UBOOT_SUFFIX}"
```

The `k3r5` configuration has no kernel (`linux-dummy`); its only real product is `tiboot3.bin`.

## From Recipe To File

The A53 U-Boot recipe passes exactly the variables upstream U-Boot documents for a manual K3 build (`BL31=`, `TEE=`, `TI_DM=`, `BINMAN_INDIRS=`), with paths into the recipe sysroot:

```text
# meta-ti-bsp/recipes-bsp/u-boot/u-boot-ti.inc (excerpt)
ATF_BL31_PATH = "${STAGING_DIR_HOST}/firmware/trusted-firmware-a/bl31.bin"
OPTEE_BL32_PATH = "${STAGING_DIR_HOST}${nonarch_base_libdir}/firmware/bl32.bin"
TI_DM_PATH = "${STAGING_DIR_HOST}${nonarch_base_libdir}/firmware/ti-dm/${PLAT_SFX}/${DM_FIRMWARE}"

PACKAGECONFIG[atf] = "BL31=${ATF_BL31_PATH},,trusted-firmware-a"
PACKAGECONFIG[optee] = "TEE=${OPTEE_BL32_PATH},,optee-os"
PACKAGECONFIG[dm] = "TI_DM=${TI_DM_PATH},,ti-dm-fw"

PACKAGECONFIG:append:aarch64 = " atf optee"
PACKAGECONFIG:append:am62xx = " dm"

BINMAN_INDIRS = "${STAGING_DIR_HOST}${nonarch_base_libdir}/firmware"
EXTRA_OEMAKE += "${PACKAGECONFIG_CONFARGS} BINMAN_INDIRS=${BINMAN_INDIRS}"
```

Each `PACKAGECONFIG` entry does two things: it adds a make argument and a build dependency (`trusted-firmware-a`, `optee-os`, `ti-dm-fw`). This is the same pattern any recipe uses to wrap a Makefile (see [Recipes: Makefile-based software](../yocto-openembedded/recipes.md#makefile-based-software)). U-Boot's **binman** tool then assembles the final images from those inputs according to the board's device tree.

| Deployed file | Built by (recipe, configuration) | Inputs |
| --- | --- | --- |
| `tiboot3-am62x-{gp,hs-fs,hs}-evm.bin`, `tiboot3.bin` | `u-boot-ti-staging`, `k3r5` multiconfig | R5 SPL, TIFS from `ti-sci-fw`, board config |
| `tispl.bin` | `u-boot-ti-staging`, main configuration | A53 SPL, `bl31.bin` (`trusted-firmware-a`, `TFA_PLATFORM = "k3"`, `TFA_BOARD = "lite"`), `bl32.bin` (`optee-os`, `OPTEEMACHINE = "k3-am62x"`), DM (`ti-dm-fw`) |
| `u-boot.img` | `u-boot-ti-staging`, main configuration | A53 U-Boot proper and its DTB |
| `Image`, `*.dtb`, `*.dtbo` | `linux-ti-staging` | `KERNEL_DEVICETREE` from the machine |
| `fitImage` (signed) | `ti-kernel-fitimage` | kernel + DTBs, signed with keys from `TI_SECURE_DEV_PKG` |
| `*.wic.xz`, `*.wic.bmap` | the image recipe | `WKS_FILE`, `IMAGE_BOOT_FILES` |

Which recipe is "the" kernel or bootloader is selected by one variable, `TI_PREFERRED_BSP`. Its default and alternatives are in `meta-ti-bsp/conf/machine/include/ti-bsp.inc`:

```text
# Supported options are: mainline, next, ti-6_18, ti-6_12, ti-6_6
TI_PREFERRED_BSP ??= "ti-6_18"

BSP_KERNEL_PROVIDER:bsp-ti-6_18 = "linux-ti-staging"
BSP_KERNEL_VERSION:bsp-ti-6_18 = "6.18%"
BSP_BOOTLOADER_PROVIDER:bsp-ti-6_18 = "u-boot-ti-staging"
BSP_BOOTLOADER_VERSION:bsp-ti-6_18 = "2026.01%"
```

`TI_PREFERRED_BSP = "mainline"` in your distro switches both to upstream kernel and U-Boot. Use that switch for upstreaming work, not partial `PREFERRED_PROVIDER` edits. The active values for the reference build are in [Machines, Distros, and Image Targets](machines-distros-and-image-targets.md).

## What Lands On The SD Card

The machine configuration decides which deployed files the WIC image copies into the FAT boot partition:

```text
# meta-ti-bsp/conf/machine/include/k3.inc (excerpt)
IMAGE_FSTYPES += "tar.xz wic.xz wic.bmap"
IMAGE_BOOT_FILES += "${SPL_BINARYNAME} u-boot.${UBOOT_SUFFIX} tiboot3.bin tiboot3-*-evm.bin"
WKS_FILE ?= "${@bb.utils.contains("MACHINE_FEATURES", "efi", "sdimage-2part-efi.wks.in", "sdimage-2part.wks", d)}"
do_image_wic[depends] += "virtual/bootloader:do_deploy"
```

In SD boot mode the ROM reads `tiboot3.bin` from the first FAT partition, and SPL then reads `tispl.bin` and `u-boot.img` from the same partition. Because the boot partition carries every `tiboot3-*-evm.bin` variant plus the `tiboot3.bin` name the ROM looks for, "which variant is `tiboot3.bin`?" is a question to answer on the media, not only in the deploy directory.

On eMMC or OSPI boot the same three files are written to raw boot partitions or flash offsets instead of a FAT filesystem, and a stale copy there can silently win over the SD card. See [Deployment and Flashing](deployment-and-flashing.md).

## Deploy Directory Inspection

meta-ti moves the deploy directory for all TI machines out of `tmp/`: its `layer.conf` sets `TI_COMMON_DEPLOY ?= "${TOPDIR}/deploy-ti"` and `DEPLOY_DIR_IMAGE:ti-soc = "${TI_COMMON_DEPLOY}/images/${MACHINE}"` (SDK installers go to `deploy-ti/sdk`). Ask BitBake instead of guessing:

```bash
DEPLOY=$(bitbake-getvar --quiet --value DEPLOY_DIR_IMAGE)
ls -l "$DEPLOY" | grep -E 'tiboot3|tispl|u-boot|Image|\.dtb|wic'
readlink -f "$DEPLOY"/tiboot3.bin "$DEPLOY"/tispl.bin "$DEPLOY"/u-boot.img
sha256sum "$DEPLOY"/tiboot3.bin "$DEPLOY"/tispl.bin "$DEPLOY"/u-boot.img
```

Yocto deploy directories contain versioned files plus stable symlinks. Record both the symlink and its target in release notes, and record checksums of what you flashed.

To check what is inside the WIC image without flashing it (`wic` needs the uncompressed image):

```bash
xz -dkc "$DEPLOY"/kb-image-am62xx-evm.rootfs.wic.xz > /tmp/kb-image.wic
wic ls /tmp/kb-image.wic:1
wic cp /tmp/kb-image.wic:1/tiboot3.bin /tmp/tiboot3-from-wic.bin
cmp /tmp/tiboot3-from-wic.bin "$DEPLOY"/tiboot3.bin
```

## Runtime Verification

The R5 SPL and A53 SPL banners on the serial console identify every stage. Save the full boot log from power-on; it is the cheapest provenance record you have. On the target:

```bash
cat /proc/version
cat /proc/cmdline
tr -d '\0' < /proc/device-tree/model; echo
dmesg | grep -iE 'ti_sci|sysfw|tifs|optee|psci'
cat /sys/class/remoteproc/remoteproc*/firmware 2>/dev/null
```

From U-Boot:

```text
version
printenv
bdinfo
fdt addr ${fdtcontroladdr}
fdt print / model
```

The goal is to prove which TIFS/DM, TF-A, OP-TEE, U-Boot, kernel, DTB, and rootfs are actually running, not which ones you built last.

## Common Mistakes

- Creating `myboard.conf` without `myboard-k3r5.conf`, or with an R5 defconfig for a different board.
- Booting a GP `tiboot3.bin` on an HS-FS or HS-SE device and seeing nothing on the console.
- Rebuilding U-Boot but booting an older copy from eMMC boot partitions or OSPI.
- Updating `ti-linux-firmware` (TIFS/DM) without rebuilding both U-Boot configurations; TIFS and DM versions must match each other and the TISCI ABI that U-Boot and Linux expect.
- Rebuilding the DTB but writing only the rootfs partition.
- Assuming the newest timestamp in the deploy directory is what WIC packed.
- Debugging Linux when the failure is in R5 SPL, TIFS, or the A53 handoff (no A53 SPL banner means Linux is irrelevant).

## Debugging Checklist

- Which device type is the board (GP, HS-FS, HS-SE), and which `tiboot3` variant is `tiboot3.bin` on the media?
- Does the console show the R5 SPL banner? The A53 SPL banner? U-Boot proper? Stop at the first missing one.
- Are `TFA_PLATFORM`, `TFA_BOARD`, `OPTEEMACHINE`, and `UBOOT_MACHINE` (both configurations) what the machine files intend? Check with `bitbake-getvar`, and use `bitbake-getvar` inside the `k3r5` configuration for the R5 values.
- Were TF-A, OP-TEE, DM, and TIFS staged into the U-Boot recipe sysroot from the expected versions? Check the U-Boot `log.do_compile` for the `BL31=`, `TEE=`, `TI_DM=` arguments.
- Do the checksums of the files on the boot media match the deploy directory?

## Related Topics

- [TI Yocto and Arago Build Flow](ti-yocto-arago-build-flow.md)
- [U-Boot Integration](u-boot-integration.md)
- [Firmware and Heterogeneous Cores](firmware-and-heterogeneous-cores.md)
- [Multiconfig and Firmware Builds](../yocto-openembedded/multiconfig-and-firmware-builds.md)
- [SPL, TPL, and U-Boot Proper](../u-boot/spl-tpl-and-u-boot-proper.md)
- [Deployment and Flashing](deployment-and-flashing.md)

## References

- [U-Boot: Texas Instruments K3 boot flow and build](https://docs.u-boot.org/en/latest/board/ti/k3.html)
- [U-Boot: AM62x SK board documentation](https://docs.u-boot.org/en/latest/board/ti/am62x_sk.html)
- [meta-ti repository](https://git.yoctoproject.org/meta-ti/) (`meta-ti-bsp/conf/machine/include/`, `recipes-bsp/u-boot/`)
- [U-Boot binman documentation](https://docs.u-boot.org/en/latest/develop/package/binman.html)
- TI Processor SDK Linux AM62x documentation and release notes for the selected SDK version
