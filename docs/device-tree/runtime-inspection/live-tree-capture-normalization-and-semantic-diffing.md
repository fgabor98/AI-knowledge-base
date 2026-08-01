---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Live-Tree Capture, Normalization, And Semantic Diffing

A runtime filesystem tree can be re-encoded into a diagnostic DTB or DTS. That capture preserves live nodes and properties, but not the original FDT serialization, padding, header, or reservation map. Use it for semantic comparison, not exact provenance.

## Capture The Live Tree

```bash
dtroot=/sys/firmware/devicetree/base

sudo dtc -I fs -O dtb -o live-tree.dtb "$dtroot"
sudo dtc -I fs -O dts -o live-tree.dts "$dtroot"

sha256sum live-tree.dtb
```

The resulting hash identifies this reconstructed diagnostic serialization under this `dtc` version/flags. It is not the hash of the original boot FDT.

Capture tool version:

```bash
dtc --version
```

Runtime reads are not necessarily an atomic snapshot if overlays or other live-tree changes occur concurrently. Quiesce the mutation mechanism or record the capture boundary.

## Preserve Raw Boot Blob Separately

```bash
sudo cp -- /sys/firmware/fdt boot-fdt.dtb
sha256sum boot-fdt.dtb live-tree.dtb
```

Different hashes are expected even with equal node/property semantics because reconstruction changes serialization and excludes original reservation/header/padding details. Decode both with the same tool for semantic comparison.

## Normalize For Diff

```bash
dtc -s -I dtb -O dts -o boot.sorted.dts boot-fdt.dtb
dtc -s -I dtb -O dts -o live.sorted.dts live-tree.dtb
diff -u boot.sorted.dts live.sorted.dts
```

`-s` sorting availability depends on the installed `dtc`. If unavailable, use a documented normalization tool or focused property comparisons.

Keep unnormalized captures. Normalization can hide ordering and metadata relevant to overlay debugging.

## Compare Adjacent Checkpoints

Recommended order:

```text
built DTB
  vs packaged/extracted DTB
packaged base + overlays
  vs bootloader post-overlay capture
post-overlay capture
  vs pre-handoff capture
pre-handoff capture
  vs /sys/firmware/fdt
boot FDT semantics
  vs live filesystem semantics
```

The first unexplained difference identifies the responsible stage. Comparing source DTS directly with live-tree DTS mixes preprocessing, flattening, packaging, selection, overlay, fixup, and runtime effects.

## Expected Differences

Bootloader/firmware may legitimately change:

- `/memory` banks
- `/reserved-memory` or reservation map
- `/chosen/bootargs`
- initrd bounds
- stdout/console path
- random/KASLR seeds
- MAC addresses and product identity
- status/topology selected from authenticated board data

Linux live overlays can later add/update nodes and properties. Each difference still needs a named owner, input, validation rule, and lifecycle.

Do not publish sensitive `/chosen` seed values in diffs.

## Reservation Blind Spot

`dtc -I fs` walks node/property files. The original FDT memory reservation block is not a normal node and cannot be reconstructed from `/sys/firmware/devicetree/base` alone.

For memory forensics, preserve:

- raw boot FDT or U-Boot capture
- decoded reservation map
- live `/reserved-memory` nodes
- kernel early memory/reservation logs
- `/proc/iomem` and subsystem ownership evidence

These views have related but non-identical semantics.

## Focused Semantic Assertions

Whole-tree diffs are noisy. Query important invariants:

```bash
fdtget live-tree.dtb / model
fdtget live-tree.dtb / compatible
fdtget live-tree.dtb /chosen stdout-path
fdtget live-tree.dtb /soc/device@48000000 status
fdtget -tx live-tree.dtb /soc/device@48000000 clocks
```

Use exact node paths discovered from the capture. Build a table of expected values and owners.

## Phandle Normalization

Phandle numbers can change between separately compiled or composed trees while relationships remain equivalent. A raw DTS diff can therefore show many numeric changes.

For relationship-aware comparison:

1. resolve each consumer phandle to its provider node path
2. decode arguments with provider cells/binding
3. compare `(consumer property/name -> provider path + semantic args)`

Do not normalize phandles away when diagnosing corrupt references.

## Source-To-Live Attribution

To map a live difference back to source:

- find exact live node path/compatible
- inspect built DTB and preprocessed DTS
- search labels and compatible strings in source
- identify owning `.dts`/`.dtsi` amendment
- check overlay fragments and bootloader fixup code
- check runtime overlay/change notifier path

Decompiled live DTS contains no reliable original include line information. Attribution requires build records.

## Capture Consistency

For a field bundle:

```text
timestamp/boot ID
kernel release/build ID
dtc version
raw /sys/firmware/fdt hash/copy if available
live-tree reconstructed DTB and decoded DTS
root identity and /chosen redacted summary
target raw property files
bootloader pre-handoff hash/capture
overlay/runtime mutation log
```

Use `/proc/sys/kernel/random/boot_id` as a boot-instance identifier when available; it is not an artifact version.

## Common Diff Traps

- byte-comparing boot FDT with `dtc -I fs` output
- assuming sorted decompilation is canonical across tool versions
- losing the FDT reservation block
- treating phandle renumbering as relationship change
- ignoring runtime overlay timing during capture
- comparing source DTS with live flattened tree without preprocessing
- dumping secrets from `/chosen`
- attributing every difference to Linux instead of pre-handoff fixups

## Authoritative References

- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux sysfs firmware OF ABI](https://docs.kernel.org/admin-guide/abi-testing-files.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [Linux Devicetree overlay notes](https://docs.kernel.org/devicetree/overlay-notes.html)

## Continue

Proceed to [From Live Device Tree Node To Linux Device](from-live-device-tree-node-to-linux-device.md).
