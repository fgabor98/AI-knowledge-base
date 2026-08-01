---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Board Porting Workflow

A reliable board port is a sequence of controlled experiments, not a bulk copy of the closest evaluation board. Start with proven firmware, kernel, toolchain, and reference artifacts; describe only verified custom-board deltas; establish console and boot storage; then add dependency chains and peripherals one subsystem at a time.

This capstone turns the earlier Device Tree topics into a gated workflow. Every stage defines its inputs, smallest useful change, runtime proof, failure boundary, and evidence to preserve before advancing.

## Learning Outcomes

After completing this module, you should be able to:

- choose a reference board by SoC/package, PMIC, DDR, boot path, and peripheral topology rather than visual similarity
- create a hardware delta ledger from schematics, BOM, layout constraints, strap state, and firmware responsibilities
- freeze known-good reference and custom-board evidence before modifying DTS
- separate bootloader control FDT, kernel working FDT, packaged artifact, and live Linux tree during bring-up
- build a minimal board DTS with correct root identity, memory, chosen console, essential fixed clocks, and boot-critical hardware only
- establish a reproducible early-console and boot-storage path before enabling optional buses
- trace power, clock, reset, pinctrl, GPIO, interrupt, and provider dependencies in safe activation order
- bring up I2C, SPI, MMC, Ethernet, USB, and other peripherals with subsystem-level tests rather than probe-only evidence
- design memory and DMA ownership across Linux, firmware, IOMMUs, CMA/shared pools, and remote processors without overlap
- model board revisions and optional hardware using explicit DTS composition, compatible strings, or controlled overlays
- reject auto-detection or overlay schemes whose identity source, failure behavior, or compatibility is unsafe
- use binding validation, warning-clean builds, artifact diffs, U-Boot inspection, live-tree evidence, deferred-probe state, and functional tests as stage gates
- structure bindings, driver support, SoC data, and board DTS patches for upstream review
- hand the port to production with provenance, security, compatibility, recovery, and regression coverage
- diagnose the capstone incident by finding the earliest divergence instead of editing every visible failure

## Prerequisites

This workflow assumes completion of the Device Tree chapter through [Product-Scale Maintenance And Engineering](product-scale-maintenance-and-engineering.md). In particular, be comfortable with provider-consumer decoding, binding schemas, U-Boot mutation, build provenance, runtime inspection, and stable ABI.

## Learning Path

1. [Reference Board Selection, Hardware Delta, And Evidence Baseline](board-porting-workflow/reference-board-selection-hardware-delta-and-evidence-baseline.md)
2. [Minimal DTB, Boot Handoff, Memory, Console, And Boot Storage](board-porting-workflow/minimal-dtb-boot-handoff-memory-console-and-boot-storage.md)
3. [Power, Clock, Reset, Pinctrl, And GPIO Bring-Up](board-porting-workflow/power-clock-reset-pinctrl-and-gpio-bring-up.md)
4. [Buses, Storage, Networking, And Peripheral Enablement](board-porting-workflow/buses-storage-networking-and-peripheral-enablement.md)
5. [DMA, IOMMU, Reserved Memory, And Remote Processor Integration](board-porting-workflow/dma-iommu-reserved-memory-and-remote-processor-integration.md)
6. [Board Revisions, Variants, Overlays, And Identity](board-porting-workflow/board-revisions-variants-overlays-and-identity.md)
7. [Validation, Upstreaming, And Production Handoff](board-porting-workflow/validation-upstreaming-and-production-handoff.md)
8. [Custom Board Porting Capstone Lab](board-porting-workflow/custom-board-porting-capstone-lab.md)

## Stage-Gate Model

```text
known-good reference and delta ledger
  -> exact artifact selection and handoff
  -> CPU/RAM/console visibility
  -> boot storage and root filesystem
  -> power/clock/reset/pinctrl providers
  -> one bus and consumer chain at a time
  -> DMA/reserved-memory/remote processors
  -> variants and optional compositions
  -> full validation, upstreaming, production qualification
```

Do not advance because a timeout expired or a node looks plausible. Advance when the stage's proof gate passes and the evidence bundle can explain the result later.

## Bring-Up Evidence Matrix

| Evidence | Proves | Does not prove |
|---|---|---|
| same SoC as reference | many on-chip blocks are reusable | board wiring, PMIC, DDR, clocks, pins, or PHY are identical |
| `dtc` compiles | source encodes a valid FDT structure | binding or hardware correctness |
| `dtbs_check` passes | applicable schema constraints pass | electrical design or bootloader handoff is correct |
| U-Boot `fdt print` | working FDT contains shown values | Linux received the same address/blob |
| early console output | CPU, RAM subset, UART path, and handoff progressed | boot storage or remaining memory is safe |
| device exists in sysfs | Linux created the bus device | driver bound or hardware operates |
| driver is bound | matching and probe succeeded | data integrity, timing, interrupts, DMA, or power management works |
| one cold boot works | one state/path worked | repeated boots, warm reset, suspend, stress, or recovery work |

## Working Rules

1. Change one dependency chain at a time.
2. Keep a known bootable artifact and recovery path.
3. Preserve complete logs before rebooting or rebinding.
4. Compare built and runtime trees, not only source text.
5. Disable unknown optional hardware; do not invent placeholder resources.
6. Verify provider cells and electrical polarity from bindings and schematics.
7. Treat memory, DMA, power, thermal, boot storage, and console changes as high risk.
8. Move proven common facts upward only after multiple boards demonstrate true commonality.
9. Convert every discovered board delta into source, inventory, validation, and ownership records.
10. Never use a production workaround to conceal an unexplained bring-up failure.

## Completion Check

You have completed the Device Tree chapter when you can:

- reconstruct why a chosen reference is close and enumerate every unproven difference
- reach a kernel console with a minimal custom DTB and prove exact boot/live artifact identity
- establish boot storage without depending on optional regulators, buses, or overlays
- enable a consumer only after its pin, power, clock, reset, interrupt, and parent-bus chain is proven
- validate Ethernet/storage/peripherals at subsystem level under stress and reset
- produce a non-overlapping memory ownership map and safely start/stop remote processors
- select board revision and optional hardware from a trustworthy identity source with defined failure behavior
- create an upstreamable patch series and a product-ready evidence/compatibility record
- explain every stage gate, rollback point, and retained artifact in the lab
- apply the workflow to a new custom board without copying unexplained reference-board nodes

## Authoritative References

- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Booting AArch64 Linux](https://docs.kernel.org/arch/arm64/booting.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Writing Devicetree bindings](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [U-Boot fdt command](https://docs.u-boot.org/en/stable/usage/cmd/fdt.html)
- [U-Boot bootm command](https://docs.u-boot.org/en/stable/usage/cmd/bootm.html)
- [Linux remoteproc framework](https://docs.kernel.org/staging/remoteproc.html)

## Related Topics

- [Common Peripheral Nodes](common-peripheral-nodes.md)
- [Runtime Inspection](runtime-inspection.md)
- [Product-Scale Maintenance And Engineering](product-scale-maintenance-and-engineering.md)
- [Custom Sitara Board Bring-Up](../build-systems/advanced/ti-processor-sdk/custom-sitara-board-bring-up.md)
