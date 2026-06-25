---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Board Porting and Bring-Up

## What Problem Does This Solve?

Board porting adapts U-Boot from a vendor EVM or reference board to custom hardware. It touches defconfig, device tree, board code, SPL, DRAM, PMIC, pinmux, boot media, environment, and Linux handoff.

For embedded Linux developers working with SoCs such as TI Sitara, this is one of the most important U-Boot workflows.

## Core Concepts

- reference EVM
- custom board
- board defconfig
- board DTS
- SPL bring-up
- DRAM init
- PMIC setup
- pinmux
- boot media
- board detection
- Linux handoff
- staged validation

## Mental Model

Bring-up should be staged:

```text
start from closest EVM
-> get serial output
-> get SPL to load U-Boot proper
-> initialize DRAM
-> load from intended boot media
-> pass correct DTB and bootargs to Linux
-> validate peripherals
```

Do not try to validate every peripheral before the boot chain is stable.

## Start From The Closest Reference Board

Choose the closest EVM based on:

- SoC variant
- DDR type and topology
- PMIC/regulator design
- boot media
- Ethernet PHY/interface
- board power sequencing
- SDK support level

For TI Sitara-style work, start from the closest TI EVM in the same SDK release. Then isolate product changes as patches.

## Files Usually Touched

Common areas:

```text
configs/<board>_defconfig
arch/<arch>/dts/<board>.dts
board/<vendor>/<board>/
include/configs/<board>.h
```

Exact paths vary by U-Boot version and architecture.

## Defconfig Strategy

For a custom board:

- copy the closest EVM defconfig only when you need an owned board target
- keep changes minimal
- use `savedefconfig`
- document the vendor baseline
- avoid mixing debug-only options into production defconfig

Keep board enablement distinct from product boot policy where possible.

## Device Tree Strategy

Create or adapt a U-Boot DTS for:

- serial console
- boot media
- PMIC/regulators
- clocks
- pinctrl
- Ethernet PHY
- I2C EEPROM
- reset lines
- devices needed before Linux

U-Boot DTS does not always need every Linux peripheral. It needs enough hardware description for bootloader responsibilities.

## DRAM Bring-Up

DRAM is often the hard boundary between early boot and full bootloader.

Validate:

- memory type
- width and ranks
- timing data
- training firmware if applicable
- voltage rails
- reset sequencing
- board layout differences from EVM

Symptoms of DRAM problems:

- SPL banner appears, then reset
- random crashes before U-Boot prompt
- U-Boot loads but commands crash
- Linux decompresses then hangs

DRAM changes should be treated as high-risk board-port patches.

## PMIC And Regulator Bring-Up

Early boot may need regulators before Linux exists.

Check:

- PMIC I2C address
- power rails required before DRAM
- voltage settings
- enable GPIOs
- reset lines
- sequencing dependencies

If PMIC setup is wrong, failures may look like DRAM, MMC, or CPU instability.

## Pinmux Bring-Up

Early pinmux matters for:

- UART console
- MMC/eMMC
- SPI flash
- I2C PMIC
- Ethernet reset/MDIO
- boot mode pins

Validate pinmux in U-Boot before assuming the driver is broken.

## Serial First

Serial console is the first bring-up milestone.

No serial output can mean:

- wrong boot media
- wrong first-stage image
- UART pins not muxed
- wrong UART instance
- wrong baud rate
- board held in reset
- power rail issue
- secure boot rejection before UART init

Preserve logs from reset, not only from the U-Boot prompt.

## Boot Media Bring-Up

Bring up one boot media at a time:

- SD for recovery
- eMMC for production
- SPI NOR for bootloader storage
- NAND if required
- network boot for development

For each media:

- enable SPL support if needed
- validate pinmux
- validate voltage rails
- validate partition/offset layout
- validate environment storage
- validate recovery path

## Ethernet Bring-Up

Ethernet in U-Boot may require:

- MAC driver
- PHY driver
- MDIO bus
- reset GPIO
- clock source
- pinmux
- PHY address
- RGMII/RMII mode
- environment variables for IP/TFTP

Useful commands:

```text
mdio list
ping <serverip>
dhcp
tftpboot
```

Command availability depends on config.

## Board Detection

Some boards use EEPROM or GPIO straps to identify variants.

Risks:

- U-Boot selects wrong DTB
- wrong MAC address loaded
- wrong PMIC setup
- wrong boot targets
- wrong overlay applied

Keep board-detection logic small and well logged.

## U-Boot Vs Linux Responsibility Split

U-Boot should do what is needed to boot reliably:

- early power
- memory
- boot media
- artifact loading
- kernel handoff

Linux should own full runtime policy:

- complete device initialization
- power management
- full networking stack
- filesystems
- userspace policy

Avoid moving Linux runtime policy into U-Boot unless the boot chain requires it.

## Bring-Up Order

Recommended order:

1. Confirm boot mode and recovery path.
2. Build closest EVM U-Boot unchanged.
3. Get serial output.
4. Confirm SPL and U-Boot proper versions.
5. Validate DRAM.
6. Validate boot media loading.
7. Validate environment behavior.
8. Pass known-good kernel and DTB.
9. Validate Linux `/proc/cmdline` and `/proc/device-tree`.
10. Add peripherals one at a time.

## TI Sitara/K3 Notes

For TI platforms:

- match SDK release to board support baseline
- track firmware artifacts with U-Boot artifacts
- distinguish secure device variants
- keep TI EVM baseline patches separate from custom board patches
- validate `tiboot3.bin`, `tispl.bin`, and U-Boot proper as one chain where applicable
- compare custom board DTS against the closest EVM DTS carefully

## Common Mistakes

- Starting from an unrelated board because it "mostly boots."
- Changing DRAM, PMIC, DTS, and environment in one patch.
- Debugging Linux before proving U-Boot passes the expected DTB.
- Ignoring saved environment from an earlier board revision.
- Enabling many commands and drivers in SPL until size breaks.
- Treating EVM boot media layout as guaranteed for custom hardware.

## Debugging Checklist

- Confirm recovery path.
- Confirm reference board baseline.
- Confirm defconfig changes.
- Confirm U-Boot DTS changes.
- Confirm serial from reset.
- Confirm SPL handoff.
- Confirm DRAM stability.
- Confirm boot media read.
- Confirm environment and boot flow.
- Confirm Linux handoff DTB and bootargs.
- Validate one peripheral at a time.

## Related Topics

- [Board Defconfigs](board-defconfigs.md)
- [Device Tree in U-Boot](device-tree-in-u-boot.md)
- [Driver Model and Pre-Relocation](driver-model-and-pre-relocation.md)
- [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)

## References

- U-Boot board porting documentation
- U-Boot driver model documentation
- TI Processor SDK Linux documentation
