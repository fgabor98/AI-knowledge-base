---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Custom Board Porting Capstone Lab

This capstone ports a custom AXC300 board from an AX9 evaluation module. The initial “mostly copied” DTB reaches U-Boot but Linux is unstable, console disappears, eMMC intermittently fails, Ethernet has no traffic, and a remote DSP corrupts memory. Your task is to stage the port, identify the earliest divergence in each chain, and produce a maintainable production handoff.

## Hardware Scenario

### Reference EVM-A

```text
SoC: AX9 BGA400
DDR: 2 GiB LPDDR4, firmware initialized
PMIC: PX901
console: UART0
boot/root: removable SD on MMC0, 4-bit, 3.3 V
Ethernet: MAC0 + X1000 PHY, RGMII-ID, PHY address 1
DSP carveout: 0x9c000000 size 32 MiB
board identity: fixed EVM compatible
```

### Custom AXC300 revB

```text
SoC: AX9 BGA400
DDR: 4 GiB LPDDR4, new firmware training data
PMIC: PX901, but peripheral 3.3 V rail moved behind GPIO2_11
console: UART2 on different pins
boot/root: soldered eMMC on MMC1, 8-bit, 1.8/3.3 V
Ethernet: MAC0 + Y2000 PHY, RGMII; RX delay strapped in PHY
PHY: address 3, reset GPIO1_7 active-low, 10 ms assert + 30 ms settle
DSP firmware: 48 MiB carveout at 0x98000000 plus 8 MiB vring at 0x9b000000
optional radio: SPI2 daughtercard identified by authenticated option EEPROM
board identity: OTP product AXC300, revision B
```

## Initial Port State

The engineer copied `evm-a.dts` to `axc300.dts`, changed `model`, enabled eMMC, and added the DSP carveout. All other EVM nodes remain.

### Initial source excerpts

```dts
/ {
    model = "Acme AXC300";
    compatible = "acme,evm-a", "acme,ax9";

    aliases {
        serial0 = &uart0;
        mmc0 = &mmc0;
    };

    chosen {
        stdout-path = "serial0:115200n8";
    };

    memory@80000000 {
        device_type = "memory";
        reg = <0x0 0x80000000 0x0 0x80000000>;
    };

    reserved-memory {
        #address-cells = <2>;
        #size-cells = <2>;
        ranges;

        dsp_mem: dsp@98000000 {
            reg = <0x0 0x98000000 0x0 0x03000000>;
            no-map;
        };

        dsp_vring: vring@9b000000 {
            reg = <0x0 0x9b000000 0x0 0x00800000>;
            no-map;
        };
    };
};

&mmc0 {
    status = "okay";
};

&mmc1 {
    bus-width = <8>;
    non-removable;
    status = "okay";
};

&ethernet0 {
    phy-handle = <&evm_phy>;
    phy-mode = "rgmii-id";
    status = "okay";
};

&dsp0 {
    memory-region = <&dsp_mem>, <&dsp_vring>;
    status = "okay";
};
```

## Evidence Package

### Build/package

```text
built axc300.dtb sha256: aaaa...
packaged evm-a.dtb sha256: eeee...
boot script variable fdtfile=evm-a.dtb
engineer manually loads axc300.dtb during some sessions
no artifact manifest records the manual path
```

### U-Boot automatic boot

```text
Loading /boot/evm-a.dtb
Working FDT model: Acme AX9 Evaluation Module A
Working FDT compatible: acme,evm-a, acme,ax9
Booting Linux with fdt at 0x88000000
```

### U-Boot manual boot with custom DTB

```text
Working FDT model: Acme AXC300
Working FDT compatible: acme,evm-a, acme,ax9
memory reg after firmware fixup: base 0x80000000 size 0x100000000
Booting Linux with fdt at 0x88000000
```

### Kernel log from manual boot

```text
OF: fdt: Machine model: Acme AXC300
Reserved memory: created region dsp@98000000, base 0x98000000, size 48 MiB
Reserved memory: created region vring@9b000000, base 0x9b000000, size 8 MiB
serial serial0: ttyS0 at MMIO ...
mmc0: new high speed SDHC card at address aaaa
mmc1: error -110 whilst initialising MMC card
ethernet0: attached PHY at address 1
ethernet0: Link is Up - 1000/Full
remoteproc dsp0: powering up
IOMMU: fault stream 0x31 address 0x9b100000
```

UART0 is not routed on AXC300, so the log was recovered from persistent memory. The physical UART2 header shows only U-Boot output; normal kernel console never appears.

### Memory observation

Firmware reports 4 GiB starting at `0x80000000`. DSP crash occurs soon after start. The intended DSP firmware region is `[0x98000000, 0x9b000000)`, while the vring is `[0x9b000000, 0x9b800000)`. A secure firmware region occupies `[0x9a000000, 0x9a400000)` and is injected by boot firmware as a separate reservation.

### Schematics/BOM findings

```text
MMC0 pins: not connected
MMC1 vmmc: periph_3v3 switched by GPIO2_11
MMC1 vqmmc: PMIC rail vcc_io_1v8
MMC1 card detect: none; eMMC non-removable
UART2 pins: D12/D13, function UART2, 1.8 V
PHY Y2000: MDIO address 3, reset active-low GPIO1_7
RGMII RX delay: enabled by PHY strap; TX delay must be configured by MAC
radio option: SPI2 CS0, 20 MHz, interrupt GPIO3_4 active-low
```

## Part 1: Establish The Baseline

Create:

1. reference candidate matrix
2. hardware delta ledger with confidence states
3. artifact/provenance table for automatic and manual boot
4. first minimal-boot hypothesis
5. recovery and evidence plan

Identify why the automatic-boot failure must be solved before interpreting custom DTS changes.

## Part 2: Design The Minimal DTS

Write a minimal source plan—not necessarily a complete compilable board file—that contains only:

- correct root model and new board compatible with justified fallback
- firmware-owned memory strategy
- UART2 console and pinctrl
- eMMC controller, conservative mode, pins, and supplies
- only providers needed by console/eMMC

For each EVM node decide `remove`, `inherit disabled`, or `retain with custom evidence`. Explain whether a static 2 GiB memory node should remain when trusted firmware correctly fixes 4 GiB.

Define proof at these checkpoints:

```text
built DTB
packaged DTB
U-Boot loaded DTB
U-Boot post-fixup working FDT
kernel boot FDT
Linux live tree
```

## Part 3: Restore Console

Trace:

```text
physical UART2
  -> bootloader output
  -> UART2 clock/reset
  -> UART2 pinctrl
  -> root alias/stdout-path
  -> kernel early console if used
  -> normal serial driver handover
```

State why changing only `console=ttyS2` may still fail, and why the current `serial0` alias points to the wrong device.

Define a pass result that includes uninterrupted kernel output through normal console handover.

## Part 4: Bring Up eMMC

Build the dependency graph for MMC1, including `periph_3v3`, GPIO2_11, PMIC 1.8 V, pinctrl, clock/reset, and controller.

Propose a conservative sequence:

1. provider availability
2. power measurement
3. controller/pins
4. basic eMMC identification
5. repeated read hashes
6. root mount
7. disposable write/read test
8. voltage switching/high-speed modes

Explain why MMC0 must stay disabled and how copying an EVM alias can affect boot scripts or Linux numbering.

## Part 5: Bring Up Ethernet

Correct the conceptual errors:

- stale EVM PHY phandle/address
- wrong PHY compatible if one is required by its binding
- reset GPIO/polarity/timing
- `phy-mode` and delay responsibility
- PHY supply/reference clock

Do not choose a delay mode by trial-and-error pinging. Use binding definitions, strap state, MAC/PHY delay responsibility, and timing evidence.

Define identity, link, packet, throughput/error, cycling, and power-management tests.

## Part 6: Repair Memory And Remoteproc

Normalize all ranges as half-open intervals and find the overlap. Explain why `dsp_mem` overlaps the secure firmware region even though the vring begins exactly at `0x9b000000`.

Create a corrected ownership map that includes:

- 4 GiB Linux-visible banks/holes
- secure firmware reservation
- DSP firmware region large enough for 48 MiB without overlap
- vring region
- kernel/initrd/FDT/U-Boot relocation constraints

Then trace the IOMMU fault:

```text
DSP iommus property
  -> provider #iommu-cells
  -> custom-board/SoC stream ID
  -> domain/mapping
  -> firmware device address/resource table
```

Define start, IPC, stop, restart, crash, and stress gates before enabling auto-boot.

## Part 7: Add The Optional Radio

Decide whether to use:

- separate full DTB
- base plus authenticated overlay
- static disabled node with a fixup

Justify the answer using independent fitment, authenticated option EEPROM, SPI/power/interrupt dependencies, base compatibility, update/security policy, and test matrix.

Define behavior for valid-present, valid-absent, corrupt identity, unknown option, incompatible overlay, and EEPROM unavailable.

## Part 8: Build The Patch Series

Propose patches for:

1. board compatible schema
2. any missing Y2000 PHY binding/driver support
3. SoC data corrections if truly common
4. AXC300 base board DTS with console/eMMC
5. regulators/pinctrl
6. Ethernet
7. corrected reserved memory/remoteproc
8. radio overlay and compatibility policy
9. build target/MAINTAINERS/inventory/CI

Keep urgent correctness fixes, mechanical source moves, feature enablement, and cleanup separate. State maintainer routing and dependencies.

## Part 9: Define Final Qualification

Create a matrix for:

- revB without radio
- revB with radio
- normal release
- recovery release
- supported current/rollback kernel and bootloader

Include:

```text
schema and W=1 build
artifact/hash/semantic diff
automatic boot selection
console continuity
memory stress and reservation checks
eMMC data integrity and power cycles
Ethernet traffic/error/cycling
DSP IPC/restart/crash/IOMMU
radio composition and function
update/rollback/recovery
suspend/resume/thermal/watchdog as product requires
```

## Deliverables

1. reference and hardware delta analysis
2. minimal DTS and stage-gate plan
3. corrected boot selection/console/eMMC chains
4. Ethernet timing and validation plan
5. non-overlapping memory/DMA/IOMMU/remoteproc design
6. radio variant/overlay identity policy
7. upstream/downstream patch series
8. production qualification and handoff bundle
9. causal timeline explaining every initial symptom

## Reference Analysis

The first divergence is artifact selection: automatic boot loads `evm-a.dtb`, so no custom source edit can affect that path. Manual boot uses the custom bytes, but the root compatible remains the EVM compatible, preserving wrong identity and possibly platform policy. Fix selection and root binding before peripheral diagnosis.

Console fails because the copied alias and `stdout-path` select UART0, which is not routed. Bootloader UART2 output proves some firmware-side setup, not that the Linux working FDT enables UART2 with the correct pins, clock/reset, alias, and command-line handover.

eMMC fails because the copied port enables MMC1 without its custom power and pin dependency chain while MMC0 remains an irrelevant EVM device. Establish the GPIO-controlled 3.3 V and PMIC I/O rail first, then conservative MMC1 operation.

Ethernet binds the wrong PHY at address 1 and inherits `rgmii-id` even though the custom PHY is address 3 and RX delay is strapped. Rebuild MAC/MDIO/PHY/reset/reference-clock/delay facts from binding and hardware evidence.

The DSP region `[0x98000000, 0x9b000000)` contains secure firmware `[0x9a000000, 0x9a400000)`, so starting the remote processor can corrupt or fault protected ownership. The adjacent vring start is not itself an overlap with the DSP interval. Redesign the carveout rather than hiding the secure reservation, then independently correct IOMMU stream/mapping and firmware resource-table agreement.

Only after the stable base qualifies should the radio be introduced. Its independently fitted nature and authenticated option identity may justify a signed overlay, but unknown/corrupt identity must fail safely and every base/overlay pair needs composition and hardware tests.

## Further Reading

- [Reference Board Selection, Hardware Delta, And Evidence Baseline](reference-board-selection-hardware-delta-and-evidence-baseline.md)
- [Minimal DTB, Boot Handoff, Memory, Console, And Boot Storage](minimal-dtb-boot-handoff-memory-console-and-boot-storage.md)
- [DMA, IOMMU, Reserved Memory, And Remote Processor Integration](dma-iommu-reserved-memory-and-remote-processor-integration.md)
- [Validation, Upstreaming, And Production Handoff](validation-upstreaming-and-production-handoff.md)
