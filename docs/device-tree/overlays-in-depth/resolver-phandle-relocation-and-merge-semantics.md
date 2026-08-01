---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Resolver, Phandle Relocation, And Merge Semantics

The resolver converts a detached overlay's symbolic and local references into phandles valid in the target tree. The overlay engine or libfdt merge then applies its fragments. Resolution and application are related stages with different failures.

## Resolver Sequence

For the Linux live-tree resolver, the upstream algorithm is conceptually:

1. find the maximum phandle in the live tree and derive a relocation delta
2. add that delta to phandles defined inside the overlay
3. use `__local_fixups__` to add the same delta to every overlay-local reference
4. for each symbol named in `__fixups__`, find the matching live-tree `__symbols__` entry
5. obtain the target node's phandle
6. patch every recorded overlay property offset with that phandle

After this, the overlay is internally consistent with the target phandle namespace.

## Worked Phandle Example

Assume:

```text
maximum base phandle = 100
overlay local phandle for module_reg = 1
overlay property vdd-supply refers to local phandle 1
base symbol gpio1 has phandle 37
overlay reset-gpios contains external placeholder for gpio1
```

After relocation and resolution, conceptually:

```text
module_reg phandle        1 + 100 = 101
vdd-supply local ref      1 + 100 = 101
reset-gpios provider ref  external placeholder -> 37
```

Provider arguments following phandles—GPIO line 12 and flags, for example—remain arguments. Only cells recorded as fixup locations are patched.

Do not infer exact relocation values from this example for a product; existing phandle allocation and tool implementation determine them.

## Resolution Preconditions

- overlay blob is structurally valid and detached from the live tree
- metadata paths, properties, and byte offsets are valid
- every external symbol exists in the target's exported symbol table
- referenced target nodes can have valid phandles
- local phandle arithmetic does not overflow or collide
- application is serialized under the platform's supported mechanism

A missing target label is a resolver/linkage failure. A resolved overlay can still fail changeset creation, application, notifier handling, or device probe.

## Fragment Merge

For each resolved fragment:

- `target` contains a resolved phandle, or `target-path` identifies a path
- the target node must exist
- properties in `__overlay__` add or update target properties
- child nodes add or recursively merge by full node name
- overlay-local phandles and references now use the target namespace

Conceptual example:

```text
base /soc/spi@2000000:
  status = "disabled"

overlay:
  status = "okay"
  sensor@0 { ... }

result:
  status = "okay"
  sensor@0 { ... }
```

The final result contains no abstract relationship to “disabled before.” Pre-boot consumers see only the merged value.

## Same-Name Child Merge

If the base already contains `sensor@0`, an overlay child with the same full name merges into it. That may be intentional enablement:

```dts
/* Base owns complete but disabled hardware description. */
sensor@0 {
        compatible = "acme,temp100";
        reg = <0>;
        status = "disabled";
};
```

```dts
/* Overlay changes population state only. */
&sensor0 {
        status = "okay";
};
```

But a changed unit address or name creates a second node. Validate uniqueness and the physical assembly, not only merge success.

## Property Ownership And Previous Values

When several fragments/overlays update one property, later effective changes can replace earlier values. Record ownership:

| Path/property | base | revision overlay | module overlay | final owner |
|---|---|---|---|---|
| `/soc/spi@.../status` | disabled | okay | unchanged | revision |
| `/soc/spi@.../pinctrl-0` | header pins | alternate pins | module pins | conflict unless declared |
| `/aliases/spi-module` | absent | absent | target path | module |

Do not use application order as an undocumented conflict-resolution language. Reject multiple writers unless the layering contract explicitly authorizes replacement.

## Changeset Semantics In Linux

Linux converts resolved overlay content into an overlay changeset. Applying it updates the live tree and emits reconfiguration notifications after changes are made consistently. Device-population mechanisms can then create devices for active new nodes; removal/revert can depopulate them.

Tree atomicity is not whole-system transactionality. A post-apply notifier can fail after side effects begin, and drivers can start asynchronous work. Callers must follow the exact error and removal contract of their kernel version.

## Pre-Boot Flat-Blob Application

Host `fdtoverlay` or U-Boot/libfdt mutates a writable flat blob. It must have enough capacity and a failure-safe working copy. There is no Linux device lifecycle yet:

```bash
fdtoverlay -i base.dtb -o merged.dtb module.dtbo
```

This is ideal for deterministic CI composition. Compare its decoded semantic result with the bootloader-produced final FDT when both paths matter.

An application error does not authorize booting an uncertain buffer. U-Boot documents that failed application can invalidate both base and overlay blobs; preserve pristine/reloadable inputs.

## Deletion And Reversion Distinction

Three operations are often confused:

- source deletion directives modify the source tree being compiled
- applying an ordinary overlay adds/updates described content
- removing that applied overlay reverts its recorded changeset, subject to dependencies and lifecycle rules

Reversion restores the state tracked for that overlay. It is not a general-purpose request to delete arbitrary base-tree data.

## Failure Localization

| Failure | Stage | Evidence |
|---|---|---|
| symbol not found | external resolution | `__fixups__`, target `__symbols__` |
| malformed local reference | local relocation | `__local_fixups__`, DTBO dump |
| target path absent | fragment targeting | base dump and target path |
| no space in working FDT | flat-blob application | totalsize/capacity and libfdt status |
| property appears but driver absent | post-merge matching/population | final tree, modalias, logs |
| probe defers | provider/lifecycle | final provider links and probe logs |
| removal rejected | stack/lifecycle | overlay dependency and notifier/device state |

Always save or inspect the last trustworthy checkpoint.

## Semantic Postconditions

After application, verify more than the target node:

- compatible and `status`
- correct address and unique bus child
- clocks, resets, supplies, GPIOs, IRQs, DMA, and IOMMU references
- provider cell counts and names
- pin ownership and electrical constraints
- aliases and chosen data if intentionally affected
- reserved-memory and address-window non-overlap
- graph endpoint reciprocity
- schema validation of the merged tree

Resolution can succeed while every one of these is wrong.

## Authoritative References

- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Changesets](https://docs.kernel.org/devicetree/changesets.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)

## Continue

Proceed to [Base Compatibility, Versioning, And Overlay ABI](base-compatibility-versioning-and-overlay-abi.md).
