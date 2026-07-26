---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Bootloader Overlay Application And Working-FDT Safety

U-Boot can apply DT overlays before booting Linux. Correct application requires a compatible base, symbol/fixup metadata, writable space, deterministic order, authentication, and all-or-nothing failure policy. A successful `fdt apply` proves only that libfdt could merge the structures.

## Base And Overlay Build Requirements

An overlay normally uses fragment targets and fixups resolved against symbols in the base:

```dts
/dts-v1/;
/plugin/;

&spi2 {
        status = "okay";

        sensor@0 {
                compatible = "vendor,product-sensor";
                reg = <0>;
        };
};
```

Compile the base with symbol information when label-based targets require it, and compile the overlay as a plugin. The resulting DTB/DTBO metadata can include `__symbols__`, `__fixups__`, and local-fixup data.

Labels are source/build interfaces used to produce fixups; Linux ultimately consumes phandles and paths. Changing or removing a base label can break independently distributed overlays even when the hardware node path remains.

## Work On The Working FDT

Set and inspect the OS-bound working tree:

```text
=> fdt addr ${fdt_addr_r}
=> fdt header
=> fdt print /model
```

Do not apply product overlays to the live control FDT. U-Boot's driver model has already consumed it, and changing it does not reliably rebind U-Boot devices.

Keep the base DTB, overlay blobs, kernel, initrd, U-Boot runtime, and decompression destinations in nonoverlapping memory.

## Reserve Writable Space

Applying an overlay grows the base blob. Use the supported resizing operation with a capacity derived from actual artifacts plus margin:

```text
=> fdt resize 0x10000
=> fdt apply ${overlay_addr_r}
```

The command syntax and semantics depend on U-Boot version; the documented `fdt` command is authoritative.

The number passed is additional space in current U-Boot behavior, not a guarantee that surrounding RAM is unused. Address planning must reserve the entire expanded interval.

Budget:

```text
base totalsize
  + sum of overlay structural/string growth
  + bootloader fixups
  + future bounded margin
```

Test the largest supported overlay combination.

## Apply Order Is Part Of The Product

Overlays can:

- add independent nodes
- enable or disable a common node
- replace the same property
- delete nodes or properties
- reference nodes introduced by an earlier overlay

Therefore order can change the result. Define layers such as:

```text
base board
  -> board revision
  -> factory-fitted module
  -> field expansion
  -> deployment policy
```

Reject contradictory or unsupported combinations before mutation. Do not rely on directory enumeration order, filesystem order, or a list editable without authentication.

## Validate Compatibility Before Applying

An overlay manifest should constrain:

- base board family
- base DT ABI/version
- board revision range
- required/forbidden overlays
- kernel compatibility
- firmware dependencies
- resource conflicts
- ordering
- signature and rollback state

The Devicetree overlay format itself does not provide this full product compatibility policy.

Compile-time schema validation of an isolated overlay is limited because its target context is absent. In CI, apply every supported combination to its base, then validate the merged tree.

## Failure Is Not Transactional Recovery

U-Boot's overlay documentation warns that after an application error, both base and overlay blobs can be invalidated. Do not continue booting a partially changed or uncertain tree.

Safe pattern:

1. load and authenticate base and every selected overlay
2. validate manifest compatibility and calculate space
3. keep pristine source artifacts or reload capability
4. copy the base to a dedicated working buffer
5. resize once
6. apply in fixed order, checking every return status
7. validate required nodes/properties
8. on any failure, discard the working buffer and enter defined fallback/recovery

Do not “undo” by applying another overlay unless that exact transition is designed and tested.

## Phandles And Resource Conflicts

Libfdt can resolve local phandles and target references, but it does not understand electrical conflicts. A structurally valid merged tree can:

- drive one GPIO from two consumers
- enable mutually exclusive pin states
- assign one chip select twice
- overlap `reg` windows
- enable devices needing incompatible voltages
- duplicate aliases
- violate provider cell counts or schemas

Validate the final tree and apply product-specific conflict checks.

## Authentication And Measured State

An overlay changes the hardware contract visible to Linux. Authenticate it before application under the same trust policy as the base DTB. Protect the overlay list, ordering, and board-identity mapping too.

If measured boot records only the base DTB, attestation does not identify the final hardware description. Measure either:

- each ordered, authenticated input and selection manifest
- the final serialized DTB after authorized mutations
- preferably both, so input provenance and resulting state are available

Define whether field-installed hardware can legitimately change measurements.

## Inspect The Result

Before boot:

```text
=> fdt print /model
=> fdt print /chosen
=> fdt print /soc/spi@...
=> fdt get value status /soc/spi@... status
```

Host-side CI:

```sh
fdtoverlay -i base.dtb -o merged.dtb revision.dtbo module.dtbo
dtc -I dtb -O dts -o merged.dts merged.dtb
```

Then run schema validation and semantic checks. Compare the host result with the U-Boot-produced final DTB for representative combinations.

## Authoritative References

- [U-Boot Device Tree overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [U-Boot `fdt` command](https://docs.u-boot.org/en/latest/usage/cmd/fdt.html)
- [Devicetree overlay notes in the Linux kernel](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to [Binman, Firmware Packaging, And DT-Based Image Layout](binman-firmware-packaging-and-dt-based-image-layout.md).
