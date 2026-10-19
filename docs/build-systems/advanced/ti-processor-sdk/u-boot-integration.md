---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# U-Boot Integration

## Goal

Learn how the TI SDK builds, configures, patches, and deploys U-Boot and related early boot artifacts.

## What The U-Boot Recipe Wraps

The U-Boot recipe wraps the U-Boot build system. BitBake fetches source, applies patches, selects configuration, runs the U-Boot build, and deploys boot artifacts.

Typical outputs may include:

- SPL-related images
- `tiboot3.bin`
- `tispl.bin`
- `u-boot.img`
- environment files
- boot scripts
- configuration metadata

Exact outputs depend on SoC family, boot flow, and SDK release.

## Provider Selection

The same `TI_PREFERRED_BSP` switch that selects the kernel selects U-Boot: `u-boot-ti-staging` 2026.01 for `ti-6_18` (2025.01 for `ti-6_12`), or `u-boot-ti-mainline` / `u-boot-ti-next`. One recipe is built **twice** on K3:

| Configuration | `MACHINE` | `UBOOT_MACHINE` (AM62x EVM) | Main output |
| --- | --- | --- | --- |
| main (A53) | `am62xx-evm` | `am62x_evm_a53_defconfig` | `tispl.bin`, `u-boot.img` |
| `k3r5` multiconfig (R5) | `am62xx-evm-k3r5` | `am62x_evm_r5_defconfig` | `tiboot3-*.bin` |

Inspect both:

```bash
bitbake-getvar --quiet PREFERRED_PROVIDER_virtual/bootloader
bitbake-getvar --quiet --value UBOOT_MACHINE
bitbake -e mc:k3r5:virtual/bootloader | grep -E '^(PN|UBOOT_MACHINE|MACHINE)='
```

## SPL, TPL, And U-Boot Proper

The K3 boot chain, the meaning of GP/HS-FS/HS-SE images, and how TF-A, OP-TEE, DM, and TIFS are packed into `tiboot3.bin` and `tispl.bin` are covered in [Boot Artifact Pipeline](boot-artifact-pipeline.md). From the U-Boot recipe's point of view:

- the R5 build needs `ti-sci-fw` (TIFS) in its sysroot, and binman packs it with the R5 SPL into `tiboot3.bin`;
- the A53 build needs `trusted-firmware-a`, `optee-os`, and `ti-dm-fw`, passed as `BL31=`, `TEE=`, and `TI_DM=` through `PACKAGECONFIG`;
- both builds read firmware from `BINMAN_INDIRS`, the recipe sysroot's firmware directory.

The job of each stage is constrained by SRAM size, boot media, security state, clocks, DDR initialization, and firmware handoff.

## Configuration Changes

Product U-Boot changes may involve:

- defconfig changes
- Kconfig options
- board files
- device tree changes
- environment defaults
- boot command policy
- boot target order
- storage driver enablement
- Ethernet or USB support for recovery

Prefer product-owned patches and config fragments where supported by the release.

## Environment Policy

U-Boot environment may come from:

- compiled default environment
- text-based environment source
- boot scripts
- persistent environment in flash/eMMC
- distro boot variables
- user changes made at the U-Boot prompt

For products, decide whether persistent environment is allowed. A field device with stale environment can ignore new boot policy even after a software update.

Useful commands:

```text
printenv
env default -a
saveenv
version
bdinfo
```

Use `saveenv` carefully. It changes persistent target state.

## Boot Scripts And FIT Images

Some TI workflows use boot scripts, extlinux files, or FIT images. Find the active mechanism before editing random environment variables.

Questions:

- Does U-Boot load `boot.scr`?
- Does it use `extlinux.conf`?
- Does it load a FIT image?
- Where is the kernel command line constructed?
- Which partition and filesystem does U-Boot read?
- Are overlays applied by U-Boot?

## Rebuilding Only U-Boot

Rebuild both configurations; a mismatched R5 and A53 U-Boot is a common source of confusing boot hangs:

```bash
bitbake virtual/bootloader mc:k3r5:virtual/bootloader
```

After changing U-Boot sources in a `devtool` workspace or a patch, BitBake notices the change and reruns the needed tasks. To force a rebuild from one task onward without touching sources, use `-C` (it invalidates that task's stamp and continues):

```bash
bitbake -C compile virtual/bootloader
```

Avoid `-c compile -f` for this: it runs only `do_compile` and leaves `do_install` and `do_deploy` untouched until the next full build, so the deploy directory keeps old files in the meantime. Then inspect the deploy directory and make sure the boot media actually receives the new artifacts.

## Common Mistakes

- Rebuilding U-Boot but not rewriting the boot partition.
- Booting from eMMC while updating SD-card files.
- Changing Linux DT only when U-Boot also needs a DT change.
- Forgetting persistent environment overrides compiled defaults.
- Mixing secure and non-secure boot artifacts.
- Assuming `u-boot.img` alone is enough on systems requiring earlier boot binaries.

## Related Topics

- [U-Boot Build System](../u-boot/index.md)
- [Boot Artifact Pipeline](boot-artifact-pipeline.md)
- [Deployment and Flashing](deployment-and-flashing.md)
