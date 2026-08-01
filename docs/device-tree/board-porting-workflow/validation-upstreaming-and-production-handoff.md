---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Validation, Upstreaming, And Production Handoff

Bring-up ends when the board is a maintained platform, not when the last peripheral probes. Close every temporary assumption, validate the full artifact and hardware matrix, organize changes for upstream review, and hand production an exact release, recovery, identity, and evidence contract.

## Reconcile The Delta Ledger

Every initial hardware delta must end as:

```text
implemented and proven
not applicable with hardware evidence
owned by boot firmware/secure firmware with interface evidence
deferred feature with node disabled and tracked scope
rejected design assumption with reason
```

No `COPIED`, `INFERRED`, or `CONFLICT` item should silently ship.

## Run Validation In Layers

### Binding and source

- new compatibles/properties documented in schema
- schema and examples pass `dt_binding_check`
- style and include organization reviewed
- no Linux-driver policy encoded as hardware

### Complete artifact set

- all board DTBs and DTBOs build with the chosen warning policy
- `dtbs_check` covers applicable schemas
- every allowed overlay composition succeeds
- forbidden compositions are rejected
- exact artifact inventory, sizes, and hashes match packaging

### Semantic review

- normalized before/reference/custom diffs
- root identity, memory, chosen, aliases
- enabled/disabled node inventory
- address/interrupt/power/clock/reset/pinctrl/provider changes
- reserved-memory and DMA/IOMMU map
- unexpected inherited reference-board nodes

### Boot-chain verification

- correct signed/selected artifact
- bootloader fixups and overlay order
- non-overlapping load/handoff regions
- final handoff identity/digest
- live-tree comparison

### Runtime and hardware

- no critical deferred probes or resource errors
- expected drivers and subsystem interfaces
- cold/warm/repeated boot
- stress, error recovery, suspend/resume/poweroff as required
- update, rollback, and authenticated recovery
- board revision/option matrix

## Use Targeted And Full Checks

Illustrative kernel-tree commands:

```bash
make dt_binding_check DT_SCHEMA_FILES=/acme/
make ARCH=arm64 dtbs_check DT_SCHEMA_FILES=/acme/
make ARCH=arm64 W=1 acme/axc300-revb.dtb
scripts/checkpatch.pl --strict 000*.patch
scripts/get_maintainer.pl 0001-*.patch
```

Targeted checks shorten iteration; full architecture/family jobs catch unintended effects. Record exact commands and versions.

## Remove Bring-Up Scaffolding

Audit and either remove or formally justify:

- temporary bootargs and maximum log level
- `ignore_loglevel`, initcall debugging, early console dependency
- manual U-Boot `fdt set` commands
- hardcoded test MAC/serial/calibration values
- disabled IOMMU or integrity/security policy
- global warning/schema exclusions
- forced regulators/clocks that mask missing consumers
- reduced bus rates and disabled performance states
- debug GPIO exports or userspace pokes
- unsigned/raw artifact fallback

If conservative settings remain for signal integrity or safety, document them as the real product limit with evidence.

## Structure An Upstreamable Series

A typical shape:

```text
1. dt-bindings: document new device/board compatibles
2. dt-bindings: add constants if necessary
3. subsystem: add driver support preserving old bindings
4. arm64: dts: vendor: add SoC/package data
5. arm64: dts: vendor: add module/carrier common hardware
6. arm64: dts: vendor: add custom board
7. arm64: dts: vendor: enable later board features in logical patches
```

Binding/driver and DTS patches may travel through different maintainer trees. Keep dependencies explicit, use documented fallback compatibility only when real, and make intermediate patches buildable/bisectable.

## Write Board Patches For Review

Commit messages should state:

- physical hardware/change being described
- board/revision identity
- important topology and reference differences
- why compatible fallback is valid
- boot/peripheral functionality reached
- validation performed
- dependency on binding/driver series

Do not post a monolithic “add board support” patch containing unrelated bindings, driver hacks, generated binaries, and formatting churn.

## Define Product Qualification

Create a signed-off result per supported variant:

| Area | Required evidence |
|---|---|
| identity/selection | OTP/EEPROM input -> signed config -> live compatible |
| boot/recovery | repeated cold/warm boots, failed-slot recovery |
| memory | ownership map, reservations, stress/ECC if present |
| storage | read/write integrity, modes, power loss/update |
| network | link modes, throughput, errors, cycling |
| power/thermal | rails/sequencing, load, trips/cooling |
| DMA/IOMMU | mappings, faults, stress/isolation |
| remote cores | firmware identity, IPC, restart/crash |
| PM | suspend/resume/poweroff/wake where supported |
| options | every allowed overlay/composition and negative pair |

## Create The Production Handoff Bundle

Include:

```text
hardware revision/BOM/schematic references
final delta ledger and ownership
source commits and patch/upstream status
toolchain and build provenance
DTB/DTBO/FIT/release manifests and hashes
supported kernel/bootloader/firmware compatibility
identity and selection policy
bootloader fixup/overlay ledger
memory/DMA ownership map
CI and hardware qualification evidence
known limitations and disabled features
update/rollback/recovery procedure
support-bundle collection procedure
security/signing/measurement policy
```

Production should not depend on the bring-up engineer's shell history.

## Prepare Support Evidence

Automate read-only collection of:

- unit product/revision/option identity
- release-set and artifact manifest identity
- bootloader/kernel/module/firmware versions
- boot FDT and live-tree identity/capture per policy
- target device/of_node/driver relationships
- deferred probes and relevant logs
- memory reservations and IOMMU/remoteproc state
- subsystem health/error counters
- reboot/rollback reason

Protect serials, keys, seeds, memory contents, and customer data.

## Define Regression Gates

Every later change affecting common DTSI, schema, driver, toolchain, boot fixup, packaging, or selector must trigger:

- transitive artifact rebuild
- semantic diff for this board
- declared overlay compositions
- compatibility directions promised for the product
- risk-selected hardware classes
- release manifest update if bytes change

The new board becomes part of the maintained matrix immediately, not after the first regression.

## Final Exit Gate

```text
[ ] hardware delta ledger has no unexplained state
[ ] schema/style/build/semantic/artifact checks pass
[ ] final packaged and runtime trees match intended composition
[ ] all supported revisions/options qualify at required depth
[ ] temporary debug and security bypasses are removed
[ ] upstream/downstream series and ownership are recorded
[ ] production/recovery artifacts are authenticated and compatible
[ ] support evidence is automated and privacy-reviewed
[ ] regression impact rules include the new board
[ ] another engineer can reproduce, diagnose, update, and recover it
```

## Further Reading

- [Submitting Devicetree binding patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Upstream Development And Downstream Patch-Stack Discipline](../product-scale-maintenance-and-engineering/upstream-development-and-downstream-patch-stack-discipline.md)
- [Secure Device Tree Release And Update Lab](../security-and-production-lifecycle/secure-device-tree-release-and-update-lab.md)
- [Custom Board Porting Capstone Lab](custom-board-porting-capstone-lab.md)
