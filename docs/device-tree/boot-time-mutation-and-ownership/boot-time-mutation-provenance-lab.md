---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Boot-Time Mutation Provenance Lab

This lab reconstructs every change TI U-Boot makes to the AM62x SK's Linux DTB between the file on disk and the tree Linux receives. You will predict each mutation from source, capture the final blob from `/sys/firmware/fdt`, and attribute every difference to one function. Sources are mainline Linux (`k3-am625-sk.dts`, `k3-am62x-sk-hdmi-audio.dtso`) and TI U-Boot v2026.01-rc3 (`boot/image-fdt.c`, `boot/fdt_support.c`, `arch/arm/mach-k3/am62x/am625_fdt.c`, `arch/arm/mach-k3/common_fdt.c`, `include/env/ti/*.env`).

## Checkpoints On This Platform

The chapter's timeline names roles, not a fixed order. On the AM62x SK, the default environment produces this sequence:

```text
C0  k3-am625-sk.dtb as built
C1  loaded by get_fdt_mmc to fdtaddr=0x88000000 (no authentication in the default env)
C3  overlays from $name_overlays applied by get_overlay_mmc (fdt resize 0x100000 first)
    booti: FDT relocated with CONFIG_SYS_FDT_PAD (0x3000) extra space
C4  image_setup_libfdt(): fdt_root, fdt_chosen, arch_fixup_fdt, OP-TEE nodes,
    fdt_fixup_ethernet, [ft_board_setup], ft_system_setup, fdt_initrd
C2  the firmware reservations (tfa, optee) are rewritten inside ft_system_setup,
    after the overlays, not before them
C5  shrunk with fdt_shrink_to_minimum() and passed to Linux
C6  Linux live tree; C5 itself is visible as /sys/firmware/fdt
```

Note the reversal: on this platform the “firmware-adjusted” step runs after overlays and board fixups. Record the real order for your build before reasoning about conflicts.

## Inputs

- `k3-am625-sk.dtb` and `k3-am62x-sk-hdmi-audio.dtbo`, built as in the [Overlays lab](../overlays-in-depth/overlay-composition-and-lifecycle-lab.md)
- an AM62x SK booting TI U-Boot from SD with the default environment
- `dtc`, `fdtget`, and `fdtoverlay` on the host

## Step 1: Freeze C0 And C3 On The Host

```sh
sha256sum k3-am625-sk.dtb k3-am62x-sk-hdmi-audio.dtbo
fdtoverlay -i k3-am625-sk.dtb -o c3.dtb k3-am62x-sk-hdmi-audio.dtbo
dtc -I dtb -O dts -s -o c3.dts c3.dtb
```

`-s` sorts nodes and properties, which keeps later diffs readable.

## Step 2: Predict Every Mutation

Read the functions named under C4 and fill the table before booting:

| Function | Writes | Condition | Prediction for your SK |
|---|---|---|---|
| `fdt_root()` | `/serial-number` | env `serial#` set | |
| `fdt_chosen()` | `/chosen/bootargs`, `/chosen/u-boot,version` | always (bootargs if set) | |
| `fdt_chosen()` | `/chosen/kaslr-seed`, `/chosen/rng-seed` | `DM_RNG` / `BOARD_RNG_SEED` enabled | |
| `arch_fixup_fdt()` | `/memory@80000000/reg` | DRAM banks known to U-Boot | |
| `fdt_fixup_ethernet()` | `mac-address` (replace), `local-mac-address` (create) on the `ethernet0` alias target | env `ethaddr` set | |
| `fdt_fixup_cores_nodes_am625()` | deletes `/cpus/cpu@N`, `cpu-map` cores, and per-core watchdogs | fewer than 4 cores | |
| `fdt_fixup_gpu_nodes_am625()` | deletes `gpu@fd00000` and its watchdog | no GPU in this part | |
| `fdt_fixup_pru_node_am625()` | deletes `pruss@30040000` | no PRU | |
| `fdt_fixup_thermal_critical_trips_k3()` | every `critical` trip `temperature` | always: 95, 105, or 125 °C by temperature grade | |
| `fdt_fixup_thermal_cooling_device_cpus_am625()` | `cooling-device` in both zones | always, truncated to 3 cells per remaining core | |
| `fdt_fixup_reserved()` | deletes and re-adds `tfa@...`, `optee@...` | always | |
| `fdt_initrd()` | `/chosen/linux,initrd-*` | initrd given | |

Use `k3_get_max_temp()` in `am62_hardware.h` to read the temperature grade mapping: commercial and unknown give 95, industrial 105, automotive 125.

## Step 3: Capture C5 And C6 On The Board

```sh
cp /sys/firmware/fdt c5.dtb
sha256sum c5.dtb
dtc -I dtb -O dts -s -o c5.dts c5.dtb 2>/dev/null
```

Copy `c5.dtb` to the host. `/sys/firmware/fdt` is the blob as Linux received it; `/sys/firmware/devicetree/base` is the C6 view after unflattening.

## Step 4: Attribute Every Difference

```sh
diff -u c3.dts c5.dts
```

Expected classes of difference, each to be mapped to one row of your Step 2 table:

- `/chosen` gains `bootargs` and `u-boot,version`
- `cpsw_port1`'s all-zero `mac-address` becomes the board's address and `local-mac-address` appears, if `ethaddr` was set
- `critical` trip temperatures under both thermal zones change from the DTS value (105000) to the grade value
- the `tfa` and `optee` reservations are re-created; compare their addresses with your build's `CONFIG_K3_ATF_LOAD_ADDR` and `CONFIG_K3_OPTEE_LOAD_ADDR` and explain any change
- on a 4-core, GPU, PRU part, no nodes are deleted

Any difference you cannot attribute is a finding, not noise.

## Step 5: Check Cross-Stage Consistency

1. Compare the passive trip (`main0-alert`, 95 °C in the DTS) with the rewritten critical trip on a commercial-grade part. If both are 95 °C, explain what that leaves of the passive cooling range and who owns the decision: the DTS author, the U-Boot fixup, or the product.
2. Explain why `fdt_fixup_thermal_cooling_device_cpus_am625()` must run whenever CPU nodes are deleted, and what would remain in `cooling-device` otherwise.
3. Compare `/memory@80000000/reg` in C3 and C5 and decide whether U-Boot or the DTS is the authority on this board.

## Step 6: Check Capacity And Failure Behavior

1. From `fdt header` at the U-Boot prompt after `get_overlay_mmc`, record `totalsize` and `size_dt_struct`. Compare the free space with the growth you measured in Step 4.
2. Read the error handling in `image_setup_libfdt()` and the functions it calls. Which failures reach `- must RESET the board to recover.`? Note that `fdt_chosen()` prints `WARNING: could not set bootargs` and then returns the error, so that “warning” is fatal, while `ft_system_setup()` for AM62x returns 0 even when an individual fixup fails.
3. Find the environment variable that skips `ft_board_setup()` (`skip_board_fixup`) and decide whether a product should allow it.

## Step 7: Diagnose Deliberate Faults

### Fault A: A Missing Overlay File

Set `name_overlays` to a file that does not exist. Predict that `get_overlay_mmc` skips it (`load ... && fdt apply ...`) and the boot continues; C5 equals the no-overlay tree plus C4 fixups. Decide where the boot should have failed.

### Fault B: An Overlay That Re-Enables The Analog Card

Write a scratch overlay that sets `&codec_audio { status = "okay"; };` and list it after the HDMI audio overlay. Predict the C3 result (both cards enabled, both using `mcasp1`) and the runtime symptom. Explain why ordering alone decided the result.

### Fault C: A Single-Core Part

Without hardware, simulate `fdt_fixup_cores_nodes_am625(blob, 1)` on a copy of C3 with `fdtput -r` for `cpu@1`–`cpu@3`, their `cpu-map` cores, and watchdogs `e010000`–`e030000`. Run `dtc` and note the dangling `cooling-device` phandles:

```text
Warning (cooling_device_property): /thermal-zones/main0-thermal/cooling-maps/map0:cooling-device: Could not get phandle node for (cell 3)
Warning (cooling_device_property): /thermal-zones/main1-thermal/cooling-maps/map0:cooling-device: Could not get phandle node for (cell 3)
```

Then truncate each `cooling-device` to its first three cells and confirm the warnings disappear. This shows why the two fixups are a pair.

### Fault D: A Wrong Temperature Grade

Explain the consequence if `k3_get_temp_grade()` misread an automotive part as commercial: critical shutdown at 95 °C instead of 125 °C. Then the reverse case. Decide which error is the safety problem.

## Exit Review

The lab is complete when you can present:

- the platform-specific checkpoint order with the C2/C3 reversal explained
- the prediction table and the C3-to-C5 diff with every line attributed
- the cross-stage consistency findings, including the thermal trip interaction
- capacity measurements and the abort-versus-warn classification
- predicted and observed results for each fault

## Authoritative References

- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Upstream `/chosen` schema](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/chosen.yaml)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux reserved-memory binding](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt)

## Continue

Proceed to [Build And Diagnostic Tools](../build-and-diagnostic-tools.md).
