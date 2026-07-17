---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# Source And Binary Artifacts

## Why The Artifact Chain Matters

A correct edit to the correct source file can have no effect if the build produced another DTB, the deployment copied it to the wrong location, the bootloader selected a different file, or an overlay changed the result. Treat Device Tree as an artifact pipeline, not only as source code.

```text
SoC DTSI + module DTSI + board DTS + headers
                    |
              preprocessing
                    |
                    v
              complete DTS
                    |
                   dtc
                    |
                    v
                  DTB
                    |
       packaging, selection, and fixups
                    |
                    v
          bootloader-visible tree
                    |
              kernel handoff
                    |
                    v
             Linux runtime tree
```

Every arrow is a version boundary and a possible source of drift.

## DTS: Devicetree Source

A `.dts` file normally represents a complete board or machine description and is a build entry point.

```dts
/dts-v1/;

#include "example-soc.dtsi"

/ {
        model = "Example Trainer Board";
        compatible = "example,trainer-board", "example,trainer-soc";
};

&uart0 {
        status = "okay";
};
```

The file may be short because it composes reusable descriptions and adds only board-specific facts.

## DTSI: Reusable Source Fragments

A `.dtsi` file is source intended for inclusion. Typical layers include:

```text
SoC family DTSI
-> exact SoC DTSI
-> system-on-module DTSI
-> carrier or shared board-family DTSI
-> final board DTS
```

Good layering follows physical reuse. SoC-integrated controllers belong in the SoC description. A regulator physically located on a system-on-module belongs in the module description. A connector or peripheral fitted only to one carrier belongs in that board's source.

An SoC DTSI often describes controllers as present in silicon but unavailable until a board supplies wiring:

```dts
uart0: serial@1000 {
        compatible = "example,trainer-uart";
        reg = <0x1000 0x100>;
        status = "disabled";
};
```

The board enables and completes it:

```dts
&uart0 {
        pinctrl-0 = <&uart0_pins>;
        pinctrl-names = "default";
        status = "okay";
};
```

The resulting tree contains one merged node, not two runtime nodes.

## Includes Are Source Composition, Not Runtime Inheritance

Developers often say that a board “inherits” a SoC DTSI. This is useful shorthand but can mislead. Inclusion and `&label` amendments produce one source representation before compilation. The flattened tree does not preserve a class hierarchy or an include stack.

Two common inclusion mechanisms exist:

- `/include/ "file.dtsi"` is understood by `dtc`.
- `#include "file.dtsi"` and `#include <dt-bindings/...>` require C preprocessing in common kernel build flows.

The exact build rule determines which include paths and macros are available. A standalone `dtc board.dts` command may fail on a kernel DTS that expects preprocessing.

## DTB: The Flattened Binary

A `.dtb` is a flattened, pointerless binary encoding used to exchange a tree between software components. A bootloader commonly places it in memory and passes its address to the kernel.

The major regions are:

```text
+----------------------------+
| header                     |
+----------------------------+
| memory reservation block   |
+----------------------------+
| structure block            |
+----------------------------+
| strings block              |
+----------------------------+
```

Padding can exist between or after blocks. Header offsets locate each region; do not assume a hand-calculated fixed position.

### Header

The header records information such as:

- magic value identifying an FDT
- total blob size
- offsets and sizes of the structure and strings blocks
- offset of the memory reservation block
- flattened-format version compatibility fields
- boot CPU physical ID

The flattened format version describes the binary container, not the version of a board binding or DTS source.

### Memory Reservation Block

This block contains address/size pairs for physical memory that the client program must not use for general allocation. It is distinct from the logical `/reserved-memory` node, even though both concern reserved memory.

### Structure Block

The logical tree is serialized as aligned tokens such as begin-node, property, end-node, no-op, and end. Property values live here. Property names are referenced by offsets into the strings block.

### Strings Block

This block stores NUL-terminated property names. Reusing a name such as `compatible` across many nodes does not require embedding that name repeatedly in every structure entry.

Most engineers should understand these regions but use `libfdt`, `dtc`, `fdtdump`, `fdtget`, or bootloader commands rather than parsing raw bytes manually.

## DTBO: Overlay Binary

A `.dtbo` is a compiled overlay artifact. It describes changes targeted at a base tree rather than a complete bootable hardware description.

```text
base DTB + selected DTBO(s)
-> resolved references and applied fragments
-> effective tree
```

Overlay correctness depends on the base tree, exported symbols or target paths, application order, and the software applying it. An overlay that compiles is not guaranteed to apply to every base DTB version.

The detailed overlay mechanism belongs in [Overlays In Depth](../overlays-in-depth.md). At foundations level, remember that a DTBO is a transformation input, not simply a smaller DTB.

## What The Bootloader Can Change

Before Linux receives the blob, firmware or the bootloader may:

- select a DTB based on board identity
- apply overlays
- update installed memory size
- inject MAC addresses or serial numbers
- set `/chosen/bootargs`
- add initramfs bounds
- reserve memory
- disable unavailable components
- relocate or resize the blob

Therefore these are different objects:

```text
DTB produced by the build
!= DTB stored on boot media
!= tree after bootloader fixups
!= tree Linux exposes at runtime
```

They may be identical, but that must be demonstrated rather than assumed.

## How Linux Consumes The Blob

Linux can scan important flattened-tree data during early boot, including platform identity, memory, and `/chosen` configuration. It later unflattens the blob into its runtime representation and uses bus and firmware-node logic to create or describe devices.

Not every node becomes a `platform_device`:

- `/chosen` and `/aliases` carry information rather than ordinary devices
- an I2C child normally becomes an I2C client through its bus controller
- an SPI child is populated by SPI infrastructure
- graph or sound-card nodes may describe relationships rather than one physical component

Tree hierarchy and bindings determine interpretation.

## Artifact Identity Checklist

When an edit has no effect, record evidence at every boundary:

| Boundary | Evidence |
|---|---|
| source | Git commit and exact DTS/DTSI paths |
| build | command, output directory, timestamp, DTB hash |
| package | FIT contents, boot partition pathname, image manifest |
| selection | boot script, environment, firmware log, board-ID logic |
| pre-handoff | bootloader `fdt` inspection or dumped blob |
| runtime | `/sys/firmware/devicetree/base`, `/proc/device-tree`, boot log |

A DTB hash proves blob identity only at the point where it was measured. Boot-time fixups legitimately change the hash.

## Failure Scenarios

### Source Change Compiles But Is Absent At Runtime

Likely causes:

- another board target was built
- an out-of-tree build directory contains the actual output
- deployment copied an old file
- the bootloader selected another DTB
- a FIT image embeds a separate copy
- an overlay later deleted or replaced the property

### Decompilation Does Not Reproduce The Original Source

This is expected. Compilation removes comments, include boundaries, labels as source syntax, formatting, and macro names. Decompilation reconstructs an equivalent source-like view of the binary; it is not a source-code round trip.

### DTB Is Valid But Linux Does Not Boot

Binary structure validity does not prove semantic correctness. Incorrect memory ranges, CPU descriptions, console selection, reserved regions, or interrupt topology can break boot long before a peripheral driver logs an error.

## Review Questions

1. What information is lost when DTS becomes DTB?
2. Why is a DTSI not normally deployed by itself?
3. How does a DTBO differ from a complete DTB?
4. Which DTB identity does a build hash prove?
5. Why might standalone `dtc` fail on a DTS that builds inside the kernel tree?
6. How do the memory reservation block and `/reserved-memory` differ conceptually?

## References

- [Devicetree Specification releases](https://www.devicetree.org/specifications/)
- [Flattened Devicetree format](https://devicetree-specification.readthedocs.io/en/stable/flattened-format.html)
- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

## Next Step

Continue with [Tree Anatomy And Vocabulary](tree-anatomy-and-vocabulary.md).
