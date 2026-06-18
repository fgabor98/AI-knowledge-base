---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Driver Model and Pre-Relocation

## What Problem Does This Solve?

U-Boot driver model controls how devices are discovered, bound, probed, and used. Pre-relocation support controls which devices are available before U-Boot moves itself to its final runtime location.

For embedded SoC bring-up, this explains why a driver can be compiled but unavailable when SPL or early U-Boot needs it.

## Core Concepts

- driver model
- uclass
- bind
- probe
- device tree node
- compatible string
- pre-relocation
- SPL driver model
- clocks
- resets
- pinctrl
- regulators
- `dm tree`

## Mental Model

Driver availability requires several conditions:

```text
Kconfig enables driver
-> device tree contains matching node
-> driver binds to node
-> dependencies are available
-> driver probes successfully
-> command or boot flow uses device
```

If any step fails, the source file can exist and compile while the hardware remains unusable.

## Bind Vs Probe

Binding creates a U-Boot device object from a driver and description.

Probing initializes the hardware.

A device can be bound but not probed. That usually means U-Boot knows the device exists but has not initialized it yet, or probing failed.

## Uclasses

U-Boot groups devices into uclasses:

- serial
- MMC
- Ethernet
- SPI
- I2C
- GPIO
- pinctrl
- clock
- reset
- regulator

Uclasses provide common APIs used by commands and boot code.

## Inspecting Driver Model

At U-Boot prompt, if commands are enabled:

```text
dm tree
dm uclass
dm drivers
```

Interpretation:

- device absent: config or DTB problem
- device listed but not active: probe not requested or failed
- dependency missing: clock, reset, pinctrl, regulator, parent bus issue

## Device Tree Dependency

A driver typically needs:

- matching `compatible`
- correct `reg`
- clocks
- resets
- pinctrl
- power domains
- bus parent
- status enabled

Example class of failure:

```text
CONFIG_DM_MMC=y
```

but the MMC node is missing or disabled in U-Boot DTB. The driver is built, but no device appears.

## Pre-Relocation

Before relocation, U-Boot runs with limited memory and limited initialized hardware.

Devices needed early may include:

- serial console
- boot storage
- clocks
- pinctrl
- reset controller
- PMIC/regulators
- watchdog

These devices may need U-Boot-specific pre-relocation markings in the DTB, depending on U-Boot version and stage.

## SPL Driver Model

SPL may use driver model too, but it is constrained by size and memory.

SPL needs only the devices required to:

- print early logs
- initialize memory
- access boot media
- load next stage
- authenticate next stage if secure boot is active

Do not enable full U-Boot driver coverage in SPL unless needed.

## Dependency Chains

An MMC device may depend on:

```text
clock controller
-> reset controller
-> pinctrl
-> regulator
-> MMC controller
-> filesystem loader
```

If any earlier dependency is missing in SPL, loading U-Boot proper from MMC may fail.

## Built But Not Probed

Debug sequence:

1. Confirm final `.config`.
2. Confirm SPL-specific config if relevant.
3. Confirm U-Boot DTB node.
4. Confirm `status = "okay"`.
5. Confirm compatible string.
6. Confirm parent bus.
7. Confirm clocks/resets/regulators.
8. Check `dm tree`.
9. Check serial logs for probe failure.

## Pre-Relocation Debugging

Symptoms:

- no early serial
- SPL cannot read boot media
- U-Boot proper can use device but SPL cannot
- device works after prompt but not during autoboot
- random failures before relocation

Checks:

- pre-relocation DT properties
- SPL config symbols
- SPL DTB contents
- SPL size after enabling dependencies
- parent bus availability
- clock/reset/pinctrl availability

## DTS Minimization For SPL

Some platforms use a reduced DTB for SPL to save space.

Be careful:

- removing unused Linux nodes is fine
- removing clock/reset parents can break needed devices
- removing aliases can break boot code
- removing pinctrl can break storage

Always validate SPL boot after DTB minimization.

## TI Sitara Considerations

For TI platforms, early boot often depends on:

- UART
- MMC/eMMC or OSPI
- pinctrl
- clocks
- power domains
- system firmware interfaces
- board-specific boot media wiring

When a TI EVM works but a custom board does not, compare both U-Boot DTBs and SPL-relevant nodes before modifying driver code.

## Common Mistakes

- Assuming built driver means usable device.
- Checking Linux DTS instead of U-Boot DTB.
- Enabling U-Boot proper driver but not SPL driver.
- Forgetting parent bus or clock/reset dependency.
- Removing required nodes from SPL DTB to save size.
- Debugging command behavior without checking `dm tree`.

## Debugging Checklist

- Confirm driver Kconfig.
- Confirm SPL/TPL variant config.
- Confirm U-Boot DTB node.
- Confirm compatible string.
- Confirm dependencies.
- Run `dm tree` if available.
- Check logs for probe failures.
- Confirm pre-relocation markings.
- Confirm SPL DTB contents.
- Compare with vendor EVM.

## Related Topics

- [Device Tree in U-Boot](device-tree-in-u-boot.md)
- [Kconfig and Generated Config](kconfig-and-generated-config.md)
- [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)
- [Board Porting and Bring-Up](board-porting-and-bring-up.md)

## References

- U-Boot driver model documentation
- U-Boot device tree documentation
- U-Boot SPL documentation
