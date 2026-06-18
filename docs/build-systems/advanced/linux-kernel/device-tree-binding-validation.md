---
status: draft
reviewed: false
domain: build-systems
difficulty: advanced
reviewer: null
last_reviewed: null
---

# Device Tree Binding Validation

## What Problem Does This Solve?

Device tree binding validation checks whether a DTS describes hardware according to the schemas expected by kernel drivers and subsystem maintainers.

For embedded Linux, this matters because a DTB can compile successfully and still be wrong. Binding validation catches many errors before you boot the board.

## Core Concepts

- DTS
- DTSI
- DTB
- YAML binding
- compatible string
- required property
- `dtbs_check`
- `dt_binding_check`
- schema warning
- runtime probe

## Mental Model

Device tree validation has layers:

```text
DTS syntax
-> DTB compilation
-> binding schema validation
-> runtime driver probe
-> functional hardware test
```

Passing one layer does not guarantee the next layer.

## Why Compilation Is Not Enough

This can compile:

```dts
ethernet@8000000 {
    compatible = "vendor,my-ethernet";
    reg = <0x8000000 0x1000>;
};
```

but the driver binding may require:

- clocks
- resets
- interrupts
- PHY handle
- pinctrl references
- power domains
- DMA properties

The compiler checks syntax. Bindings check structure and meaning.

## Binding Files

Binding schemas live in the kernel tree under:

```text
Documentation/devicetree/bindings/
```

Examples:

```text
Documentation/devicetree/bindings/net/
Documentation/devicetree/bindings/i2c/
Documentation/devicetree/bindings/spi/
Documentation/devicetree/bindings/gpio/
Documentation/devicetree/bindings/arm/ti/
```

A binding describes:

- compatible strings
- required properties
- optional properties
- property types
- child node rules
- examples

## Running Checks

Build DTBs:

```sh
make O=build ARCH=arm64 dtbs
```

Run binding checks:

```sh
make O=build ARCH=arm64 dtbs_check
```

Run binding schema checks:

```sh
make O=build ARCH=arm64 dt_binding_check
```

Some environments require Python dependencies for schema validation. In Yocto or SDK builds, those dependencies may be handled by the build environment.

## Checking One DTB

Kernel versions differ in exact variables, but many support narrowing the target:

```sh
make O=build ARCH=arm64 DT_SCHEMA_FILES=Documentation/devicetree/bindings/net/ti,davinci-mdio.yaml dtbs_check
```

You can also build a specific DTB target:

```sh
make O=build ARCH=arm64 ti/k3-am625-sk.dtb
```

Use the exact path style used by the architecture DTS Makefile.

## Understanding Warnings

Common warning categories:

- required property missing
- property has wrong type
- property is not allowed by schema
- compatible string has no schema
- node name does not match expected pattern
- interrupt cells are wrong
- clock or reset property has wrong number of entries

Not every warning blocks boot, but every warning deserves classification.

## Vendor Bindings

Vendor BSPs may contain:

- downstream bindings not yet upstream
- older text bindings
- partially converted YAML bindings
- warnings inherited from vendor DTS files

For product work, track whether a warning is:

- inherited from vendor baseline
- introduced by product DTS changes
- harmless but understood
- a real bug
- blocked by missing downstream schema support

Do not allow new product warnings to disappear inside a large inherited warning set.

## Compatible Strings

Compatible strings are the contract between DTS and drivers.

Example:

```dts
compatible = "ti,am625-sk", "ti,am625";
```

Ordering matters. Usually the most specific compatible comes first, followed by broader fallback compatibles.

Problems:

- typo in vendor prefix
- wrong SoC compatible
- missing board-specific compatible
- using a compatible for similar but not identical hardware
- driver matches but binding expects different properties

## Runtime Verification

After boot, inspect what Linux actually received:

```sh
cat /proc/device-tree/compatible
find /proc/device-tree -maxdepth 3 -name compatible -print
```

Check probe logs:

```sh
dmesg | grep -i 'of:'
dmesg | grep -i 'probe'
dmesg | grep -i 'deferred'
```

Runtime problems often show as:

- probe deferral
- missing regulator
- missing clock
- invalid interrupt
- PHY not found
- pinctrl lookup failure

## U-Boot Interaction

U-Boot can load, patch, or replace the DTB before Linux sees it.

Check:

- DTB filename loaded by boot script
- overlays applied by U-Boot
- fixups applied by U-Boot
- FIT image configuration selected
- runtime `/proc/device-tree`

A source DTS can be correct while the deployed runtime DTB is old or modified.

## Overlay Validation

Overlays add another layer:

```text
base DTS
+ overlay
-> runtime tree
```

Validate:

- overlay compiles
- overlay target paths or labels exist
- overlay compatible matches board policy
- bootloader applies overlay
- runtime tree contains expected nodes

## CI Policy

Useful CI checks:

- build all product DTBs
- run `dtbs_check`
- fail on new warnings
- archive warning logs
- compare warnings against an accepted baseline
- validate overlays if used
- verify deployed DTB checksum in image tests

For vendor BSPs with existing warnings, keep a baseline log and require product changes not to add new warnings.

## Common Mistakes

- Treating `dtc` success as correctness.
- Ignoring binding warnings because the board boots.
- Editing included DTSI files without checking other boards.
- Validating source DTS but deploying an old DTB.
- Forgetting U-Boot overlays or fixups.
- Using a compatible string copied from similar hardware without checking binding requirements.
- Ignoring probe deferral logs.

## Debugging Checklist

- Identify the source DTS.
- Identify included DTSI files.
- Build the target DTB.
- Run `dtbs_check`.
- Check binding file for the compatible.
- Check required properties.
- Check deployed DTB checksum.
- Check U-Boot selected DTB or FIT config.
- Check runtime `/proc/device-tree`.
- Check probe logs.

## Related Topics

- [Device Tree Builds](device-tree-builds.md)
- [Debugging Kernel Builds](debugging-kernel-builds.md)
- [Vendor Kernel Patch Management](vendor-kernel-patch-management.md)
- [Boot Debugging and Runtime Validation](../bsp-integration/boot-debugging-and-runtime-validation.md)

## References

- Linux kernel devicetree documentation
- Linux kernel binding schemas
- Devicetree specification
- Device tree compiler documentation
