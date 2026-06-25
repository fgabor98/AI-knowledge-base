---
status: draft
reviewed: false
domain: embedded-linux
difficulty: intermediate
last_reviewed: null
---

# Embedded Linux

Embedded Linux topics focused on boot flow, board bring-up, root filesystems, build systems, deployment, and field diagnostics.

For product-level release, test, update, provisioning, and diagnostics workflows, see [Embedded Productization](../embedded-productization/index.md).

For build-system learning paths, start with:

1. [Build Systems for Embedded Linux](../build-systems/embedded-linux-roadmap.md)
2. [Advanced Build Systems](../build-systems/advanced/index.md)
3. [BSP Build Integration](../build-systems/advanced/bsp-build-integration.md)
4. [Linux Kernel Build System](../build-systems/advanced/linux-kernel/index.md)
5. [U-Boot Build System](../build-systems/advanced/u-boot/index.md)
6. [Yocto and OpenEmbedded](../build-systems/advanced/yocto-openembedded/index.md)
7. [TI Processor SDK Linux](../build-systems/advanced/ti-processor-sdk/index.md)

## Roadmap

### Boot Flow And Bring-Up

- ROM boot overview
- SPL/TPL handoff
- U-Boot handoff
- boot media selection
- boot scripts
- `extlinux.conf`
- FIT image handoff
- kernel entry
- kernel command line
- initramfs
- rootfs mount
- init and systemd startup
- serial log failure classification

### Board Bring-Up Workflow

- vendor EVM baseline
- source-built baseline
- custom board delta list
- boot media validation
- serial console validation
- minimal rootfs boot
- Ethernet bring-up
- storage bring-up
- peripheral bring-up
- firmware loading validation
- boot log capture

### Embedded Linux Debugging

- serial console
- `dmesg`
- `journalctl`
- `strace`
- `ltrace`
- `gdbserver`
- core dumps
- `perf` overview
- ftrace overview
- dynamic debug overview
- boot hangs
- kernel panics
- watchdog resets
- support bundles

### Systemd For Embedded Systems

- unit files
- service dependencies
- ordering
- targets
- restart policies
- watchdog integration
- journald
- tmpfiles
- timers
- systemd-networkd overview
- read-only rootfs integration
- service health checks

### Storage And Filesystems

- SD cards
- eMMC
- raw NAND
- NOR/QSPI/OSPI
- UBI and UBIFS
- ext4
- squashfs
- overlayfs
- partitioning
- UUID and PARTUUID
- `/etc/fstab`
- power-loss behavior
- wear considerations

### Runtime Recovery And Provisioning Basics

- rescue shell
- recovery boot path
- initramfs recovery
- factory provisioning runtime flow
- version compatibility checks
- rollback status reporting
- persistent data partition

### Hardware Interface Fundamentals

- GPIO
- I2C
- SPI
- UART
- CAN
- USB
- PCIe
- interrupts
- reset lines
- clocks
- regulators
- pinmux
- logic analyzer workflow

## Related Topics

- [Device Tree](../device-tree/index.md)
- [Linux Kernel Programming](../linux-kernel/index.md)
- [Embedded Productization](../embedded-productization/index.md)
- [Networking](../networking/index.md)
- [Topic Map](../topic-map.md)
