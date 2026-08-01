---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binding Design And ABI Review Lab

This lab turns a driver-shaped downstream description into a hardware-centered binding and a staged ABI migration. The goal is the reasoning package: hardware inventory, ownership, compatible strategy, property semantics, compatibility matrix, patch order, and tests. Full YAML syntax is deferred to the next module.

## Scenario

Acme's AXC100 capture engine shipped in Falcon revision A. Falcon revision B uses AXC200, a strict register-level extension in normal streaming mode, but it adds an error interrupt and a mandatory reset sequence after DMA faults. Some revision B units also populate an external sample-clock oscillator.

Artifacts update independently:

- U-Boot and the Linux DTB are in the boot partition
- Linux kernel and modules are in A/B system slots
- factory recovery contains an older kernel
- an optional sensor module is described by an overlay
- U-Boot reads board revision from a CRC-protected EEPROM

The current downstream DTS is:

```dts
capture@48000000 {
        compatible = "acme,ax-capture-driver";
        reg = <0x0 0x48000000 0x0 0x1000>;
        interrupts = <GIC_SPI 80 IRQ_TYPE_LEVEL_HIGH>;
        clocks = <&ccu 12>;
        acme,version = <2>;
        acme,use-error-workaround;
        acme,poll-timeout = <500>;
        acme,dma-buffer-size = <1048576>;
        acme,external-clock-rate = <24000000>;
        acme,enable-fast-mode;
        status = "okay";
};
```

The driver:

- reads `acme,version` to choose register definitions
- allocates the requested DMA buffer size
- sleeps for `acme,poll-timeout` microseconds between status checks
- performs a reset only when `acme,use-error-workaround` is present
- programs 24 MHz internally when `acme,external-clock-rate` exists
- enables a more aggressive batching algorithm for `acme,enable-fast-mode`
- uses interrupt index zero as completion
- does not request AXC200's error interrupt or reset line

## Hardware Evidence

The manuals and schematics establish:

### AXC100

- one 4 KiB control register window
- one completion interrupt
- `bus` and `sample` clock inputs
- no reset input exposed to the block
- FIFO depth is readable from `CAPS`
- maximum sample clock is 20 MHz

### AXC200

- the AXC100 register programming model remains safe for normal capture
- a new error interrupt reports DMA faults
- a `core` reset input must be asserted after a DMA fault before capture resumes
- FIFO depth and silicon revision are readable from `CAPS`
- maximum sample clock is 30 MHz
- the old normal-streaming path is safe until a DMA fault occurs; old software cannot recover from that fault

### Board Integration

- revision A uses AXC100 with a fixed SoC-derived 20 MHz sample clock
- revision B uses AXC200
- all revision B boards connect the error IRQ and reset
- revision B option X supplies a 24 MHz oscillator to the `sample` clock input
- the carrier limits the sample input to 25 MHz for signal-integrity reasons
- buffer size and polling cadence are product performance policy, not electrical facts

## Deployment Evidence

- 20,000 revision A DTBs with `acme,ax-capture-driver` have shipped
- 500 early revision B DTBs use the same compatible plus `acme,version = <2>`
- the old recovery kernel matches only `acme,ax-capture-driver`
- recovery performs normal capture but does not inject DMA faults
- production requirements say recovery must never continue after an unrecoverable capture fault
- bootloader code does not parse the capture node
- overlay `falcon-sensor.dtbo` targets label `capture0` and adds a graph endpoint

## Lab Objectives

Produce these artifacts:

1. candidate-property disposition table
2. corrected hardware model
3. compatible and fallback decision
4. required/optional/conditional property table
5. old/new compatibility matrix
6. migration and rollback plan
7. patch dependency graph
8. validation and hardware test plan
9. ABI decision record

Do not begin by editing the driver. First prove the contract.

## Task 1: Dispose Of Existing Properties

Complete this table:

| Existing field | Hardware fact? | Correct home | Migration concern |
|---|---|---|---|
| `acme,version` |  |  |  |
| `acme,use-error-workaround` |  |  |  |
| `acme,poll-timeout` |  |  |  |
| `acme,dma-buffer-size` |  |  |  |
| `acme,external-clock-rate` |  |  |  |
| `acme,enable-fast-mode` |  |  |  |

For each rejected property, name the underlying fact or policy. Do not simply label it “bad.”

## Task 2: Inventory Resources Per Compatible

Write separate AXC100 and AXC200 resource contracts. Decide:

- compatible strings
- `reg` regions
- interrupts and semantic names
- clocks and semantic names
- resets and semantic names
- board-imposed clock limit
- discoverable capabilities
- sensor graph relationship

Explain whether option X requires a peripheral compatible, root compatible, overlay, or only a different clock provider relationship.

## Task 3: Decide Fallback Safety

Evaluate this proposed AXC200 list:

```dts
compatible = "acme,axc200", "acme,axc100";
```

Answer separately for:

- initial capture
- sustained capture without faults
- DMA fault handling
- suspend/resume
- recovery mode

Then decide whether the fallback claim is globally safe. Product requirements, not just the happy path, determine the answer.

## Task 4: Handle The Shipped Legacy ABI

The string `acme,ax-capture-driver` and `acme,version` are already deployed. Design new production ABI without pretending those DTBs do not exist.

State:

- how a new driver recognizes legacy revision A
- how it recognizes the 500 legacy revision B units
- what happens when legacy revision B lacks the error IRQ/reset description
- whether in-field DTB replacement is mandatory
- how factory recovery avoids unsafe operation
- when, if ever, legacy parsing can be removed

## Task 5: Draft Corrected Instances

Draft conceptual DTS for revision A and revision B option X. Use labels and graph contents only as needed to preserve the overlay target. Do not invent numeric provider specifier details beyond the scenario.

A possible structural starting point is:

```dts
capture0: capture@48000000 {
        compatible = "...";
        reg = <0x0 0x48000000 0x0 0x1000>;
        ...
};
```

Explain why the 25 MHz carrier limit is or is not represented, using the relevant clock or subsystem semantics rather than blindly copying the old scalar.

## Task 6: Build The Compatibility Matrix

At minimum, analyze:

| DTB | new production kernel | old recovery kernel |
|---|---|---|
| legacy revision A |  |  |
| corrected revision A |  |  |
| legacy revision B |  |  |
| corrected revision B |  |  |

Add U-Boot selection, overlay compatibility, and A/B rollback constraints outside the table.

## Task 7: Submission And Evidence

Create a patch series whose dependency order accounts for:

- new binding
- driver changes
- revision A DTS migration
- revision B DTS
- overlay target compatibility
- any recovery/update guard

List schema, DTB, cross-version, and hardware tests. “Booted both boards” is not sufficient.

## Reference Analysis

Use this section only after completing your design.

### Property Disposition

| Existing field | Disposition | Reason and migration |
|---|---|---|
| `acme,version` | replace with specific compatibles | implementation identity is non-discoverable to pre-probe matching, but the numeric switch is a weak ABI; retain legacy parsing for shipped blobs |
| `acme,use-error-workaround` | replace with AXC200 match data plus required reset resource | the sequence is inherent to AXC200, not an instance choice |
| `acme,poll-timeout` | remove from new ABI | selects a driver algorithm; new driver should use interrupts/timeouts derived from its runtime design and hardware limits |
| `acme,dma-buffer-size` | remove from new ABI | memory allocation/performance policy belongs outside the hardware description |
| `acme,external-clock-rate` | replace with `clocks`/`clock-names` and clock framework data | the relationship identifies the actual oscillator; do not duplicate provider rate |
| `acme,enable-fast-mode` | remove from new ABI | selects a batching policy, not a fixed hardware fact |

The shipped legacy properties remain parsing inputs only for the compatibility path. They should be deprecated and no longer emitted by corrected DTS producers.

### Corrected Resource Contracts

AXC100:

- `compatible = "acme,axc100"`
- one register region
- one interrupt named `completion`
- clocks named `bus` and `sample`
- no reset
- FIFO depth read from `CAPS`

AXC200:

- `compatible = "acme,axc200"`
- one register region
- interrupts named `completion` and `error`
- clocks named `bus` and `sample`
- reset named `core`
- FIFO depth/revision read from `CAPS`

Because safe fault recovery requires resources and behavior old AXC100 software does not provide, do not list `acme,axc100` as an AXC200 fallback. The absence of a fallback deliberately prevents an old kernel from binding corrected AXC200 DTBs.

Option X changes the provider connected to the `sample` input, not the capture IP identity. Whether it uses an overlay or a separate board DTS depends on how the option is assembled and selected; the capture compatible remains `acme,axc200`. The board's 25 MHz constraint should be represented using established clock/subsystem constraints if such semantics exist. Do not invent a duplicate rate property; ensure assigned/provider rate cannot exceed the electrical limit.

### Conceptual Corrected DTS

Revision A:

```dts
capture0: capture@48000000 {
        compatible = "acme,axc100";
        reg = <0x0 0x48000000 0x0 0x1000>;
        interrupts = <GIC_SPI 80 IRQ_TYPE_LEVEL_HIGH>;
        interrupt-names = "completion";
        clocks = <&ccu 12>, <&ccu 13>;
        clock-names = "bus", "sample";
        status = "okay";
};
```

Revision B option X:

```dts
capture0: capture@48000000 {
        compatible = "acme,axc200";
        reg = <0x0 0x48000000 0x0 0x1000>;
        interrupts = <GIC_SPI 80 IRQ_TYPE_LEVEL_HIGH>,
                     <GIC_SPI 81 IRQ_TYPE_LEVEL_HIGH>;
        interrupt-names = "completion", "error";
        clocks = <&ccu 12>, <&capture_osc>;
        clock-names = "bus", "sample";
        resets = <&resetc 7>;
        reset-names = "core";
        status = "okay";
};
```

The label `capture0` is source-level support for the known overlay build. Verify that the base is compiled with symbols when label-based fixups are used and validate the merged final tree. The binding contract is the graph relationship, not the spelling of the label.

### Legacy Migration

A defensible staged plan is:

1. Add a legacy driver match for `acme,ax-capture-driver`.
2. For legacy `acme,version = <1>`, preserve AXC100 behavior while warning that the description is deprecated.
3. For legacy version 2, refuse normal production operation unless the driver can obtain and validate the missing error IRQ/reset through a documented legacy mapping. Guessing provider specifiers is unacceptable.
4. Replace all revision B DTBs atomically with the corrected AXC200 description before enabling the new production path.
5. Make old factory recovery reject corrected AXC200 through the lack of fallback, then update recovery or route revision B to a non-capture recovery flow.
6. Preserve old revision A parsing for the stated product lifetime because those blobs have shipped broadly.
7. Preserve a field manifest that distinguishes corrected and legacy DTBs and blocks unsafe rollback pairs.

If product recovery requires capture on revision B, recovery must be updated before corrected DTBs are deployed. A claim that faults are unlikely does not satisfy the stated failure requirement.

### Compatibility Matrix

One possible policy is:

| DTB | new production kernel | old recovery kernel |
|---|---|---|
| legacy revision A | legacy match, supported with warning | released behavior |
| corrected revision A | specific AXC100 match | no match unless recovery is updated; coordinate rollout |
| legacy revision B | block until corrected or use an explicitly proven legacy recovery path | unsafe capture path; update/disable |
| corrected revision B | full AXC200 path with IRQ/reset recovery | no match by design |

If preserving old recovery for corrected revision A is mandatory, a safe compatibility list containing the deployed legacy string may be considered specifically for AXC100, provided new schemas and consumers define it intentionally. Do not extend that conclusion to AXC200.

U-Boot must map EEPROM identities to exact DTB/overlay manifests, reject unknown revisions, and log the selection. A/B metadata must prevent rollback to a kernel that cannot handle the selected DTB. Rebuild and apply `falcon-sensor.dtbo` against both supported corrected bases, and test its graph endpoint after merge.

### Patch Dependency Graph

```text
binding: document AXC100, AXC200, and deprecated legacy form
  -> driver: add specific matches, complete resources, fault recovery, legacy gate
  -> DTS: migrate revision A to AXC100
  -> DTS: add corrected revision B AXC200 resources
  -> overlay: validate/revise sensor endpoint targeting
  -> update/recovery: enforce compatible artifact combinations
```

Bindings should be posted first in the series. The cover letter should state that the legacy ABI shipped, explain why AXC200 has no AXC100 fallback, and describe how recovery is coordinated.

### Validation Plan

Static checks:

- validate the targeted binding and its examples
- compile all affected DTBs with warning checks
- run targeted `dtbs_check`
- compile overlays with symbols and validate every supported merged tree
- inspect normalized DTB diffs for revision A before/after migration
- test invalid fixtures: AXC200 missing error IRQ, AXC200 missing reset, reversed names, legacy/new conflicts

Compatibility checks:

- legacy revision A DTB with old and new kernels
- corrected revision A DTB with allowed old/new kernels
- legacy revision B block/migration behavior
- corrected revision B with new kernel and deliberate old-kernel non-match
- A/B upgrade and rollback transitions
- unknown/corrupt EEPROM identity and wrong overlay selection

Hardware checks:

- capture at minimum, nominal, and maximum supported clock rates
- interrupt load and buffer pressure
- injected DMA fault followed by reset and successful recovery
- repeated fault and failed-reset behavior
- runtime PM and system suspend/resume
- warm reboot and recovery boot
- option X oscillator present, absent, and wrongly selected
- sensor overlay data path after final-tree composition

## ABI Decision Record

```text
hardware fact:
  AXC200 adds a mandatory error IRQ and core reset recovery contract.
why software needs it:
  DMA faults cannot be recovered safely through the AXC100 path.
why it is not discoverable:
  compatible matching and provider wiring are needed before safe operation.
chosen vocabulary:
  specific compatibles; standard interrupts/names, clocks/names, resets/names.
fallback:
  AXC200 does not fall back to AXC100 because lifecycle/error behavior is unsafe.
legacy behavior:
  shipped generic strings remain accepted only under defined revision-specific gates.
deployment:
  update recovery/guards, then corrected DTBs and production driver as a compatible set.
evidence:
  schema/DTB checks, merged-overlay checks, four-way version tests, injected-fault recovery.
```

## Completion Criteria

You have completed the lab when you can defend:

- every kept and removed property in terms of interface ownership
- the complete AXC100 and AXC200 resource contracts
- the decision not to provide a misleading fallback
- a migration for all shipped legacy DTBs
- safe A/B and recovery behavior
- a reviewable upstream dependency order
- static, compatibility, lifecycle, and negative test evidence

## Authoritative References

- [Linux binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux submitting Devicetree patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)

## Next Step

Continue with [Writing And Validating Binding Schemas](../writing-and-validating-binding-schemas.md), where this contract is encoded as YAML and exercised with schema tooling.
