---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# U-Boot DT Sources, Upstream Sync, And Build Artifacts

Modern U-Boot can consume upstream Devicetree sources while adding bootloader-only fragments. A maintainable port keeps hardware description shared, isolates phase or packaging metadata, and can reproduce the exact source closure for every generated DTB.

## Shared Hardware, Different Consumers

SoC registers, interrupts, clocks, resets, bus topology, and board wiring should normally agree across firmware and Linux. Divergent copies drift: one receives a binding migration or corrected polarity while the other keeps an obsolete contract.

U-Boot may still need distinct information:

- `bootph-*` phase availability
- verified-boot key nodes
- binman packaging nodes
- bootloader-only configuration
- deliberate `status` differences for boot policy
- properties for U-Boot drivers not yet aligned with the shared binding

Keep those deltas explicit and small. A `*-u-boot.dtsi` should explain bootloader policy, not become an unreviewed fork of the board DTS.

## U-Boot's Devicetree Source Flow

The U-Boot documentation describes supported source locations and automatic inclusion of a U-Boot-specific `.dtsi` chosen from board, SoC, CPU, vendor, or generic names. Exact precedence evolves; inspect the documentation and build output for the release in use.

Conceptually:

```text
selected base DTS
  + ordinary includes
  + one automatically selected *-u-boot.dtsi
  + optional external includes
  -> preprocessed DTS
  -> dtc
  -> u-boot.dtb
  -> phase filtering
  -> spl/u-boot-spl.dtb or tpl/u-boot-tpl.dtb
```

External includes can carry product keys or deployment-specific data that must not be committed upstream. Their presence makes the build environment part of DT provenance; record and authenticate them.

## Upstream Devicetree Synchronization

U-Boot maintains tooling and policy for syncing its Devicetree source subtree with the upstream Devicetree repository. Prefer the documented sync process over copying individual Linux files ad hoc.

When updating:

1. identify the upstream commit or tag
2. import the coherent source/binding set
3. rebase U-Boot-specific deltas
4. build every affected board and phase DTB
5. run schema and U-Boot tests
6. compare functional nodes and boot-critical properties
7. test cold boot and recovery media

A clean textual merge does not prove driver compatibility. U-Boot and Linux can support different subsets of a binding during a transition.

## Configuration Selects The Default Tree

Board configuration commonly selects a default Devicetree name. Multi-platform builds can compile several DTBs and choose later. Distinguish:

- build-time default
- list of DTBs built or packaged
- control-DTB selection by SPL/board code
- OS-DTB selection by FIT or boot flow

These may use similar names but answer different questions.

Do not encode field-upgradable product identity solely in a build default. A universal image needs a trusted runtime identifier and deterministic fallback.

## Inspect The Actual Build

Use an out-of-tree build directory and retain:

```sh
make O=build board_defconfig
make O=build V=1
fdtdump build/u-boot.dtb
fdtdump build/spl/u-boot-spl.dtb
dtc -I dtb -O dts -o - build/u-boot.dtb
```

Artifact names depend on the configuration. Inspect the verbose compiler and `dtc` commands to discover preprocessing flags, include paths, generated inputs, and phase filtering.

For a reproducible release, record:

- U-Boot commit and dirty state
- defconfig plus final `.config`
- toolchain and host-tool versions
- upstream DT synchronization point
- external include content hashes
- generated DTB hashes
- binman/FIT inputs and outputs

The defconfig alone is not the complete build input.

## U-Boot-Specific Properties

Before adding a property, ask:

1. Is it a stable hardware fact that belongs in the shared binding?
2. Is it phase availability represented by a standard `bootph-*` property?
3. Is it build packaging that belongs under binman metadata?
4. Is it a boot policy better expressed by a FIT configuration, boot flow, or environment?
5. Is it a workaround for a driver missing a binding feature?

Document temporary deviations and remove them when the shared binding and driver converge. Avoid properties that directly name a C driver or instantiate software-only pseudo-devices.

## Source Review With Semantic Diffs

Textual diffs of a `.dts` file miss inherited changes. Compare compiled trees:

```sh
dtc -I dtb -O dts old/u-boot.dtb > old.dts
dtc -I dtb -O dts new/u-boot.dtb > new.dts
diff -u old.dts new.dts
```

For production automation, normalize labels and generated noise, then gate semantic changes to boot-critical paths:

- console and timer
- storage and boot media
- clocks, resets, regulators, pinctrl
- DRAM and reserved memory
- verification keys and required policy
- phase tags
- binman layout

Review the SPL/TPL outputs independently; a node present in `u-boot.dtb` can disappear during filtering.

## Common Failure Patterns

### U-Boot Builds But Loses Boot Storage

The shared DTS update changed a compatible, clock, pin state, or `status`, and the older U-Boot driver no longer binds. Compare the compiled control tree with the driver's match table and binding.

### SPL Loses A Supplier

The consumer was tagged for pre-RAM use but its clock, reset, pinctrl, regulator, or bus parent was not. The full control tree works; the filtered tree is incomplete.

### Local Fix Returns After Every Sync

The change was never separated into an appropriate U-Boot-specific fragment or sent to the authoritative upstream source. Establish ownership instead of repeatedly copying it.

### Release Build Uses Another Key

An external include or signing step was not part of recorded provenance. Treat key injection and the resulting control DTB as security-sensitive build outputs.

## Authoritative References

- [U-Boot Devicetree documentation](https://docs.u-boot.org/en/latest/develop/devicetree/index.html)
- [U-Boot Devicetree Control and source inclusion](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot Devicetree synchronization and source control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html#resyncing-with-devicetree-rebasing)
- [U-Boot build documentation](https://docs.u-boot.org/en/latest/build/index.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to [Driver Model, Boot Phases, And Pre-Relocation Properties](driver-model-boot-phases-and-pre-relocation-properties.md).
