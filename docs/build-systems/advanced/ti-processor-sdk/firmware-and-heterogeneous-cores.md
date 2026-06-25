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
