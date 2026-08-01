---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Symbols, Fixups, Local Fixups, And Compilation

An overlay DTBO is relocatable because the compiler records where unresolved external phandles and overlay-local phandles occur. The resolver consumes this metadata; drivers should never depend on it after the final tree is constructed.

## Build The Base For External Symbols

```bash
dtc -@ -I dts -O dtb -o base.dtb base.dts
```

`-@` asks `dtc` to emit symbol information for labels. A simplified decoded base can contain:

```dts
__symbols__ {
        spi2 = "/soc/spi@2000000";
        gpio1 = "/soc/gpio@2100000";
};
```

The property name is the source label and its string value is the node path. The exact phandle values assigned during compilation are implementation details; the exported label-to-path mapping is the useful overlay interface.

Not every build invokes standalone `dtc`. Kernel and bootloader makefiles may add overlay-specific flags. Inspect the actual command and artifact rather than assuming a source suffix enables symbols.

## Compile The Overlay As A Plugin

```bash
dtc -@ -I dts -O dtb -o module.dtbo module.dtso
```

The source needs both:

```dts
/dts-v1/;
/plugin/;
```

`/plugin/` permits unresolved external references and causes the compiler to describe how they must be fixed later. `-@` preserves the symbol/fixup information needed for overlay linkage, including labels exported by the overlay for later overlays.

## `__symbols__`

In a base DTB, `__symbols__` makes source labels available to overlays. In an overlay, `__symbols__` can describe labels introduced by that overlay:

```dts
module_reg: regulator-module {
        compatible = "regulator-fixed";
        ...
};
```

A later stacked overlay may reference `&module_reg` after the first overlay has been resolved and applied. This creates an explicit stack dependency; the first overlay cannot be removed while the later overlay depends on it.

Treat exported overlay labels as ABI if other artifacts ship independently.

## `__fixups__`: External References

Given:

```dts
&spi2 {
        reset-gpios = <&gpio1 12 1>;
};
```

`spi2` and `gpio1` do not exist inside the detached overlay. The compiled DTBO includes `__fixups__` properties keyed by those external symbol names. Each value identifies one or more patch sites conceptually as:

```text
overlay-node-path : property-name : byte-offset
```

At application time, the resolver:

1. finds the symbol in the base/live tree's `__symbols__`
2. obtains or uses the target node's phandle
3. writes that phandle into each recorded property offset

One external label can have multiple fixup sites. Missing a single symbol blocks correct resolution even if every fragment target exists.

Do not hand-edit offsets. Recompile the source.

## `__local_fixups__`: References Inside The Overlay

```dts
&pinctrl {
        module_pins: module-pins {
                ...
        };
};

&spi2 {
        pinctrl-0 = <&module_pins>;
};
```

`module_pins` is defined inside the overlay. Its compiler-assigned phandle can collide with phandles already used by the base. The resolver shifts overlay-local phandles into a non-conflicting range, then uses `__local_fixups__` to find and shift every reference to them by the same delta.

The local-fixup tree mirrors the overlay paths/properties that contain local phandle cells and stores patch offsets. It does not resolve labels against the base.

## External Versus Local Reference Table

| Reference | Definition lives in | Metadata | Resolver action |
|---|---|---|---|
| `&spi2` target | base | `__fixups__` | look up symbol and patch base phandle |
| `&gpio1` GPIO provider | base | `__fixups__` | look up symbol and patch provider phandle |
| `&module_pins` | same overlay | `__local_fixups__` | add relocation delta to local reference |
| `&module_reg` from later overlay | earlier applied overlay | later overlay `__fixups__` | resolve against symbol exported into live tree |

This classification is the fastest way to diagnose missing-symbol versus bad-local-relocation errors.

## Inspect Compiled Artifacts

Useful host-side commands include:

```bash
fdtdump base.dtb
fdtdump module.dtbo
dtc -I dtb -O dts -o module.decoded.dts module.dtbo
fdtget -l module.dtbo /
fdtget -p module.dtbo /__fixups__
```

Tool output format varies, and decompilation is not a byte-for-byte source reconstruction. Use it to verify structural facts:

- expected `__symbols__` exist in the base
- expected external labels appear under `__fixups__`
- local-reference properties have corresponding local fixups
- fragments target the intended nodes
- no stale reference remains from copied source

## Symbol Names Are Case-Sensitive Interfaces

Renaming `spi2` to `spi_2`, moving a label to another node, or building without `-@` breaks independently compiled overlays. The underlying node can have identical runtime properties and still be overlay-incompatible.

Define exported labels deliberately:

- stable label name
- stable hardware meaning
- expected provider `#*-cells`
- supported base versions
- consumers allowed to depend on it

Avoid exporting every incidental internal label as an accidental long-term promise.

## Paths Inside Fixup Metadata

Fixup locations refer to paths inside the overlay blob. Changes to fragment layout or property order can change offsets without changing semantics. Compare decoded meaning, not raw fixup byte offsets, across rebuilds.

The base symbol values are paths. If the base moves a labeled node but preserves and regenerates the label, the new `__symbols__` value lets label-targeted overlays follow it.

## Build Failure Diagnosis

| Symptom | Likely cause |
|---|---|
| unresolved label at overlay compile time | missing `/plugin/`, malformed reference, or flags/toolchain mismatch |
| DTBO lacks expected fixups | reference resolved locally or overlay not built with expected plugin/symbol flow |
| base lacks `__symbols__` | base not compiled with symbol generation |
| local phandle points incorrectly after apply | missing/corrupt local-fixup metadata or unsupported tool combination |
| later overlay cannot reference first overlay | first overlay did not export/apply symbol as expected or order is wrong |

Reproduce with the exact `dtc` version and flags used in production. Overlay metadata behavior has evolved with toolchains.

## Artifact Review Checklist

- Is the base built with the symbol support required by every label target/reference?
- Is the overlay compiled as a plugin with the expected toolchain?
- Does each external source label have a `__fixups__` entry?
- Does each local phandle reference have local-fixup coverage?
- Does the overlay export only intentional symbols?
- Are base and overlay artifacts from the declared source commits?
- Are DTBO hash and compiler version recorded?
- Can the artifact be decoded without structural warnings?

## Authoritative References

- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to [Resolver, Phandle Relocation, And Merge Semantics](resolver-phandle-relocation-and-merge-semantics.md).
