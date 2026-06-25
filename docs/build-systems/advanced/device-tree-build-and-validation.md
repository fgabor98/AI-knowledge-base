---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Device Tree Build And Validation

## What Problem Does This Solve?

Device tree is where hardware description, kernel drivers, U-Boot behavior, pinctrl, regulators, clocks, reserved memory, and remote processors meet. Platform engineers need a standalone workflow for building, validating, deploying, and proving DTBs.

## Core Concepts

- DTS
- DTSI
- DTB
- overlays
- bindings
- schema validation
- pinctrl
- regulators
- clocks
- reserved memory
- aliases
- chosen node

## Build Flow

```text
DTS/DTSI source
-> cpp-style includes
-> dtc
-> DTB
-> boot partition or FIT
-> kernel unflattens tree
-> drivers probe
```

## Validation Layers

Validate at multiple layers:

- source review for include structure
- `dtc` warnings
- YAML binding schema checks
- deployed DTB checksum
- runtime `/proc/device-tree`
- driver probe logs in `dmesg`

## Runtime Checks

```bash
strings /proc/device-tree/model
find /proc/device-tree -maxdepth 2 -type d | sort | head
dmesg | grep -i 'OF:'
dmesg | grep -i regulator
dmesg | grep -i pinctrl
```

For a copied DTB:

```bash
dtc -I dtb -O dts board.dtb > board.decoded.dts
```

## Common Board-Porting Areas

- serial console
- boot media
- Ethernet PHY reset and delays
- fixed regulators
- PMIC regulators
- GPIO polarity
- I2C/SPI peripherals
- pinmux groups
- reserved memory for firmware
- remoteproc nodes

## Common Mistakes

- editing Linux DTS while U-Boot uses a different DTB
- deploying the right DTB to the wrong partition
- disabling warnings instead of fixing bindings
- copying EVM regulator assumptions into a custom board
- forgetting aliases that userspace or boot scripts depend on
- treating pinmux as peripheral documentation instead of executable hardware policy

## Related Topics

- [Linux Kernel Device Tree Builds](linux-kernel/device-tree-builds.md)
- [Device Tree Binding Validation](linux-kernel/device-tree-binding-validation.md)
- [Custom Sitara Board Bring-Up](ti-processor-sdk/custom-sitara-board-bring-up.md)

