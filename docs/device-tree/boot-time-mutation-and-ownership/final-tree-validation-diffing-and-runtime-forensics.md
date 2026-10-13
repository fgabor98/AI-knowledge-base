---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Final-Tree Validation, Diffing, And Runtime Forensics

Source validation proves only the compiled base. A mutation pipeline needs validation at checkpoints and at the final Linux handoff. Field diagnosis depends on distinguishing meaningful changes from phandle renumbering, padding, ephemeral seeds, and kernel-consumed properties.

## Preserve Boundary Artifacts

Capture the checkpoints listed in [Checkpoint Strategy](mutation-provenance-authorities-and-checkpoints.md#checkpoint-strategy). In production, storage may permit only hashes and a mutation manifest; provide a privileged service workflow for retrieving sanitized artifacts when needed. The most valuable artifact is the FDT immediately before jumping to Linux, because it separates bootloader behavior from kernel interpretation.

## Structural Validation

At every binary boundary:

```sh
fdtget tree.dtb / compatible
fdtdump tree.dtb
dtc -I dtb -O dts -o tree.dts tree.dtb
```

Check:

- magic/version/header and total size
- reservation map termination
- valid structure and strings blocks
- property/node encoding
- phandle consistency
- no operation returned a libfdt error

Use host tools from a controlled version and keep the original binary for evidence.

## Schema Validation Of Final Compositions

Run schema validation on:

- base DTBs
- every supported overlay composition
- representative firmware/board fixup outputs
- final trees with deterministic test values

Runtime-only values such as per-boot seeds make byte-for-byte fixtures unsuitable. Use a test mutator with deterministic injected inputs, or redact only schema-approved ephemeral values before comparison.

A live production tree can be copied to a host and checked, but schema versions must match the kernel/platform vintage being investigated.

## Semantic Diff

Decompiled textual diff contains noise:

- phandle number changes
- node/property ordering
- symbol/fixup metadata
- padding/packing
- generated seed bytes
- bootargs and addresses expected to vary

Build a semantic path/property diff:

For an AM62x SK booted by TI U-Boot with the HDMI audio overlay, the attributed diff from host-built base to `/sys/firmware/fdt` looks like this (see the [lab](boot-time-mutation-provenance-lab.md)):

```text
added    /sound-sii9022                                   overlay k3-am62x-sk-hdmi-audio
changed  /sound/status okay -> disabled                   overlay k3-am62x-sk-hdmi-audio
added    /chosen/bootargs, /chosen/u-boot,version         fdt_chosen()
changed  .../ethernet-ports/port@1/mac-address [redacted] fdt_fixup_ethernet()
added    .../ethernet-ports/port@1/local-mac-address      fdt_fixup_ethernet()
changed  /thermal-zones/*/trips/*-crit/temperature        fdt_fixup_thermal_critical_trips_k3()
replaced /reserved-memory/tfa@..., optee@...              fdt_fixup_reserved()
```

Maintain an allowlist by mutation ID, not a broad ignore list. Unexpected changes outside the owner's path set fail CI.

## Capture From U-Boot And Linux

Export exactly the header `totalsize` bytes of the working FDT using the procedure in [Capture The Pre-Handoff FDT](../build-and-diagnostic-tools/u-boot-fdt-commands-and-handoff-inspection.md#capture-the-pre-handoff-fdt), and record its address before and after `bootm` relocation; the tree inspected earlier can differ from the tree actually passed. On the Linux side, capture `/sys/firmware/fdt` and the live tree as described in [Runtime Tree Surfaces And Boot-FDT Identity](../runtime-inspection/runtime-tree-surfaces-and-boot-fdt-identity.md); some handoff properties can be consumed, cleared, or hidden, so the live tree is not guaranteed to match the input blob.

## Triangulate With Subsystem Evidence

DT says what was described; runtime state says what Linux accepted:

- `/proc/iomem` for memory/reservations
- `ip link` for MAC selection
- `/proc/cmdline` for effective bootargs
- console logs for `stdout-path`/console
- driver bind paths and modaliases
- regulator/clock/IOMMU debug evidence

A correct final tree can still fail because the kernel driver lacks support. A working driver can hide an incorrect tree by retaining boot-firmware state.

## Mutation Bisection

If the final tree is wrong:

1. identify the first checkpoint containing the wrong value
2. list transforms between the last good and first bad checkpoints
3. replay them with captured inputs
4. disable or isolate only one transform in a lab build
5. compare return codes and postconditions

Do not edit the base DTS when the first bad checkpoint proves an overlay or board fixup owns the property.

## Field Provenance Bundle

Include:

- hardware/product normalized identity
- boot reason and selected slot/configuration
- firmware/U-Boot/kernel versions
- base and overlay hashes
- mutation event list
- final hardware-composition and final-tree hashes
- memory map summary
- redacted semantic diff
- first error/status from each failed transform

Sign or authenticate bundles when used for security decisions. Protect serials, MACs, command lines, and logs according to privacy policy.

## Authoritative References

- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux dynamic debug](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
- [Linux debugfs](https://docs.kernel.org/filesystems/debugfs.html)

## Continue

Proceed to [Boot-Time Mutation Provenance Lab](boot-time-mutation-provenance-lab.md).
