---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Debugging U-Boot Builds

## What Problem Does This Solve?

U-Boot failures often sit between build system, boot ROM, flashing layout, SPL, U-Boot proper, environment, device tree, and Linux handoff. Effective debugging starts by locating the failing layer.

For embedded Linux work, this prevents wasting time debugging Linux when the board is actually booting an old U-Boot or wrong DTB.

## Core Concepts

- build failure
- wrong artifact
- SPL failure
- U-Boot proper failure
- environment override
- device tree mismatch
- boot media mismatch
- FIT selection
- serial log
- recovery path

## Mental Model

Classify failures:

```text
build/config failure
deployment/flashing failure
boot ROM/SPL failure
U-Boot proper failure
Linux handoff failure
```

Each class has different first checks.

## Build Verbosity

Use verbose build output:

```sh
make O=build CROSS_COMPILE=aarch64-linux-gnu- V=1
```

Useful when checking:

- compiler used
- generated tools
- image packaging commands
- DTB compilation
- SPL link steps
- `mkimage` invocation

## Failure-Mode Matrix

| Symptom | Likely Layer | First Checks |
| --- | --- | --- |
| source changed but output unchanged | build directory/config | `O=`, defconfig, final `.config`, rebuild target |
| command missing at prompt | Kconfig | `CONFIG_CMD_*`, SPL vs U-Boot proper, saved environment |
| no serial output | boot ROM/first stage | boot media, flashed artifact, UART pins, first-stage image |
| SPL banner only | SPL handoff | DRAM init, next-stage load path, SPL config, artifact location |
| U-Boot banner old | deployment | boot media priority, eMMC boot0, SPI flash, SD card contents |
| Linux gets wrong DTB | boot flow/FIT | `printenv`, FIT config, loaded DTB path, `/proc/device-tree` |
| new bootcmd ignored | environment | persistent env, `env default -a`, env storage config |
| image signature failure | verified boot | keys, hashes, FIT signatures, modified artifacts |

## Wrong Artifact Debugging

Check generated artifacts:

```sh
find build -maxdepth 3 -type f \( -name 'u-boot*' -o -name '*.itb' -o -name '*.img' -o -name '*.bin' \)
```

Check deployed artifacts:

```sh
sha256sum build/u-boot.img
sha256sum /media/boot/u-boot.img
```

For raw flash/eMMC, use board-specific readback commands where available.

## Stale Build Directory

Symptoms:

- old board name in build output
- unexpected config symbols
- wrong DTB selected
- objects from previous defconfig

Fix:

```sh
rm -rf build
make O=build CROSS_COMPILE=aarch64-linux-gnu- board_defconfig
make O=build CROSS_COMPILE=aarch64-linux-gnu- -j8
```

Use caution with deletion, but do not trust an output directory that has switched boards repeatedly.

## Environment Overrides

Persistent environment can make a new U-Boot behave like the old one.

At prompt:

```text
printenv
version
bdinfo
```

If appropriate:

```text
env default -a
saveenv
```

Do this only when you understand environment storage and recovery implications.

## SPL Size And Link Failures

If SPL is too large:

- inspect build error
- check map file
- disable unnecessary SPL features
- remove debug logging
- minimize SPL DTB
- compare with vendor baseline

If SPL boots but cannot load U-Boot proper:

- check boot media driver in SPL
- check filesystem/raw offset support
- check artifact filename
- check load address
- check serial log

## Device Tree Debugging

For U-Boot driver model:

- config must enable driver
- DTB must contain node
- compatible must match
- clock/reset/regulator dependencies must be usable
- pre-relocation devices need early properties

At U-Boot prompt, useful commands may include:

```text
dm tree
fdt addr ${fdt_addr_r}
fdt print
```

Command availability depends on configuration.

## Linux Handoff Debugging

If U-Boot works but Linux fails:

- inspect `bootargs`
- inspect loaded kernel address
- inspect loaded DTB address
- inspect FIT selected config
- check initramfs address if used
- compare runtime `/proc/cmdline`
- compare runtime `/proc/device-tree`

Do not assume Linux receives the DTB file you edited.

## TI Sitara Debugging Pattern

For TI SDK platforms, collect:

- SDK release
- board and SoC
- security variant
- boot media
- artifact names and checksums
- serial log from reset
- U-Boot environment
- deploy directory listing
- kernel/DTB artifact names

Stage-specific serial output is especially important because the failure may happen before U-Boot proper.

## Common Mistakes

- Debugging a newly built artifact that was never flashed.
- Ignoring serial logs from SPL.
- Resetting environment without knowing where it is stored.
- Assuming SD card boot when eMMC/SPI has priority.
- Editing Linux DTS instead of U-Boot DTS.
- Treating signature errors as generic boot failures.
- Mixing artifacts from different SDK releases.

## Debugging Checklist

- Classify the failure layer.
- Confirm build defconfig.
- Confirm final `.config`.
- Confirm generated artifacts.
- Confirm flashed artifacts and locations.
- Confirm serial version string.
- Confirm persistent environment.
- Confirm SPL handoff path.
- Confirm U-Boot DTB.
- Confirm FIT or boot script selection.
- Confirm Linux handoff artifacts.
- Preserve full serial log from reset.

## Related Topics

- [Kconfig and Generated Config](kconfig-and-generated-config.md)
- [SPL, TPL, and U-Boot Proper](spl-tpl-and-u-boot-proper.md)
- [Device Tree in U-Boot](device-tree-in-u-boot.md)
- [Cross-Building and Flashing](cross-building-and-flashing.md)
- [Boot Debugging and Runtime Validation](../bsp-integration/boot-debugging-and-runtime-validation.md)

## References

- U-Boot documentation
- U-Boot driver model documentation
- U-Boot FIT documentation
- TI Processor SDK Linux documentation
