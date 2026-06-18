---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Device Tree Builds

## What Problem Does This Solve?

Device tree describes board hardware to the kernel. The kernel build system compiles DTS/DTSI sources into DTBs that the bootloader passes to Linux.

For embedded Linux, device tree build knowledge is essential for board bring-up, peripheral enablement, pinmux, regulators, clocks, Ethernet PHYs, and validating that the board runs the DTB you built.

## Core Concepts

- DTS
- DTSI
- DTB
- DTBO
- device tree compiler
- bindings
- `dtbs` target
- overlays
- `/proc/device-tree`
- kernel DTB vs U-Boot DTB

## Mental Model

Device tree flow:

```text
*.dts + included *.dtsi
-> dtc
-> *.dtb
-> boot partition / FIT image / firmware location
-> U-Boot passes DTB to kernel
-> kernel populates devices
```

Changing DTS source is not enough. You must rebuild, deploy, boot, and validate the runtime tree.

## Syntax / API / Mechanism

Build all DTBs for an architecture:

```sh
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dtbs
```

Build kernel and DTBs:

```sh
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j8 Image dtbs
```

Find DTBs:

```sh
find build -name '*.dtb'
```

Decompile DTB:

```sh
dtc -I dtb -O dts board.dtb > board.dts
```

Inspect runtime device tree:

```sh
tr -d '\0' < /proc/device-tree/model
find /proc/device-tree -name status
```

## Source Layout

Common paths:

```text
arch/arm/boot/dts/
arch/arm64/boot/dts/
arch/riscv/boot/dts/
```

Common source types:

```text
soc.dtsi          -> SoC-level definitions
board.dts         -> board-level hardware
carrier.dtsi      -> carrier board or shared board fragment
overlay.dtso      -> overlay source where used
```

## Device Tree Bindings

Bindings describe valid nodes and properties for hardware.

Modern bindings are often YAML:

```text
Documentation/devicetree/bindings/
```

Use bindings to answer:

- required properties
- optional properties
- compatible strings
- child node structure
- clock/regulator/reset requirements

## Common Scenarios

### DTS Builds But Driver Does Not Probe

Likely causes:

- driver not enabled in kernel config
- wrong `compatible`
- node status is disabled
- missing clocks/regulators/resets
- bus controller disabled
- pinctrl missing
- address/interrupt wrong

Check:

```sh
dmesg | grep -i driver_or_device
find /proc/device-tree -name compatible
```

### Runtime Tree Does Not Match Source

Likely causes:

- old DTB deployed
- U-Boot loads a different DTB path
- FIT image includes another DTB
- overlay not applied
- kernel and U-Boot use separate DTB workflows

Check:

```sh
tr -d '\0' < /proc/device-tree/model
cat /proc/cmdline
```

### Need Both Kernel And U-Boot DTB Changes

U-Boot may use its own DTB for driver model. If a peripheral is needed before Linux boots, U-Boot's DTB may need a matching node too.

Examples:

- MMC boot device
- Ethernet used for TFTP
- I2C EEPROM used in bootloader
- regulators needed before kernel handoff

### Overlay Workflow

Some systems apply overlays at boot. Know where overlays are:

- built
- deployed
- selected
- applied

Debug:

- inspect U-Boot environment
- inspect boot scripts
- inspect runtime `/proc/device-tree`

## Deployment Patterns

Standalone boot partition:

```text
/boot/Image
/boot/board.dtb
```

FIT image:

```text
fitImage contains kernel + one or more DTBs
```

Firmware-managed boot:

```text
platform firmware loads DTB from vendor-specific location
```

Always identify the actual deployment path.

## Common Mistakes

- Editing DTS but not rebuilding DTB.
- Rebuilding DTB but not deploying it.
- Deploying DTB but U-Boot loads another filename.
- Forgetting node `status = "okay";`.
- Changing kernel DTS when U-Boot uses a separate DTS.
- Ignoring binding documentation.
- Debugging driver probe before confirming runtime DTB.
- Treating decompiled DTB as source of truth.

## Debugging Checklist

- Identify source DTS.
- Identify all included DTSI files.
- Build `dtbs`.
- Find generated DTB.
- Decompile generated DTB if needed.
- Confirm deployed DTB checksum.
- Confirm U-Boot load path.
- Confirm runtime `/proc/device-tree`.
- Check kernel config for the driver.
- Check dmesg for probe messages.
- Check binding requirements.

## Related Topics

- [Kernel Source Tree and Outputs](source-tree-and-outputs.md)
- [Cross-Building and Installing](cross-building-and-installing.md)
- [Boot Debugging and Runtime Validation](../bsp-integration/boot-debugging-and-runtime-validation.md)
- [Configuration and Patch Ownership](../bsp-integration/configuration-and-patch-ownership.md)

## References

- Linux kernel device tree documentation
- Devicetree specification
- Linux kernel binding documentation
- Device tree compiler documentation
