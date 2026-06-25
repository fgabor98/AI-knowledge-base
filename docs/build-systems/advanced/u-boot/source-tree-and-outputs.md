---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Source Tree and Outputs

## What Problem Does This Solve?

U-Boot produces several artifacts, and the correct artifact depends on the SoC, boot ROM, board layout, and boot media. Before changing configuration or flashing a board, you need to know where U-Boot inputs live and which outputs matter.

For embedded Linux work, this prevents a common failure: building the right source but flashing the wrong file.

## Core Concepts

- U-Boot source tree
- board directory
- architecture directory
- `configs/`
- generated output tree
- SPL output
- U-Boot proper output
- DTB output
- packaged image
- boot ROM artifact

## Mental Model

U-Boot has source inputs and generated outputs:

```text
source tree
  arch/
  board/
  configs/
  drivers/
  include/
  tools/

output tree
  .config
  include/generated/
  u-boot
  u-boot.bin
  u-boot.img
  u-boot.dtb
  spl/
  board-specific packaged images
```

With `O=build`, generated files live outside the source tree.

## Source Tree Areas

### `configs/`

Board defconfigs live here:

```text
configs/am62x_evm_a53_defconfig
configs/am64x_evm_a53_defconfig
configs/sandbox_defconfig
```

The defconfig selects the board, architecture, drivers, commands, boot flow, and SPL options.

### `arch/`

Architecture-specific startup code, CPU code, linker scripts, and SoC support.

Examples:

```text
arch/arm/
arch/riscv/
arch/sandbox/
arch/x86/
```

### `board/`

Board-specific code.

Examples:

```text
board/ti/
board/freescale/
board/st/
board/raspberrypi/
```

Board code may handle DRAM setup, pinmux, EEPROM detection, board variants, and late initialization.

### `drivers/`

U-Boot drivers for serial, MMC, SPI, I2C, Ethernet, USB, GPIO, pinctrl, regulators, clocks, and other subsystems.

Driver existence does not mean the driver is built. Kconfig and driver model decide that.

### `tools/`

Host-side tools used to create or inspect boot artifacts.

Common tools:

```text
tools/mkimage
tools/dumpimage
tools/mkenvimage
```

These run on the build host, not on the target.

## Common Outputs

### `u-boot`

ELF image for U-Boot proper. Useful for symbols and debugging.

### `u-boot.bin`

Raw U-Boot binary. Sometimes used directly, but often not the final flashable artifact.

### `u-boot.img`

U-Boot image with a legacy image header, created for boot flows that expect this wrapper.

### `u-boot.dtb`

The device tree blob used by U-Boot proper when U-Boot is configured to use device tree.

### `u-boot.itb`

FIT image. Depending on board, this may package U-Boot proper, DTBs, firmware, or other components.

### `spl/u-boot-spl`

SPL ELF image.

### `spl/u-boot-spl.bin`

Raw SPL binary.

### Board-Specific Images

Some SoCs require images with boot ROM headers, certificates, combined firmware, or vendor-specific packaging.

Examples of names you may see:

```text
MLO
tiboot3.bin
tispl.bin
u-boot.img
u-boot.itb
idbloader.img
flash.bin
```

The names are platform-specific. Always check the board documentation and generated build log.

## Building With Separate Output Directory

Example:

```sh
make O=build CROSS_COMPILE=aarch64-linux-gnu- board_defconfig
make O=build CROSS_COMPILE=aarch64-linux-gnu- -j8
```

List likely outputs:

```sh
find build -maxdepth 3 -type f \( -name 'u-boot*' -o -name 'SPL' -o -name '*spl*' -o -name '*.itb' -o -name '*.bin' -o -name '*.img' \)
```

## Artifact Selection Rule

Do not ask, "Did U-Boot build?"

Ask:

- what does the SoC boot ROM load first?
- which boot media is selected?
- does the board use SPL?
- does the board use vendor firmware before U-Boot?
- does U-Boot proper need a wrapper?
- is the final artifact signed or packaged?
- where is the artifact deployed?

## TI Sitara Example Mental Model

On many TI K3/Sitara platforms, boot involves multiple firmware and bootloader stages. You may see artifacts such as:

```text
tiboot3.bin
tispl.bin
u-boot.img
```

The exact flow depends on SoC family, security type, SDK release, and boot media. Treat the TI SDK documentation and generated deploy directory as the source of truth.

## Common Mistakes

- Flashing `u-boot.bin` when the board expects `u-boot.img`.
- Updating `u-boot.img` but not updating SPL.
- Updating SD boot artifacts while the board boots from eMMC or SPI.
- Ignoring board-specific packaging scripts.
- Deleting output files without knowing which stage produced them.
- Reusing an output directory after switching board defconfigs.

## Debugging Checklist

- Identify board defconfig.
- Build with a clean output directory.
- List all generated artifacts.
- Identify the first-stage boot artifact.
- Identify the U-Boot proper artifact.
- Compare deployed artifact checksums.
- Confirm serial log version string.
- Confirm boot media priority and strap settings.

## Related Topics

- [Board Defconfigs](board-defconfigs.md)
- [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)
- [Cross-Building and Flashing](cross-building-and-flashing.md)
- [BSP Artifact Flow and Provenance](../bsp-integration/artifact-flow-and-provenance.md)

## References

- U-Boot documentation
- U-Boot board documentation
- TI Processor SDK Linux documentation
