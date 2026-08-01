---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Buses, Storage, Networking, And Peripheral Enablement

Once boot-critical suppliers are stable, enable one parent bus and one consumer chain at a time. Start conservatively, prove identity and basic transfer, then add interrupts, DMA, high-speed timing, power management, and performance features.

## Use The Same Ladder For Every Peripheral

```text
hardware fitted and powered
  -> parent controller available
  -> pins/clocks/resets correct
  -> child node address/chip select valid
  -> Linux bus device created
  -> driver matches and probes
  -> identity/status register is correct
  -> controlled transfer succeeds
  -> interrupt/DMA path succeeds
  -> stress/error/reset/power management succeeds
```

Record the first failed transition. Avoid making changes at later layers until earlier evidence passes.

## I2C Bring-Up

Verify:

- correct controller and pin mux
- open-drain behavior, pull-ups, voltage domain, and rise-time budget
- bus frequency initially conservative
- 7-bit versus 10-bit address and strap state
- mux/switch hierarchy
- regulator and reset dependencies
- interrupt line if used
- no duplicate address on the same reachable segment

Use bus scans cautiously: probing unknown addresses can alter or lock some devices. Prefer the schematic/BOM address, driver identity read, logic analyzer, and known-safe transactions.

## SPI Bring-Up

Verify:

- controller, chip-select source/index, and active polarity
- mode (CPOL/CPHA), maximum frequency, bits per word
- native versus GPIO chip select and timing constraints
- reset/power sequence
- MISO electrical behavior when deselected
- shared-bus compatibility among children

Start below the maximum clock and prove device ID. Increase frequency with signal evidence and repeated transfers.

## Storage Beyond Initial Boot

For eMMC/SD/SDIO:

- bus width, card-detect/write-protect, non-removable semantics
- `vmmc`/`vqmmc` rails and voltage switching
- pinctrl states for default and high-speed modes
- tuning/strobe properties from the controller binding
- reset line and power sequencing
- boot partition versus user-area assumptions

For SPI-NOR/NAND:

- erase/page geometry discovered by the correct binding/driver
- chip-select and clock mode
- ECC/bad-block ownership for NAND
- fixed partitions only when they are a stable firmware contract
- update/recovery power-loss tests

Never perform destructive write tests on the only boot copy. Use a dedicated region and verified recovery.

## Ethernet Bring-Up As Multiple Links

Treat Ethernet as:

```text
MAC registers/clock/reset
  -> MAC-to-PHY interface and reference clock
  -> MDIO management bus
  -> PHY power/reset/straps/identity
  -> phy-handle/fixed-link/SFP relationship
  -> link negotiation
  -> packet data path
```

Verify:

- `phy-mode` matches electrical interface and driver binding
- internal versus PCB/PHY delay responsibility is unambiguous
- PHY address and ID match straps/BOM
- reset polarity and timing
- reference clock direction/frequency
- supplies and interrupt if used
- MAC address provisioning source and uniqueness

Proof sequence:

1. read correct PHY ID
2. link at expected modes
3. inspect negotiated speed/duplex
4. ping both directions
5. sustained TCP/UDP transfer and error counters
6. repeated cable/link cycles and cold boots
7. suspend/resume or wake if required

A link LED is not packet integrity evidence.

## USB Bring-Up

Separate controller, PHY, connector, role, and VBUS:

- USB controller/PHY clocks and resets
- PHY supplies/reference clock/tuning
- `dr_mode` and role-switch/connector description
- VBUS regulator, current limit, enable polarity, overcurrent
- ID/CC detection and Type-C controller relationships
- lane routing/orientation for high speed

Test host and device roles only if the product supports them. Include connect/disconnect, current/power behavior, repeated enumeration, and transfer integrity.

## Other Peripheral Classes

For audio, display, camera, PCIe, CAN, sensors, LEDs, and watchdogs, identify:

- graph/endpoints or link topology
- external clocks and rates
- regulators, GPIO enables, resets
- lane/bus configuration and timing
- interrupt/DMA/IOMMU relationships
- calibration/firmware inputs
- subsystem-specific user-visible result

Consult the exact binding and subsystem documentation; do not derive topology from a different reference device merely because the driver name matches.

## Add DMA And Performance Only After PIO/Basic Function

Where the driver supports a conservative path:

1. prove register access and basic transfer
2. validate interrupt path
3. enable DMA mapping/specifiers
4. test buffer boundaries, coherency, IOMMU faults, and load
5. enable high-speed modes

Not every driver exposes a PIO fallback, but the reasoning still separates control-plane identity from data-path failures.

## Collect Subsystem Evidence

Examples:

```bash
dmesg --color=never | grep -Ei 'i2c|spi|mmc|nand|phy|ethernet|usb'
cat /proc/interrupts
ip -details link show
ethtool eth0 2>/dev/null
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINTS
```

Add device-specific identity, error counters, test data hashes, logic-analyzer captures, and power measurements. Preserve kernel logs from before and during the test.

## Avoid Parallel Mystery Changes

Bad bring-up patch:

```text
enable I2C, Ethernet, USB, audio, camera
change PMIC voltages
add all pin states
suppress warnings
```

Good progression:

```text
power/provider prerequisites
one bus with no children
one consumer identity
consumer functional test
interrupt/DMA/performance
next consumer
```

## Stage Exit Gate

```text
[ ] each enabled device maps to fitted BOM/schematic and exact binding
[ ] parent bus works independently where observable
[ ] conservative identity/basic transfer passes
[ ] interrupt and DMA evidence is isolated and correct
[ ] high-speed modes have timing/signal/stress evidence
[ ] storage tests preserve recovery and verify data integrity
[ ] Ethernet/USB prove full data path, not LEDs/enumeration alone
[ ] power-cycle, error recovery, and required PM paths pass
[ ] no unexplained errors or global warning suppressions remain
```

## Further Reading

- [Common Peripheral Nodes](../common-peripheral-nodes.md)
- [Driver Matching](../driver-matching.md)
- [Linux phylink documentation](https://docs.kernel.org/networking/sfp-phylink.html)
- [DMA, IOMMU, Reserved Memory, And Remote Processor Integration](dma-iommu-reserved-memory-and-remote-processor-integration.md)
