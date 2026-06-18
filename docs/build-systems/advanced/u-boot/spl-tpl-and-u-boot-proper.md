---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# SPL, TPL, and U-Boot Proper

## What Problem Does This Solve?

Many SoCs cannot load full U-Boot directly from reset. They need smaller early stages to initialize clocks, pinmux, memory, or storage before loading U-Boot proper.

For embedded Linux work, understanding SPL and TPL is required when a board fails before the normal U-Boot prompt appears.

## Core Concepts

- boot ROM
- TPL
- SPL
- U-Boot proper
- SRAM constraint
- DRAM initialization
- boot media loader
- handoff
- size limit
- stage-specific config

## Mental Model

Typical staged boot:

```text
SoC boot ROM
-> TPL or vendor first stage, if used
-> SPL
-> U-Boot proper
-> Linux kernel
```

Each stage has different responsibilities and constraints.

## Why SPL Exists

SPL is a small loader. It usually runs before full DRAM and before the complete U-Boot feature set is available.

Responsibilities may include:

- early UART
- clocks
- pinmux
- PMIC setup
- DRAM initialization
- loading U-Boot proper
- authentication handoff
- selecting boot media

SPL usually cannot include every command or driver because it has size and memory limits.

## Why TPL Exists

TPL is an even smaller stage used on some platforms before SPL. It may handle very early initialization that SPL cannot handle within boot ROM constraints.

Not every board uses TPL.

## U-Boot Proper

U-Boot proper is the full bootloader stage that normally provides:

- interactive shell
- boot commands
- environment handling
- filesystem support
- network boot
- FIT loading
- Linux boot handoff

When people say "U-Boot," they often mean U-Boot proper, but early boot failures may happen long before it runs.

## Stage-Specific Outputs

Common outputs:

```text
spl/u-boot-spl
spl/u-boot-spl.bin
tpl/u-boot-tpl.bin
u-boot
u-boot.bin
u-boot.img
u-boot.itb
```

Vendor platforms may package these into named artifacts.

## Stage-Specific Config

SPL has separate config symbols:

```text
CONFIG_SPL=y
CONFIG_SPL_SERIAL=y
CONFIG_SPL_MMC=y
CONFIG_SPL_SPI_FLASH_SUPPORT=y
CONFIG_SPL_OF_CONTROL=y
CONFIG_SPL_DM=y
```

U-Boot proper symbols do not automatically imply SPL support.

## SPL Size Problems

SPL may fail to link or boot if it exceeds size limits.

Symptoms:

- build reports image too large
- board resets silently
- boot ROM rejects image
- SPL log stops early

Responses:

- disable nonessential SPL commands/drivers
- check compiler optimization
- check debug logging
- use smaller device tree
- inspect map file
- compare against vendor baseline

## DRAM Initialization Boundary

Before DRAM works, SPL may run from internal SRAM. That affects:

- available memory
- stack size
- heap availability
- logging
- driver selection
- device tree size

If DRAM init is wrong, U-Boot proper may never load.

## Boot Media Loader

SPL must know how to load the next stage from the selected boot media.

Examples:

- MMC/eMMC/SD
- SPI NOR
- NAND
- UART boot
- USB DFU
- network on some platforms

For MMC boot, SPL may need:

```text
CONFIG_SPL_MMC=y
CONFIG_SPL_FS_FAT=y
CONFIG_SPL_LOAD_FIT=y
```

The exact symbols depend on U-Boot version and board.

## TI Sitara/K3 Considerations

TI K3 platforms often involve multiple firmware components and named artifacts. Depending on device and security mode, the chain can include ROM, system firmware, SPL-like stages, and U-Boot proper packaging.

Practical rules:

- use the TI SDK artifact names as the flashing contract
- do not assume upstream generic artifact names are enough
- keep firmware, SPL, and U-Boot proper from the same build or SDK release
- check secure vs non-secure device documentation
- verify serial logs for each stage

## Expansion: SPL Size And Map Analysis

When SPL grows too large, the fix should be deliberate. Do not randomly disable features without understanding what SPL must do.

Inspect:

- build error size limit
- SPL map file
- enabled `CONFIG_SPL_*` symbols
- SPL DTB size
- debug logging options
- filesystem loaders
- crypto/signing support
- unused commands or drivers

Typical pressure sources:

- enabling too many storage drivers
- enabling network in SPL
- large DTB
- debug logs
- verified boot support
- filesystem support instead of raw loading

For custom boards, compare SPL size and config against the vendor EVM before deciding what changed.

## Expansion: DRAM Bring-Up Boundary

DRAM is often the point where board-specific hardware diverges from the EVM. If SPL starts but U-Boot proper never appears, suspect:

- DDR topology mismatch
- wrong timing/training data
- PMIC voltage issue
- reset sequencing issue
- clock configuration issue
- board layout difference

Treat DRAM changes as board-porting work, not ordinary config changes. Keep them isolated, reviewed, and validated with full serial logs.

## Debugging Stage Boundaries

Ask where the failure occurs:

```text
no serial output
-> boot ROM did not load first stage, UART not initialized, or wrong boot media

SPL banner only
-> SPL runs but cannot initialize DRAM or load U-Boot proper

U-Boot banner appears
-> U-Boot proper runs; debug environment, boot command, kernel artifacts
```

## Common Mistakes

- Enabling a driver for U-Boot proper but not SPL.
- Making SPL too large with debug features.
- Flashing U-Boot proper but not SPL.
- Mixing SPL from one build with U-Boot proper from another.
- Assuming a board reaches U-Boot proper when only SPL is running.
- Ignoring boot ROM image format requirements.

## Debugging Checklist

- Confirm whether board uses SPL/TPL.
- Confirm generated SPL/TPL artifacts.
- Confirm first-stage artifact flashed to the correct location.
- Confirm SPL size.
- Confirm SPL config symbols.
- Confirm DRAM init path.
- Confirm SPL can load U-Boot proper from selected media.
- Confirm serial logs identify each stage.

## Related Topics

- [Source Tree and Outputs](source-tree-and-outputs.md)
- [Kconfig and Generated Config](kconfig-and-generated-config.md)
- [Cross-Building and Flashing](cross-building-and-flashing.md)
- [Board Porting and Bring-Up](board-porting-and-bring-up.md)
- [Driver Model and Pre-Relocation](driver-model-and-pre-relocation.md)
- [Boot Debugging and Runtime Validation](../bsp-integration/boot-debugging-and-runtime-validation.md)

## References

- U-Boot SPL documentation
- U-Boot board documentation
- TI Processor SDK Linux documentation
