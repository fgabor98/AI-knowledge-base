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

## Custom Machine

A product board usually deserves a machine file when hardware policy differs from the EVM.

Machine metadata may own:

- kernel DTB list
- U-Boot machine/defconfig
- WIC layout
- machine features
- serial console
- firmware dependencies
- boot media assumptions

Avoid using `local.conf` as the permanent location for board policy.

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
- Debugging kernel drivers while booting the wrong DTB.
- Forgetting U-Boot may use its own device tree.
- Updating SD card artifacts while the board boots from eMMC or OSPI.
- Treating pinmux as a Linux-only issue when early boot needs pins too.

## Related Topics

- [Machines, Distros, and Image Targets](machines-distros-and-image-targets.md)
- [Kernel Integration](kernel-integration.md)
- [U-Boot Integration](u-boot-integration.md)
- [Deployment and Flashing](deployment-and-flashing.md)
