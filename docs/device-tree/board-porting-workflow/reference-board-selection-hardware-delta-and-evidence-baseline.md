---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Reference Board Selection, Hardware Delta, And Evidence Baseline

The best reference is the board whose boot-critical architecture most closely matches the custom design. A reference DTS is evidence about one assembly, not a template whose nodes are presumed valid until they fail.

## Rank Reference Candidates

Score candidates in this order:

1. SoC family, exact device, package, and silicon revision
2. boot ROM mode and first-stage boot medium
3. DDR type/topology and firmware initialization path
4. PMIC, regulator topology, sequencing, and voltage domains
5. main oscillators and reference clocks
6. console UART instance and pin routing
7. boot-storage controller, bus width, voltage, reset, and detect behavior
8. Ethernet MAC/PHY/interface mode and delays
9. USB role/PHY/power architecture
10. external buses, GPIO expanders, codecs, sensors, remote processors, and options

Physical connector similarity and marketing family names are weak evidence.

## Create A Candidate Matrix

| Dimension | Custom | EVM-A | EVM-B | Risk if copied |
|---|---|---|---|---|
| SoC/package | AX9-BGA400 | exact | AX9-BGA324 | pin/resource mismatch |
| DDR | LPDDR4 2 GiB | exact topology | DDR4 4 GiB | memory handled by firmware but DT size/fixups differ |
| PMIC | PX901 | exact | fixed rails | unsafe regulator assumptions |
| console | UART2 pins C5/C6 | exact | UART0 | no early output |
| boot | eMMC on MMC1 8-bit | SD on MMC0 | exact eMMC | selection versus pin/power tradeoff |
| Ethernet | RGMII PHY Y | same MAC, other PHY | same PHY/interface | binding/delay differences |

Often one reference provides the SoC/PMIC base while another supplies a peripheral example. Never merge them without reconciling bindings and wiring.

## Freeze Known-Good Evidence

For the chosen reference record:

```text
source repositories and commits
toolchain, dtc, dt-schema, kernel configuration
bootloader/SPL/firmware versions and environment
exact kernel, DTB, overlays, initramfs/rootfs hashes
boot-media layout and selected filenames/configuration
U-Boot control and working FDT evidence
complete boot log with timestamps
runtime root model/compatible and live-tree capture
driver/module/deferred-probe state
functional results for boot storage, console, network, and power
```

For the custom board record the same evidence even when boot fails. Differences need comparable checkpoints.

## Build The Hardware Delta Ledger

Use primary sources:

- released schematic and revision
- BOM and fitted/alternate parts
- PCB layout constraints and net names where timing matters
- SoC/PMIC/PHY datasheets and errata
- strap and fuse tables
- power tree and sequencing specification
- clock tree
- memory map and firmware ownership
- connector/option matrix

Example:

```yaml
delta_id: DELTA-ETH-03
area: ethernet0
reference: EVM-A
custom_fact: "PHY changed X1000 -> Y2000; RGMII RX delay strapped on PHY"
sources: [SCH-AXC300-revB-p42, BOM-AXC300-revB]
dt_impact:
  - phy compatible
  - reset GPIO/polarity/timing
  - phy-mode and delay properties
firmware_impact: none
test:
  - MDIO ID
  - link modes
  - ping/iperf
  - repeated cold boot
owner: network-platform
status: unproven
```

## Cover Every Hardware Domain

### Boot and identity

- boot straps/fuses and fallback media
- board ID/revision source and encoding
- EEPROM/OTP accessibility before selection
- recovery console and recovery storage

### Memory

- banks, size, holes, ECC
- secure/firmware/remoteproc reservations
- bootloader-generated memory fixups
- CMA and DMA reachability

### Power and clocks

- all rails, always-on versus switched, voltage/current constraints
- enable GPIOs, polarity, startup/off-on delay
- reference oscillators and clock routing
- reset domains and shared resets

### Pins and buses

- mux, pad voltage, bias, drive, slew
- bus instance, addresses/chip selects, widths and frequencies
- GPIO numbering through controllers/expanders
- interrupt controller, line, trigger, polarity, wake behavior

### High-speed interfaces

- PHY type and mode
- lane mapping and reference clocks
- reset/power sequencing
- board-specific delay/skew settings

### Firmware-owned resources

- secure monitor and system-controller ownership
- bootloader fixups
- remote processors and firmware files
- protected/reserved memory

## Mark Facts By Confidence

```text
CONFIRMED: primary hardware source and reviewer agree
INFERRED: derived from related evidence; needs test
COPIED: inherited from reference; custom applicability unknown
MEASURED: observed on one or more boards with method recorded
CONFLICT: sources disagree; block activation
NOT_APPLICABLE: absent by design with evidence
```

Do not allow `COPIED` facts into production unchanged.

## Establish A Board Port Workspace

Keep immutable evidence separate from generated output:

```text
port/
  evidence/reference/
  evidence/custom/
  hardware-deltas.yaml
  artifact-manifests/
  logs/
  decoded-trees/
  validation/
  test-results/
```

Record commands and tool versions. Do not hand-edit deployed DTBs as the source of a fix.

## Define The First Hypothesis

A good first hypothesis is narrow:

```text
Given the same SoC boot firmware and kernel as EVM-A,
the custom board should reach UART2 early console when supplied with:
  - correct root compatible/model
  - firmware-reported RAM
  - UART2 pinctrl/clock/status
  - bootloader chosen stdout-path
and all nonessential board devices disabled.
```

It states what is reused, what changes, and what success proves.

## Entry Gate For Editing DTS

Do not start the port until:

```text
[ ] reference boots reproducibly from a known artifact set
[ ] custom hardware revision and fitted BOM are known
[ ] closest reference and rejected alternatives are documented
[ ] boot-critical deltas have primary evidence
[ ] bootloader control/working DT distinction is understood
[ ] recovery path and immutable known-good artifacts exist
[ ] board identity method is known or explicitly deferred
[ ] first minimal-boot hypothesis and proof are written
```

## Further Reading

- [Product-Scale Maintenance And Engineering](../product-scale-maintenance-and-engineering.md)
- [Build Pipeline, Preprocessing, And Artifact Provenance](../build-and-diagnostic-tools/build-pipeline-preprocessing-and-artifact-provenance.md)
- [Minimal DTB, Boot Handoff, Memory, Console, And Boot Storage](minimal-dtb-boot-handoff-memory-console-and-boot-storage.md)
