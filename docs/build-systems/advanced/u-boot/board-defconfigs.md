---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Board Defconfigs

## What Problem Does This Solve?

The board defconfig is the maintained entry point for a U-Boot board build. It selects the target board, architecture, SPL behavior, commands, drivers, device tree, boot flow, and image format choices.

For embedded Linux work, understanding defconfigs is required before modifying U-Boot behavior in a reproducible way.

## Core Concepts

- `configs/*_defconfig`
- board target
- Kconfig symbol
- `.config`
- `savedefconfig`
- board variant
- vendor baseline
- product defconfig
- SPL-specific config

## Mental Model

Defconfig is the source input. `.config` is the generated build result.

```text
configs/<board>_defconfig
-> make <board>_defconfig
-> .config
-> generated headers
-> selected drivers, commands, and boot artifacts
```

Edit the maintained defconfig or fragments/policies around it. Do not edit generated `.config` as the long-term source of truth.

## Finding Board Defconfigs

List candidates:

```sh
ls configs/*defconfig
```

Search for a vendor or SoC:

```sh
ls configs/*am62*
grep -R "CONFIG_TARGET" configs/*ti*defconfig
```

Build one:

```sh
make O=build CROSS_COMPILE=aarch64-linux-gnu- am62x_evm_a53_defconfig
```

## Inspecting A Defconfig

Example defconfig entries:

```text
CONFIG_ARM=y
CONFIG_ARCH_K3=y
CONFIG_TARGET_AM625_A53_EVM=y
CONFIG_DEFAULT_DEVICE_TREE="k3-am625-sk"
CONFIG_SPL=y
CONFIG_CMD_MMC=y
CONFIG_CMD_NET=y
```

These are requested values. Kconfig still resolves dependencies and defaults into final `.config`.

## Saving Defconfig Changes

Workflow:

```sh
make O=build CROSS_COMPILE=aarch64-linux-gnu- board_defconfig
make O=build CROSS_COMPILE=aarch64-linux-gnu- menuconfig
make O=build CROSS_COMPILE=aarch64-linux-gnu- savedefconfig
cp build/defconfig configs/my_board_defconfig
```

Review the diff before committing.

```sh
diff -u configs/old_defconfig build/defconfig
```

## Defconfig Vs Product Changes

A board defconfig should describe the board bootloader baseline. Product policy should be separate when possible.

Good defconfig content:

- target board
- SoC family
- SPL enablement
- default U-Boot DTB
- storage boot support
- serial console
- essential commands

Questionable defconfig content:

- temporary debug commands
- experimental network boot policy
- product-specific environment defaults
- one-off manufacturing hacks

## Board Variants

Board variants can be handled through:

- separate defconfigs
- common defconfig plus runtime EEPROM detection
- separate U-Boot DTBs
- FIT configurations
- environment selection
- board code

Choose the least surprising mechanism. If two boards need different early boot drivers or DRAM setup, separate defconfigs may be justified.

## Defconfig Upgrade Workflow

When moving to a new vendor U-Boot:

1. Identify old defconfig.
2. Identify new vendor defconfig.
3. Compare both defconfigs.
4. Reapply product changes intentionally.
5. Run `olddefconfig`.
6. Save minimal defconfig.
7. Rebuild all artifacts.
8. Boot and check serial log.

Avoid copying an old full `.config` into a new U-Boot release.

## TI SDK Considerations

TI Processor SDK often provides known board defconfigs for EVMs and SoC families. Use those as the baseline unless your product has a clear reason to fork.

For custom boards:

- start from the closest EVM defconfig
- rename only when you own the board target
- keep TI baseline changes reviewable
- separate product policy from SoC enablement
- track SDK release and defconfig origin

## Common Mistakes

- Editing `.config` and losing the change later.
- Saving a huge non-minimal config.
- Using the wrong EVM defconfig for a custom board.
- Forgetting SPL options are also controlled by config.
- Reusing an output directory from another defconfig.
- Treating debug command enablement as production policy.

## Debugging Checklist

- Confirm exact defconfig name.
- Confirm output `.config`.
- Confirm `CONFIG_DEFAULT_DEVICE_TREE`.
- Confirm `CONFIG_SPL` and related options.
- Confirm selected commands and drivers.
- Run `savedefconfig` and review minimal changes.
- Compare against vendor baseline after SDK upgrades.

## Related Topics

- [Kconfig and Generated Config](kconfig-and-generated-config.md)
- [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)
- [Device Tree in U-Boot](device-tree-in-u-boot.md)
- [Configuration and Patch Ownership](../bsp-integration/configuration-and-patch-ownership.md)

## References

- U-Boot documentation
- U-Boot Kconfig documentation
- TI Processor SDK Linux documentation
