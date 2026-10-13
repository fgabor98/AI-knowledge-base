---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Custom Board Porting Capstone Lab

This capstone ports a real custom board, BeagleBoard.org's BeaglePlay, starting from the board it most resembles, TI's AM625 SK. Both use the AM625 and 2 GiB of RAM, so a copied SK tree gets surprisingly far. You will stage the port, find the earliest divergence in each failing chain, and compare your result with the upstream `k3-am625-beagleplay.dts`, which is the answer key. Every hardware fact below comes from the two mainline DTS files and from TI and BeagleBoard U-Boot sources.

## The Two Boards

| Area | AM625 SK (reference) | BeaglePlay (target) |
|---|---|---|
| root | `ti,am625-sk`, `ti,am625` | `beagle,am625-beagleplay`, `ti,am625` |
| console | `main_uart0` as `serial2` | same UART, pins, and alias |
| eMMC | `sdhci0`, internal pull-ups on DAT1–7 | `sdhci0`, own pin group |
| SD power (`vmmc`) | `vdd_mmc1`, enabled by line 3 of TCA6424 `exp1` at `0x22` on `main_i2c1` | `vdd_3v3_sd`, enabled by `main_gpio1` 19; no expander |
| SD I/O (`vqmmc`) | `tlv71033`, switched by `main_gpio0` 31 | `sd_hs200_switch`, switched by `main_gpio1` 49, fed by PMIC `ldo1` |
| PMIC | not described in DT | TPS65219 at `0x30` on `main_i2c0`; `VDD_CORE` 0.85 V |
| board EEPROM | `0x51` | `0x50` |
| Ethernet port 1 | RGMII, PHY at address 0 | RGMII, PHY at address 0 with `reset-gpios` on `main_gpio0` 15 |
| Ethernet port 2 | RGMII DP83867 at address 1 | **RMII** single-pair PHY at address 1, reset on `main_gpio1` 5 |
| HDMI | SiI9022 on `main_i2c1` | IT66121 on `main_i2c2` |
| Wi-Fi | none (`sdhci2` disabled) | WL1807 on `sdhci2` with `mmc-pwrseq-simple` |
| 1.4 GHz OPP | yes (“Requires VDD_CORE to be at 0.85V”) | yes, with the same comment |
| wake-up R5 memory | DMA pool `0x9da00000` (1 MiB) + carveout `0x9db00000` (12 MiB), remoteproc enabled | only `0x9db00000` (12 MiB), remoteproc not enabled in Linux |
| U-Boot | `am62x_evm_a53_defconfig` | `am62x_beagleplay_a53_defconfig`; `fdtfile` from `CONFIG_DEFAULT_DEVICE_TREE` |

## Initial Port State

An engineer copied `k3-am625-sk.dts` to `k3-am625-beagleplay.dts`, changed `model` to `BeagleBoard.org BeaglePlay`, and left everything else, including `compatible = "ti,am625-sk", "ti,am625"` and the include of `k3-am625-sk-common.dtsi`. U-Boot is built from `am62x_evm_a53_defconfig`. The reported symptoms:

1. Linux always reports `Texas Instruments AM625 SK`, even after the new DTB is installed
2. after installing the DTB by hand, the SD card never appears, and `/sys/kernel/debug/devices_deferred` lists the SD regulator
3. Ethernet port 2 never links
4. HDMI shows nothing, and the log reports a failed probe at I²C address `0x3b`
5. there is no Wi-Fi
6. UHS SD cards fail tuning, while high-speed-only cards work

## Part 1: Establish The Baseline

Create:

1. a hardware delta ledger, one entry per row of the table above, each with its source (schematic, DTS line, or U-Boot file) and a confidence state
2. an artifact table showing which DTB U-Boot actually loads, and why

For symptom 1, read `board/ti/am62x/evm.c`: `ti_set_fdt_env(NULL, NULL)` sets `fdtfile` to `CONFIG_DEFAULT_DEVICE_TREE`, which is `ti/k3-am625-sk` in the EVM defconfig. Explain why every later symptom is unreadable until this is fixed, and which defconfig BeaglePlay uses.

## Part 2: Design The Minimal DTS

Write the minimal tree from [Minimal Source Skeleton](minimal-dtb-boot-handoff-memory-console-and-boot-storage.md#minimal-source-skeleton): root identity, memory, console, and eMMC only. For every node inherited from the SK files, decide `remove`, `inherit disabled`, or `retain with evidence`. Justify why the new file must not include `k3-am625-sk-common.dtsi` or `k3-am62x-sk-common.dtsi`, and what `compatible` must say.

Define proof at each checkpoint: built DTB, installed DTB, U-Boot working FDT, `/sys/firmware/fdt`, and the Linux live tree.

## Part 3: Fix The SD Card Chain

Trace symptom 2 in the copied tree:

```text
sdhci1 vmmc-supply -> vdd_mmc1 (regulator-fixed)
  -> gpio = <&exp1 3 ...>
     -> exp1: TCA6424 at 0x22 on main_i2c1
        -> no device answers on BeaglePlay
```

Predict the `pca953x` probe failure and the regulator's permanent deferral, using the [Runtime Inspection lab](../runtime-inspection/runtime-device-tree-and-probe-forensics-lab.md) as a model. Then trace symptom 6: the copied `vqmmc` regulator toggles `main_gpio0` 31, a pad BeaglePlay leaves unmuxed, while the real switch on `main_gpio1` 49 never moves. Explain why the kernel still reports 1.8 V signaling. Write the corrected two regulators and compare them with upstream.

## Part 4: Bring Up Ethernet

For port 2, list every property that must change: `phy-mode` (`rgmii-id` to `rmii`), the pin group (RGMII2 pins to RMII and single-pair pins), and the PHY's reset GPIO and timings. For port 1, decide what to do with the SK's DP83867-specific properties and how to confirm the actual PHY from its ID registers. Use [`phy-mode` Is A PCB Contract](../common-peripheral-nodes/ethernet-macs-mdio-phys-and-fixed-links.md#phy-mode-is-a-pcb-contract) to explain why `rgmii-id` is still right for port 1.

## Part 5: Power And Performance

The copied tree has no PMIC node. Explain what still works (rails left on by firmware) and what is lost (PMIC interrupt, regulator visibility, power-off). Then check the 1.4 GHz OPP: confirm from BeaglePlay's TPS65219 `buck1` (`VDD_CORE`, 850000 µV) that keeping it is correct here, unlike the LP SK case in the [Security lab](../security-and-production-lifecycle/secure-device-tree-release-and-update-lab.md).

## Part 6: Memory And Remote Processors

Compare the reserved-memory maps of the two boards. BeaglePlay reserves only `0x9db00000`–`0x9e6fffff` for the wake-up R5's firmware, under a label ending `_dma_memory_region`, and does not enable `wkup_r5fss0_core0` in Linux. The SK reserves a separate 1 MiB DMA pool and enables IPC. Decide whether a product based on BeaglePlay needs IPC with the R5, and what the reserved-memory and `memory-region` entries must be if it does, using the [`memory-region` contract](../memory-firmware-and-heterogeneous-socs/memory-ownership-ram-and-reserved-regions.md#memory-region-connects-a-device-to-a-region).

## Part 7: Display, Wireless, And Options

Replace the SiI9022 with the IT66121 graph on `main_i2c2`, enable `sdhci2` with the WL1807 and its power sequencer, and decide how to handle the camera connector: BeaglePlay ships `k3-am625-beagleplay-csi2-ov5640.dtso` and `-tevi-ov5640.dtso`, composed in the TI `Makefile`. Define the overlay policy using [Board Revisions, Variants, Overlays, And Identity](board-revisions-variants-overlays-and-identity.md).

## Part 8: Build The Patch Series

Propose the series in upstream order: the board compatible in `Documentation/devicetree/bindings/arm/ti/k3.yaml`, then the board DTS, then the `Makefile` entries and overlays. Compare with the real upstream history (`git log --reverse --oneline -- arch/arm64/boot/dts/ti/k3-am625-beagleplay.dts`).

## Part 9: Define Final Qualification

Create the qualification matrix: `CHECK_DTBS=y` and `W=1` builds; semantic diff against your previous artifact; automatic boot selection with the BeaglePlay U-Boot; console continuity; eMMC and SD integrity, including UHS; both Ethernet ports; HDMI; Wi-Fi; each camera overlay; PMIC power-off; and sustained load at 1.4 GHz.

## Deliverables

1. delta ledger and artifact table
2. minimal DTS and checkpoint evidence
3. SD, Ethernet, power, memory, and peripheral analyses with corrected nodes
4. overlay policy
5. patch series plan
6. qualification matrix
7. a diff between your final DTS and upstream `k3-am625-beagleplay.dts`, with every difference explained

## Reference Analysis

The first divergence is not in any DTS: U-Boot built for the TI EVM always selects the SK's DTB, so symptom 1 hides every source change. With the right U-Boot, the copied tree fails exactly where the boards differ. The SD regulator waits forever for an expander that BeaglePlay does not have, and the I/O-voltage regulator drives an unconnected pad while the kernel believes it switched. Port 2 is configured for the wrong MAC interface, and the HDMI bridge, Wi-Fi, and PMIC are described for the wrong parts or not at all. Nothing in `dtc` or the schemas reports any of this, because every node is valid for *some* AM625 board. The upstream file avoids the problem structurally: it includes only `k3-am625.dtsi`, never the SK files, so no reference-board fact can leak in.

## Further Reading

- [Reference Board Selection, Hardware Delta, And Evidence Baseline](reference-board-selection-hardware-delta-and-evidence-baseline.md)
- [Minimal DTB, Boot Handoff, Memory, Console, And Boot Storage](minimal-dtb-boot-handoff-memory-console-and-boot-storage.md)
- [DMA, IOMMU, Reserved Memory, And Remote Processor Integration](dma-iommu-reserved-memory-and-remote-processor-integration.md)
- [Validation, Upstreaming, And Production Handoff](validation-upstreaming-and-production-handoff.md)
