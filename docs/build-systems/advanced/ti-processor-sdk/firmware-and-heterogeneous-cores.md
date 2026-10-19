---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Firmware and Heterogeneous Cores

## Goal

Understand how TI SDK builds and packages firmware for non-Linux cores and device subsystems.

## Why Firmware Matters

Many TI SoCs are heterogeneous systems. Linux may not be the only software running on the chip. Depending on the device, you may deal with:

- R5 cores
- M4 cores
- PRU-ICSS cores
- system firmware
- security firmware
- power-management firmware
- GPU/display/video firmware
- Wi-Fi/Bluetooth firmware

Firmware version mismatches can look like driver bugs, remoteproc failures, bootloader failures, or missing peripherals.

## Firmware Classes On K3

On K3 devices the firmware falls into two groups that are built and loaded very differently:

| Firmware | Runs on | Loaded by | Recipe (meta-ti-bsp) | Ends up in |
| --- | --- | --- | --- | --- |
| TIFS (security, clocks, power, resources) | security subsystem | boot ROM, packed in `tiboot3.bin` | `ti-sci-fw` (`ti-sysfw/ti-fs-firmware-*`) | `tiboot3.bin` |
| DM (device management, TISCI server) | wakeup R5F after SPL | R5 SPL, packed in `tispl.bin` | `ti-dm-fw` (`ti-dm/<soc>/...`) | `tispl.bin` |
| TF-A BL31, OP-TEE BL32 | A53, secure world | A53 SPL, packed in `tispl.bin` | `trusted-firmware-a`, `optee-os` (meta-arm) | `tispl.bin` |
| M4F / R5F application firmware | MCU and main-domain cores | Linux remoteproc | `ti-rtos-fw`, `zephyr-fw`, product recipes | `/lib/firmware` in the rootfs |
| PRU firmware | PRU-ICSS | Linux remoteproc / Ethernet drivers | `prueth-fw`, `pruhsr-fw`, ... | `/lib/firmware` |
| GPU, video, Wi-Fi firmware | peripherals | Linux drivers | `ti-img-*`, `linux-firmware` | `/lib/firmware` |

The first three are **boot firmware**: they are inputs to U-Boot's binman, so changing them means rebuilding U-Boot (both configurations) and reflashing the boot files. The TIFS, DM, and TISCI client in U-Boot and Linux must be compatible; take them from the same SDK release. Their versions are pinned in `meta-ti-bsp/recipes-bsp/ti-linux-fw/ti-linux-fw.inc` (`TI_SYSFW_VERSION`, `TI_DM_FW_VERSION`, `TI_LINUX_FW_SRCREV`). `ti-sci-fw`, `ti-dm-fw`, and most runtime firmware recipes (PRU, Ethernet, video, RTOS test firmware) fetch the same `ti-linux-firmware` repository at that one revision; TF-A and OP-TEE come from their own upstream sources through meta-arm.

The rest are **runtime firmware**: packages in the root filesystem. They can usually be updated with the rootfs, but the device tree reserves their memory (`memory-region`) and mailboxes, so a firmware image built for different memory carve-outs fails at `remoteproc` start.

## Firmware In The Build

Firmware can enter the image through:

- firmware recipes
- binary packages
- source-built firmware projects
- machine dependencies
- image package groups
- bootloader dependencies
- rootfs firmware install paths

Common rootfs location:

```text
/lib/firmware/
```

Remoteproc firmware names and paths must match what the kernel driver expects.

## Remoteproc Flow

```mermaid
flowchart TD
    Recipe[Firmware recipe] --> Package[Firmware package]
    Package --> Rootfs[/lib/firmware in rootfs]
    Kernel[Kernel remoteproc driver] --> Name[Expected firmware name]
    Rootfs --> Load[Runtime firmware load]
    Name --> Load
    Load --> Core[Remote core starts]
```

## Runtime Inspection

Useful checks:

```bash
dmesg | grep -i remoteproc
dmesg | grep -i firmware
find /lib/firmware -maxdepth 3 -type f | sort
ls /sys/class/remoteproc/
cat /sys/class/remoteproc/remoteproc*/state
cat /sys/class/remoteproc/remoteproc*/firmware
```

The exact sysfs nodes depend on kernel version and enabled drivers.

## PRU Considerations

PRU work may involve:

- PRU firmware binaries
- RPMsg examples
- industrial communication stacks
- pinmux and device-tree nodes
- UIO vs remoteproc driver model
- userspace libraries and headers

Decide early whether PRU firmware is product source, vendor binary, or generated artifact from another build. That decision affects licensing, reproducibility, and CI.

## Firmware Ownership

Product firmware should have a clear owner:

- recipe that installs the firmware
- versioning policy
- source or binary provenance
- license metadata
- runtime validation test
- upgrade compatibility policy

Do not copy firmware manually into a rootfs after image generation. That bypasses manifests and reproducibility.

## Common Mistakes

- Updating kernel remoteproc nodes without updating firmware package names.
- Installing firmware manually on the target and forgetting to add it to the image.
- Mixing firmware from a different SDK release.
- Treating PRU/R5/M4 firmware as outside the Linux product release.
- Ignoring reserved-memory DTS requirements.
- Debugging remote core startup without checking `dmesg`.

## Related Topics

- [Kernel Integration](kernel-integration.md)
- [Multiconfig and Firmware Builds](../yocto-openembedded/multiconfig-and-firmware-builds.md)
- [Release Engineering and SDK Upgrades](release-engineering-and-sdk-upgrades.md)
