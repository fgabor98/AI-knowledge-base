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

Derive the capacity from the worst-case supported overlay set plus later fixups, as described in [Libfdt Capacity, Relocation, And Failure Atomicity](../boot-time-mutation-and-ownership/libfdt-capacity-relocation-and-failure-atomicity.md#calculate-capacity), and test the largest supported combination.

## Order And Compatibility Are Product Policy

Overlays can replace the same property, delete content, or reference nodes introduced by earlier overlays, so order changes the result. Derive one canonical order from an authenticated layer model ([Overlay Order, Composition, And Conflict Ownership](../boot-time-mutation-and-ownership/overlay-order-composition-and-conflict-ownership.md)) and reject unsupported combinations before mutation; never rely on directory or filesystem order. The overlay format itself carries no base-compatibility policy, so a manifest must ([Base Compatibility, Versioning, And Overlay ABI](../overlays-in-depth/base-compatibility-versioning-and-overlay-abi.md)). Because an isolated overlay lacks its target context, CI must apply every supported combination to its base and validate the merged tree.

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

Libfdt resolves phandles and targets but does not understand electrical conflicts: a structurally valid merged tree can double-assign GPIOs, chip selects, pins, `reg` windows, or aliases. Validate the final tree against the [hardware conflict inventory](../overlays-in-depth/stacking-dependencies-conflicts-and-removal-order.md#hardware-conflict-inventory).

## Authentication And Measured State

An overlay changes the hardware contract visible to Linux, so authenticate it, the overlay list, its order, and the board-identity mapping under the same trust policy as the base DTB. If measured boot records only the base DTB, attestation does not identify the final hardware description; measure the ordered inputs and the final serialized tree ([Measurement And Provenance](../overlays-in-depth/validation-security-and-product-architecture.md#measurement-and-provenance)).

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
