---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Driver Model, Boot Phases, And Pre-Relocation Properties

Early U-Boot phases cannot bind every device from the full tree. Phase properties tell the build and driver-model machinery which nodes must remain available before later initialization. They express dependency timing, not importance.

## The Boot Phases

A platform can have:

- **TPL**: an extremely small tertiary program loader, often before SRAM or DRAM is fully usable
- **VPL**: a verification-oriented phase on configurations that use it
- **SPL**: a secondary program loader that usually initializes DRAM and loads U-Boot proper or another payload
- **U-Boot proper before relocation**
- **U-Boot proper after relocation**

Not every platform uses every phase. The meaning of “pre-relocation” depends on which binary is running and which memory transition it must survive.

## Current `bootph-*` Properties

Current U-Boot uses standardized phase properties such as:

| Property | Intent |
|---|---|
| `bootph-all` | node needed in all boot phases |
| `bootph-pre-ram` | node needed in phases before DRAM is available, including SPL use |
| `bootph-pre-sram` | node needed before SRAM is available, commonly TPL |
| `bootph-verify` | node needed in the verification phase |
| `bootph-some-ram` | node needed after some RAM is available but before full U-Boot context |

Support and precise filtering rules depend on the U-Boot version. Older trees used `u-boot,dm-*` properties; modern work should follow the current boot-phase binding and migration guidance.

Do not spray `bootph-all` across a failing path. It expands every phase tree and can hide an incorrect dependency model.

## Mark The Complete Dependency Closure

If SPL reads eMMC, the required closure can include:

```text
SoC/bus parent
  -> clock and reset controllers
  -> power domain or regulator
  -> pinctrl controller and selected state
  -> MMC controller
  -> PHY
  -> GPIO used for reset/card detect
  -> timer/delay source
```

The exact set depends on driver implementation. A provider missing from the filtered DTB can produce `-ENODEV`, deferred probe, a silent default clock, or a hang before console output.

For each early consumer:

1. enumerate all phandles and ancestors
2. identify implicit driver dependencies
3. confirm their drivers are compiled for that phase
4. mark only nodes required at that phase
5. inspect the filtered DTB
6. test with debug output and size accounting

A phase tag preserves a node; Kconfig includes code. Both are necessary.

## Driver Binding And Probe

U-Boot driver model separates binding from probing. A node present in the control tree can create a device object without initializing hardware immediately. Pre-relocation eligibility of a driver and node affects when binding/probe can occur.

Do not solve ordering with arbitrary source-file order. Model suppliers, use driver-model APIs, and understand which initialization still happens in board code. Legacy non-DM initialization mixed with DT-based drivers often creates hidden sequencing.

Inspect:

```text
=> dm tree
=> dm uclass
=> dm drivers
```

Command availability depends on configuration and phase. In SPL, use targeted debug logs, map files, and sandbox/unit tests where possible.

## Automatic Filtering

U-Boot's SPL framework uses build tooling to reduce the control tree. The result retains required nodes and removes phase markers or configured properties from the final small blob. Therefore:

- source presence does not guarantee output presence
- full `u-boot.dtb` behavior does not prove SPL behavior
- a source label cannot be inspected at runtime
- filtering can remove a property even when its node remains

Always decompile `spl/u-boot-spl.dtb` or inspect it with `fdtget`. Compare it with the phase's compiled drivers.

## Phase Properties Belong Where The Requirement Lives

Put shared phase requirements in the narrowest reusable U-Boot-specific SoC or board fragment. A controller required in SPL on every SoC board may be tagged at the SoC U-Boot layer; a board-only boot regulator belongs in the board layer.

Avoid adding boot-phase properties to upstream OS hardware sources unless the upstream Devicetree project accepts and owns those properties in that context. U-Boot's documented source-composition flow exists to keep bootloader metadata manageable.

## Size And Security Tradeoffs

Every retained node and compiled driver consumes storage or SRAM. More early attack surface also exists before later protections and update policy are active.

Review:

- whether debug UART is needed in production SPL
- whether network/USB recovery is authenticated
- whether unused aliases or strings can be filtered
- whether verification algorithms and key data fit
- whether early drivers parse untrusted media before authentication

Removing a node for size without removing its code—or the reverse—may not deliver the expected savings.

## Diagnosis Matrix

| Symptom | Evidence to gather |
|---|---|
| device works in U-Boot, not SPL | filtered DTB, SPL Kconfig, phase driver flags |
| consumer exists, provider absent | phandle closure and provider phase tag |
| provider exists, no driver | SPL/TPL config and link map |
| SPL size suddenly grows | DTB sizes, map diff, newly included drivers/data |
| old property no longer works | current `bootph-*` binding and migration |
| boot source changes with logging | ordering, uninitialized state, timing, stack/heap pressure |

## Authoritative References

- [U-Boot generic SPL framework](https://docs.u-boot.org/en/latest/develop/spl.html)
- [U-Boot boot-phase binding schema](https://github.com/u-boot/u-boot/blob/master/doc/device-tree-bindings/bootph.yaml)
- [U-Boot driver-model design](https://docs.u-boot.org/en/latest/develop/driver-model/design.html)
- [U-Boot Devicetree Control](https://docs.u-boot.org/en/latest/develop/devicetree/control.html)
- [U-Boot driver-model command](https://docs.u-boot.org/en/latest/usage/cmd/dm.html)

## Continue

Proceed to [TPL, SPL, SRAM Budgets, And Multi-DTB Selection](tpl-spl-sram-budgets-and-multi-dtb-selection.md).
