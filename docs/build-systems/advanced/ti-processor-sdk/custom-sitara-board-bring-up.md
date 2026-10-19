---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Custom Sitara Board Bring-Up

## Goal

Move from a TI EVM baseline to a custom Sitara product board without losing build-system ownership.

## Baseline Strategy

Start with the closest EVM. Prove:

- prebuilt EVM image boots
- source-built EVM image boots
- serial console works
- boot media path is understood
- kernel version and DTB are known
- U-Boot version and environment are known
- deploy artifacts map to boot stages

Then document product differences.

## Difference Inventory

Create a board delta table:

| Area | EVM | Product board | Build impact |
| --- | --- | --- | --- |
| DDR | EVM memory | product memory | SPL/DDR config |
| Boot media | SD/eMMC/OSPI | selected media | U-Boot, WIC, flashing |
| PMIC/regulators | EVM PMIC | product power tree | DTS, boot sequencing |
| Ethernet PHY | EVM PHY | product PHY | DTS, kernel config |
| MMC/eMMC | EVM layout | product layout | DTS, WIC, U-Boot env |
| GPIOs | EVM routing | product routing | pinmux, DTS |
| Displays/cameras | EVM peripherals | product peripherals | DTS, drivers, firmware |

This table becomes the input to machine, DTS, U-Boot, and image changes.

## Custom Machine: Two Files On K3

On K3 devices (AM62x, AM64x, AM62A/P, J7) a product machine is **two** machine files, because the `k3r5` multiconfig builds the R5 bootloader for `MACHINE` + `-k3r5` (see [Boot Artifact Pipeline](boot-artifact-pipeline.md#how-yocto-builds-two-architectures-in-one-build-the-k3r5-multiconfig)). The example product layer in this repository has a minimal pair derived from the AM62x EVM:

```text
--8<-- "examples/build-systems/yocto/meta-kb-example/conf/machine/kb-am62-board.conf"
```

```text
--8<-- "examples/build-systems/yocto/meta-kb-example/conf/machine/kb-am62-board-k3r5.conf"
```

The A53 file requires the SoC include (`am62xx.inc`), which pulls in `k3.inc` (tune, kernel image type, boot files, WIC), `mc_k3r5.inc` (the multiconfig), and `ti-bsp.inc` (kernel/U-Boot provider selection). The R5 file requires `k3r5.inc`. Each sets its own `UBOOT_MACHINE`. What happens when the R5 file is missing is shown on [Machines, Distros, and Image Targets](machines-distros-and-image-targets.md#a-product-machine-needs-its-k3r5-twin).

The pair above still points at the EVM defconfigs and device tree, which is a valid first step: it proves the product layer, machine selection, and both configurations before any board-specific change. Then replace one item at a time:

| Item | Where it lives | Notes |
| --- | --- | --- |
| A53 U-Boot defconfig | product U-Boot patch or `.bbappend`, `UBOOT_MACHINE` in `kb-am62-board.conf` | usually a copy of the EVM defconfig with product changes |
| R5 U-Boot defconfig | `UBOOT_MACHINE` in `kb-am62-board-k3r5.conf` | R5 SPL: DDR, boot media, early pinmux |
| DDR configuration | U-Boot R5 device tree (`k3-am62x-*-ddr-*.dtsi`) | generated with TI's DDR register configuration tool for the exact memory part |
| board/resource/power/security configuration | U-Boot `board/ti/<soc>/*-cfg.yaml`, packed into `tiboot3.bin` by binman | resource partitioning must match what Linux and remote cores expect |
| Linux device tree | product DTS via kernel `.bbappend`, `KERNEL_DEVICETREE` | keep U-Boot and Linux DTs consistent |
| WIC layout | `WKS_FILE` in the machine | only when boot media or partitioning differs |
| firmware set | machine `MACHINE_EXTRA_RRECOMMENDS`, image recipes | remote core firmware must match the DT `memory-region`/`mboxes` |

Machine metadata may own the kernel DTB list, both U-Boot defconfigs, WIC layout, machine features, serial consoles, firmware dependencies, and boot media assumptions. Product applications, init system, and package policy belong to the product distro and image recipes, not the machine. Avoid using `local.conf` as the permanent location for board policy: a board is a machine, not a developer preference.

## Device Tree Bring-Up

Bring up device tree incrementally:

1. Start with CPU, memory, chosen, aliases.
2. Enable serial console.
3. Enable pinctrl required for boot media.
4. Enable MMC/eMMC or selected boot storage.
5. Add regulators and fixed clocks.
6. Add Ethernet.
7. Add remaining buses and peripherals one at a time.
8. Validate reserved memory and remoteproc nodes.

Keep DTS changes reviewable. A giant first DTS patch is hard to debug.

## U-Boot Bring-Up

U-Boot may need changes for:

- DDR initialization
- boot media
- board detection
- environment defaults
- Ethernet recovery
- USB recovery
- device tree selection
- secure boot flow

Serial logs from ROM/SPL/U-Boot are critical. Save logs for each stage.

## Linux Bring-Up

Linux bring-up sequence:

1. Boot kernel with early console if needed.
2. Confirm correct DTB model.
3. Confirm rootfs mount.
4. Confirm clocks/regulators are not failing.
5. Confirm storage, networking, and firmware loading.
6. Add product peripherals.
7. Remove development-only debug options when stable.

## Common Mistakes

- Creating many simultaneous changes before first boot.
- Keeping the EVM machine name for production hardware.
- Creating `myboard.conf` without `myboard-k3r5.conf`, or pointing the R5 file at another board's defconfig.
- Changing DDR parts without regenerating the R5 DDR configuration.
- Debugging kernel drivers while booting the wrong DTB.
- Forgetting U-Boot may use its own device tree.
- Updating SD card artifacts while the board boots from eMMC or OSPI.
- Treating pinmux as a Linux-only issue when early boot needs pins too.

## Related Topics

- [Machines, Distros, and Image Targets](machines-distros-and-image-targets.md)
- [Kernel Integration](kernel-integration.md)
- [U-Boot Integration](u-boot-integration.md)
- [Deployment and Flashing](deployment-and-flashing.md)
