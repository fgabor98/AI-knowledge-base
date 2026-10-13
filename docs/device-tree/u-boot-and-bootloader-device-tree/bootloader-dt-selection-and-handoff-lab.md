---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Bootloader DT Selection And Handoff Lab

This lab traces how one TI U-Boot build boots two different boards, the AM642 EVM and the AM642 SK, from ROM to Linux. Board identity comes from an I²C EEPROM and is consumed four times: by the R5 SPL to pick its own DTB and DRAM configuration, by the R5 SPL again to pick the A53 SPL's DTB from `tispl.bin`, by the A53 SPL to pick U-Boot's DTB from `u-boot.img`, and by U-Boot to pick the Linux DTB file. Sources are TI U-Boot (`ti-u-boot`, v2026.01-rc3) and mainline Linux.

## Objectives

By the end, you should be able to:

- list every DT-shaped artifact in the two-board image and its consumer
- trace the SPL dependency closure needed to read the board EEPROM
- prove the R5 SPL SRAM budget from configuration
- follow each of the four selection points, including its fallback
- plan the DRAM addresses used by the default environment
- predict the failure signature when identity is missing, wrong, or overridden

## Inputs

- TI U-Boot: `configs/am64x_evm_r5_defconfig`, `configs/am64x_evm_a53_defconfig`, `arch/arm/dts/k3-am64x-binman.dtsi`, `arch/arm/dts/k3-am642-r5-{evm,sk}.dts`, `board/ti/am64x/evm.c`, `board/ti/common/board_detect.c`, `arch/arm/mach-k3/am64x/am642_init.c`, `include/env/ti/{ti_common,mmc}.env`, `board/ti/am64x/am64x.env`
- Linux: `k3-am642-evm.dts`, `k3-am642-sk.dts`
- for Steps 7–9, an AM642 SK and optionally an AM642 EVM, both with serial console

## Step 1: Create The Artifact Ledger

Build both configurations following TI's documentation (R5 first, then A53 with TF-A and OP-TEE). Then list every DT-shaped artifact:

| Artifact | Built from | Packaged in | Consumer |
|---|---|---|---|
| R5 SPL DTBs (EVM, SK) | `k3-am642-r5-evm.dts`, `k3-am642-r5-sk.dts` | multi-DTB FIT inside the R5 SPL | R5 SPL |
| A53 SPL DTBs | `spl/dts/ti/k3-am642-{evm,sk}.dtb` | `tispl.bin` `fdt-0`, `fdt-1` | R5 SPL selects, A53 SPL uses |
| U-Boot DTBs | `u-boot.dtb` (EVM) and `dts/upstream/src/arm64/ti/k3-am642-sk.dtb` | `u-boot.img` `fdt-0`, `fdt-1` | A53 SPL selects, U-Boot uses |
| Linux DTBs | Linux `ti/k3-am642-{evm,sk}.dtb` | root filesystem `/boot/dtb/ti/` | U-Boot loads, Linux uses |
| the binman description | `k3-am64x-binman.dtsi` | none (build input) | binman |

Record hashes for each, and run `mkimage -l tispl.bin` and `mkimage -l u-boot.img` to confirm two configurations in each, with descriptions `k3-am642-evm` and `k3-am642-sk`.

## Step 2: Audit The SPL Dependency Closure

The R5 SPL must read the EEPROM before it knows the board. Using the R5 SPL's EVM DTB (the one it starts on), confirm that these nodes survive phase filtering, and find where each `bootph-*` comes from (Linux DTS or `-u-boot.dtsi`):

```text
main_i2c0 (bootph-all in Linux k3-am642-{evm,sk}.dts)
  -> main_i2c0_pins_default, main_pmx0
  -> its clock and power domain providers (k3_clks, k3_pds via dmsc)
  -> eeprom@50 (EVM) / eeprom@51 (SK): not needed; board_detect.c talks to the address directly
```

Then compare `main_i2c0` in the two Linux files. Both use pads `0x0260`/`0x0264` (I2C0_SCL/SDA, `PIN_INPUT_PULLUP`) at 400 kHz. Explain why that sameness is what makes it safe to start on the EVM tree.

## Step 3: Prove The SRAM Budget

From `am64x_evm_r5_defconfig`:

```text
CONFIG_SPL_TEXT_BASE=0x70000000
CONFIG_SPL_MAX_SIZE=0x180000
CONFIG_SPL_SIZE_LIMIT=0x190000
CONFIG_SPL_SIZE_LIMIT_PROVIDE_STACK=0x4000
CONFIG_SPL_BSS_START_ADDR=0x7019b800
CONFIG_SPL_BSS_MAX_SIZE=0x4000
```

Draw the interval map, and add `CONFIG_K3_ATF_LOAD_ADDR=0x701c0000` from the A53 defconfig. Then measure: `size spl/u-boot-spl`, the size of `spl/u-boot-spl.bin` with its appended multi-DTB FIT, and compare against both limits. Explain why adding a third board to `CONFIG_SPL_OF_LIST` costs SRAM even if that board never boots.

## Step 4: Fill The Selection Table

For each row, name the function, the input, and the result:

| Selection point | Code | EVM | SK | SK with blank EEPROM |
|---|---|---|---|---|
| R5 SPL's own DTB | `do_dt_magic()` → `fdtdec_resetup()` | | | |
| A53 SPL DTB in `tispl.bin` | `fit_find_config_node()` → `board_fit_config_name_match()` | | | |
| U-Boot DTB in `u-boot.img` | same, in the A53 SPL | | | |
| Linux DTB file | `setup_board_eeprom_env()` → `ti_set_fdt_env()` | | | |

Read `board_detect.c` to see that the EEPROM is tried at `0x50`, then `0x51`; the EVM's is at `0x50` and the SK's at `0x51`. For the blank-EEPROM column, follow every fallback: `board_fit_config_name_match()` accepts the EVM name when the EEPROM was not read, `fit_find_config_node()` falls back to `default = "conf-0"`, and `ti_set_fdt_env()` falls back to `CONFIG_DEFAULT_DEVICE_TREE`.

## Step 5: Inspect FIT Pairing

In the binman source, confirm that `conf-0` pairs `fdt-0` (EVM) and `conf-1` pairs `fdt-1` (SK) in both FITs, and that configurations are matched by `description`. Explain what would happen if a developer renamed `conf-1`'s description to `k3-am642-sk-rev2` without changing `board_fit_config_name_match()`.

## Step 6: Plan DRAM Addresses

From `ti_common.env` and `am64x.env`:

```text
loadaddr / kernel_addr_r  0x82000000
fdtaddr / fdt_addr_r      0x88000000
rdaddr / ramdisk_addr_r   0x88080000
dtboaddr                  0x89000000
addr_fit                  0x90000000
rd_spec                   -          (no initrd by default)
```

`get_overlay_mmc` runs `fdt resize 0x100000` on the DTB at `fdtaddr`. Compute the largest base DTB plus overlays that fits before it reaches `rdaddr`, and decide whether that matters with the default `rd_spec=-` and with an initrd loaded. Also compute the largest `Image` that fits below `fdtaddr`.

## Step 7: Execute And Capture Selection

On the SK, capture the full serial log from power-on. Expected markers include:

```text
EEPROM not available at 0x50, trying to read at 0x51
```

At the U-Boot prompt:

```text
=> printenv board_name fdtfile
=> fdt addr
=> fdt addr -c
=> fdt print -c /model
```

Expected: `board_name=am64x_skevm`, `fdtfile=ti/k3-am642-sk.dtb`, and a control-FDT model of `Texas Instruments AM642 SK`. Repeat on the EVM if available.

## Step 8: Apply An Overlay

On the EVM, set `name_overlays` to `ti/k3-am642-evm-icssg1-dualemac.dtbo` and boot through the default `bootcmd`. Confirm that `get_overlay_mmc` loaded and applied it (`fdt print /icssg1-eth` before `booti`, if you stop there). Then set it to a file that does not exist, and note that the boot continues without any error that stops it.

## Step 9: Confirm Linux Handoff

```sh
cat /proc/device-tree/model
tr '\0' '\n' < /proc/device-tree/compatible
dmesg | grep -i 'machine model'
```

Compare with the U-Boot `fdtfile` and control-FDT model. Explain why the control model and the Linux model come from different DTB files that happen to agree.

## Step 10: Diagnose Deliberate Faults

### Fault A: The SK's EEPROM Cannot Be Read

Predict the chain from Step 4's last column. In particular, the R5 SPL stays on `k3-am642-r5-evm.dts`, which includes `k3-am64-evm-ddr4-1600MTs.dtsi`, while the SK has LPDDR4 (`k3-am64-sk-lp4-1600MTs.dtsi`). Expect DDR initialization to fail before any later stage runs. Explain why “safe default” is board-specific: falling back to the EVM is harmless on an EVM and fatal on an SK.

### Fault B: An Unknown Board Name

Program (or simulate) a name that is neither EVM nor SK. Predict `Unidentified board claims ... in eeprom header` from U-Boot proper, and work out which DTB each earlier stage chose.

### Fault C: `bootph-all` Removed From `main_i2c0`

In a scratch Linux tree used by the U-Boot build, delete `bootph-all` from `&main_i2c0`. Predict that the SPL DTB no longer contains the controller and the EEPROM read fails. On an SK the result matches Fault A, so U-Boot proper never runs. On an EVM the fallback happens to be right, and U-Boot proper, which probes the full control tree after relocation, reads the EEPROM normally. Explain why a fault that is invisible on one board is fatal on the other.

### Fault D: A Saved `fdtfile` Override

At the U-Boot prompt on the SK, run `setenv fdtfile ti/k3-am642-evm.dtb; saveenv` and reboot. Predict that `fdtfile` is back to `ti/k3-am642-sk.dtb`, because `board_late_init()` calls `setup_board_eeprom_env()` after the environment is loaded and sets it unconditionally. Decide where a deliberate override should live instead.

### Fault E: A Renamed FIT Description

Using Step 5, rebuild with `conf-1`'s description changed. Predict which stage first picks the EVM tree on an SK and what the log shows.

### Fault F: A Large Overlay With An Initrd

Using Step 6, construct the base-plus-overlay size and initrd placement at which the expanded working FDT would overlap the ramdisk at `rdaddr`. Explain which of the two would be corrupted, and why the failure would appear only on some boots.

## Exit Review

The lab is complete when you can present:

- the artifact ledger with hashes and FIT configuration listings
- the SPL dependency closure for the EEPROM read, with the source of each `bootph-*`
- the SRAM interval map and measured sizes
- the completed four-point selection table, including every fallback
- the DRAM address plan and its two limits
- serial logs from at least one board showing selection
- the overlay test, including the missing-file behavior
- Linux handoff evidence
- a predicted and, where safe, observed result for each fault

## Authoritative References

- [U-Boot Devicetree Control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot generic SPL framework](https://docs.u-boot.org/en/latest/develop/spl.html)
- [U-Boot FIT signature verification](https://docs.u-boot.org/en/latest/usage/fit/signature.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [U-Boot binman documentation](https://docs.u-boot.org/en/latest/develop/package/binman.html)

## Continue

Proceed to [Boot-Time Mutation And Ownership](../boot-time-mutation-and-ownership.md).
