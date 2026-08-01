---
status: active
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# 8. Runtime Diagnostics, Security, Production, And Board Porting

Official sources: Linux sysfs/driver-core/OF documentation, U-Boot verified/measured boot, architecture boot protocols, and upstream process documentation

Knowledge-guide companion: [Stage 8](knowledge-guide-companion.md#stage-8-runtime-diagnostics-security-production-and-board-porting)

## Runtime Tree Evidence

- [ ] **P0** Linux firmware/devicetree [sysfs ABI documentation](https://docs.kernel.org/admin-guide/abi.html) for the exact kernel.
- [ ] **P0** `/sys/firmware/fdt` when available as raw boot-blob evidence.
- [ ] **P0** `/sys/firmware/devicetree/base` as the live-tree filesystem export.
- [ ] **P0** `/proc/device-tree` only after resolving what it is on the target.
- [ ] **P0** Read properties as raw bytes: NUL-separated strings, empty booleans, big-endian cells, and byte arrays.
- [ ] **P0** Capture with `dtc -I fs` while understanding that original serialization/reservation information is not reconstructed.
- [ ] **P1** Compare packaged, pre-handoff, boot-blob, live-tree, and subsystem evidence at semantically valid checkpoints.

## Device And Probe Forensics

- [ ] **P0** `of_node`, bus device, modalias, module alias, driver symlink, and subsystem/class interface.
- [ ] **P0** Driver-core binding/unbinding rules and `driver_override` behavior from the exact bus.
- [ ] **P0** Deferred-probe reasons, device links, suppliers, and relevant kernel logs.
- [ ] **P0** Preserve evidence before rebind, module reload, overlay changes, or reboot.
- [ ] **P0** Test hardware operation and error counters; do not stop at a driver symlink.
- [ ] **P1** Use debugfs only as version/configuration-specific diagnostics, not a stable ABI.
- [ ] **P1** Define a safety case before manual bind/unbind of storage, console, network, DMA, power, or parent devices.

## Threat Model And Verified Boot

- [ ] **P0** Treat DTB/DTBO as hardware policy affecting MMIO, DMA, memory, interrupts, power, bootargs, and enabled devices.
- [ ] **P0** [U-Boot Verified Boot](https://docs.u-boot.org/en/stable/usage/fit/verified-boot.html).
- [ ] **P0** [FIT signature verification](https://docs.u-boot.org/en/stable/usage/fit/signature.html): hashes, signatures, required keys, and signed configurations.
- [ ] **P0** Authenticate selection and component relationships, not only individual payload bytes.
- [ ] **P0** Protect the verification key/control FDT through an earlier trust stage.
- [ ] **P0** Inventory and constrain every post-verification fixup or overlay.
- [ ] **P0** Close unsigned raw/legacy/recovery bypass paths.
- [ ] **P1** Define key separation, rotation, compromise response, and product scope.

## Measured Boot And Attestation

- [ ] **P0** [U-Boot measured boot](https://docs.u-boot.org/en/stable/usage/measured_boot.html).
- [ ] **P0** [Linux TPM documentation](https://docs.kernel.org/security/tpm/index.html) and event-log handling.
- [ ] **P0** PCR extension order, nonce-bound quote, event-log replay, and policy evaluation.
- [ ] **P0** Decide whether components, selected configuration, bootargs, fixup inputs, and/or final FDT are measured.
- [ ] **P0** Distinguish measured/known, authenticated/authorized, current, and healthy.
- [ ] **P1** Protect privacy in final-FDT and event-log evidence.

## Release Compatibility And Reproducibility

- [ ] **P0** Define an exact release set: boot firmware, kernel, DTB/DTBO, modules, initramfs/rootfs, and device firmware.
- [ ] **P0** Test old-DTB/new-kernel and every other compatibility direction promised by update policy.
- [ ] **P0** Separate functional A/B rollback from security anti-rollback.
- [ ] **P0** Pin source, patches, toolchain, dtc, dt-schema, flags, environment, outputs, and manifests.
- [ ] **P0** Use [SOURCE_DATE_EPOCH](https://reproducible-builds.org/specs/source-date-epoch/) appropriately without assuming it guarantees reproducibility.
- [ ] **P0** Independently rebuild unsigned payloads and compare exact plus semantic output.
- [ ] **P1** Sign or bind canonical release metadata and retain signing audit evidence.

## Field Updates And Recovery

- [ ] **P0** Authenticate update metadata before trusting offsets, sizes, identities, or versions.
- [ ] **P0** Write inactive slot, read back, verify, trial boot, run DT-aware health checks, then accept.
- [ ] **P0** Advance protected rollback state only after candidate and compatible recovery are proven.
- [ ] **P0** Test power loss at every durable state transition.
- [ ] **P0** Keep recovery authenticated, constrained, independently reachable, and compatible with the security floor.
- [ ] **P1** Use staged rollout, board/option cohorts, attested release identity, and automatic stop thresholds.

## Product Maintenance

- [ ] **P0** Maintain variant inventory, source-layer ownership, compatible/deprecation register, patch ledger, CI matrix, and hardware classes.
- [ ] **P0** Review changes separately for hardware correctness, ABI, style, maintainability, integration, and evidence.
- [ ] **P0** Build the full artifact inventory and use change impact/risk for deeper cross-version and hardware jobs.
- [ ] **P0** Convert escapes into schemas, assertions, tests, ownership, and release gates with owners and effectiveness checks.
- [ ] **P1** Track upstream/downstream divergence, patch age/status, semantic delta, and drop conditions.

## Board Porting Capstone

- [ ] **P0** Select the closest reference by boot-critical architecture and create a schematic/BOM delta ledger.
- [ ] **P0** Freeze reference artifact/runtime evidence and preserve recovery.
- [ ] **P0** Build a minimal DTB: root identity, firmware memory, console, boot storage, and essential suppliers.
- [ ] **P0** Prove build, package, bootloader selection, post-fixup working FDT, handoff, boot FDT, and live tree.
- [ ] **P0** Enable suppliers and one peripheral chain at a time.
- [ ] **P0** Validate memory/DMA/IOMMU/remoteproc ownership before starting remote/DMA masters.
- [ ] **P0** Engineer board identity, revisions, and options with safe unknown/corrupt behavior.
- [ ] **P0** Remove bring-up bypasses, upstream the contract, and create production/support evidence.

## Final Synthesis

- [ ] Build a complete evidence bundle for one board from source commit to live tree and functional subsystem results.
- [ ] Produce a trust/mutation diagram covering verifier, selected configuration, DTB, overlays, fixups, measurements, and recovery.
- [ ] Produce a release compatibility matrix and power-loss-safe update state machine.
- [ ] Complete a custom board port with warning-clean validation and an upstreamable patch series.
- [ ] Write one postmortem that converts a DT escape into permanent static, CI, hardware, and ownership controls.
- [ ] Record all project-specific P1/P2 items promoted to P0.

## Stage Completion

- [ ] I can prove which DTB and transformations reached Linux and how they affected device/probe state.
- [ ] I can secure and attest Device Tree selection without confusing integrity, authorization, measurement, or freshness.
- [ ] I can maintain compatible, reproducible release sets with safe update, rollback, and recovery.
- [ ] I can port and sustain a board using evidence gates rather than copied, unexplained DTS content.
