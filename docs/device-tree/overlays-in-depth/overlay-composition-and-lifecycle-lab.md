---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Overlay Composition And Lifecycle Lab

This lab reviews a three-overlay expansion stack from source through phandle resolution, boot-time composition, live-kernel application, and removal. The correct outcome is not “make every overlay apply.” Some combinations must be rejected, and one apparently removable overlay must be classified boot-only.

## Platform Scenario

Acme Falcon exposes an expansion connector with:

- SPI2 chip selects 0 and 1
- GPIO1 lines 12–15
- a 24 MHz clock output
- one shared interrupt input
- pins multiplexed between SPI2 and a display interface

The product supports:

- an optional power module that provides a switched 3.3 V rail
- an optional SPI temperature module powered by that rail
- an optional display module using the same physical pins as SPI2

The bootloader can apply overlays before Linux. A vendor kernel also exposes a product-specific runtime overlay loader built on the in-kernel overlay API. The temperature driver supports unbind, but a monitoring service keeps its hwmon sysfs file open and polls continuously. The power module controls a regulator also consumed by a base-tree EEPROM device.

## Base Source

```dts
/dts-v1/;

/ {
        compatible = "acme,falcon-revb", "acme,falcon";

        expansion_clk: clock-expansion {
                compatible = "fixed-clock";
                #clock-cells = <0>;
                clock-frequency = <24000000>;
        };

        soc {
                expansion_spi: spi@2000000 {
                        compatible = "acme,falcon-spi";
                        reg = <0x2000000 0x1000>;
                        #address-cells = <1>;
                        #size-cells = <0>;
                        status = "disabled";
                };

                expansion_gpio: gpio@2100000 {
                        compatible = "acme,falcon-gpio";
                        reg = <0x2100000 0x1000>;
                        gpio-controller;
                        #gpio-cells = <2>;
                        interrupt-controller;
                        #interrupt-cells = <2>;
                };

                expansion_pinctrl: pinctrl@2200000 {
                        compatible = "acme,falcon-pinctrl";
                        reg = <0x2200000 0x1000>;
                };

                display0: display@2300000 {
                        compatible = "acme,falcon-display";
                        reg = <0x2300000 0x1000>;
                        status = "disabled";
                };
        };
};
```

Assume omitted clock, interrupt, and power relationships are complete in the real base. The base is currently built without `-@`.

## Overlay A: Module Power

```dts
/dts-v1/;
/plugin/;

&{/} {
        module_3v3: regulator-module-3v3 {
                compatible = "regulator-fixed";
                regulator-name = "module-3v3";
                regulator-min-microvolt = <3300000>;
                regulator-max-microvolt = <3300000>;
                gpio = <&expansion_gpio 14 0>;
                enable-active-high;
        };
};
```

Overlay A introduces and exports `module_3v3`. Product intent says the rail is for expansion modules only, but a downstream base patch also references `&module_3v3` from the EEPROM node after A is applied at runtime.

## Overlay B: Temperature Module

```dts
/dts-v1/;
/plugin/;

&expansion_pinctrl {
        temp_spi_pins: temp-spi-pins {
                pins = "gpio12", "gpio13", "gpio15";
                function = "spi2";
        };
};

&expansion_spi {
        pinctrl-names = "default";
        pinctrl-0 = <&temp_spi_pins>;
        status = "okay";

        temperature-sensor@0 {
                compatible = "acme,temp100";
                reg = <0>;
                spi-max-frequency = <1000000>;
                vdd-supply = <&module_3v3>;
                interrupts-extended = <&expansion_gpio 12 1>;
        };
};
```

## Overlay C: Display Module

```dts
/dts-v1/;
/plugin/;

&expansion_pinctrl {
        display_pins: display-pins {
                pins = "gpio12", "gpio13", "gpio15";
                function = "display";
        };
};

&display0 {
        pinctrl-names = "default";
        pinctrl-0 = <&display_pins>;
        clocks = <&expansion_clk>;
        status = "okay";
};
```

## Field Evidence

1. Applying B to the deployed base fails with a missing `expansion_spi` symbol.
2. After rebuilding the base with symbols, B still fails when applied before A because `module_3v3` is unresolved.
3. A then B applies successfully; Linux creates the SPI controller child and the temperature driver probes.
4. Applying C after B also succeeds structurally, but both devices malfunction.
5. Removing A while B is present is rejected.
6. Removing B succeeds once, but later the monitoring process reads stale sysfs state and the kernel reports a use-after-free in a downstream OF notifier.
7. Removing A after B turns off the EEPROM's supply.

## Lab Objectives

Produce:

1. an external/local symbol inventory for each overlay
2. a resolver trace for A then B
3. a dependency, conflict, and ownership graph
4. a canonical apply/removal policy
5. a base/overlay ABI manifest
6. a merged-tree validation plan
7. a Linux runtime teardown audit
8. a product architecture decision
9. a field-incident root-cause report

## Task 1: Inventory Symbols And Fixups

For each overlay, classify every label reference:

| Overlay | Reference | External or local? | Expected metadata/action |
|---|---|---|---|
| A | `expansion_gpio` |  |  |
| A | `module_3v3` definition |  |  |
| B | `expansion_pinctrl` |  |  |
| B | `temp_spi_pins` |  |  |
| B | `expansion_spi` |  |  |
| B | `module_3v3` |  |  |
| B | `expansion_gpio` |  |  |
| C | `display_pins` |  |  |
| C | `display0` |  |  |
| C | `expansion_clk` |  |  |

State which labels must appear in the base `__symbols__` and which can appear only after a prior overlay applies.

## Task 2: Explain The First Two Failures

For each failure, name:

- stage: compilation, local relocation, external resolution, fragment application, schema validation, probe, or lifecycle
- artifact to inspect
- missing contract
- safe recovery action

Explain why recompiling only B with `-@` cannot create a missing base symbol, and why the second failure is an ordering/dependency error rather than bad local fixups.

## Task 3: Trace A Then B

Assume:

```text
maximum base phandle before A = 80
A local module_3v3 phandle = 1
maximum live-tree phandle after A = 81
B local temp_spi_pins phandle = 1
base expansion_gpio phandle = 22
base expansion_spi phandle = 31
```

Trace:

- A's local phandle relocation
- A's external GPIO provider fixup
- symbol exported for `module_3v3`
- B's local pinctrl phandle relocation
- B's `pinctrl-0` local reference
- B's external target/provider/supply references

Use conceptual values and state why production code must not hard-code them.

## Task 4: Build The Graph

Represent:

```text
requires(B, A)
conflicts(B, C)
owns(...)
before(...)
```

Include:

- pins 12, 13, and 15
- GPIO line 12 interrupt use
- GPIO line 14 regulator enable
- SPI2 CS0
- display0 enablement
- module rail consumers
- symbols exported by A

Determine whether A is truly an independently removable provider once the base EEPROM consumes its regulator.

## Task 5: Derive Apply And Removal Policy

For these selections, specify apply order or rejection:

| Selected set | Decision |
|---|---|
| A |  |
| A + B |  |
| C |  |
| A + C |  |
| B without A |  |
| A + B + C |  |

Then define legal removal transitions from A+B. Account for the monitoring service, the temperature driver, the notifier pointer bug, and the base EEPROM consumer.

## Task 6: Define The Base ABI

Create a manifest for base ABI `falcon-expansion-v2` containing:

- root compatible range
- exported target/provider symbols
- provider cell contracts
- allowed GPIO and chip-select resources
- pin-sharing conflict group
- supported overlays and versions
- required order and lifecycle class
- bootloader/kernel version constraints

Explain why preserving label spelling while changing `#gpio-cells` would still break the ABI.

## Task 7: Validate Final Trees

Write a host-side workflow to:

1. verify base and DTBO hashes
2. inspect symbols and fixups
3. merge A+B from a pristine base
4. merge C from a pristine base
5. reject A+B+C before merge
6. decode and semantically diff results
7. run binding/schema checks
8. run product-specific conflict checks
9. compare with the bootloader's final DTB

Add negative tests for wrong base, wrong order, missing symbol, duplicate overlay, and altered provider cells.

## Task 8: Audit Runtime Removal

For B, list all teardown owners and evidence required for:

- new user requests
- hwmon userspace handles
- SPI transfers
- IRQ handlers
- queued work/timers
- supplier link to A's regulator
- sysfs and subsystem registrations
- cached OF node/property pointers
- dependent overlays

Decide whether B can be advertised as runtime-removable today.

For A, include every consumer—not only devices created by overlays.

## Task 9: Choose Product Architecture

Choose among:

- bootloader-only overlays
- runtime apply-only overlays requiring reboot to remove
- fully runtime-removable overlays
- separate complete DTBs for SPI and display products
- a hybrid

Justify the choice using physical hotplug, testable combinations, security, rollback, and lifecycle support.

## Reference Analysis

### Symbol Inventory

| Overlay | Reference | Classification | Resolution |
|---|---|---|---|
| A | `expansion_gpio` | external base provider | `__fixups__` patches regulator GPIO phandle |
| A | `module_3v3` | local definition/export | local phandle relocated; `__symbols__` can publish it after apply |
| B | `expansion_pinctrl` | external base target | `__fixups__` patches fragment target |
| B | `temp_spi_pins` | local definition/reference | phandle shifted; `__local_fixups__` patches `pinctrl-0` |
| B | `expansion_spi` | external base target | resolved through base symbols |
| B | `module_3v3` | external relative to B; supplied by A | resolves only after A applies |
| B | `expansion_gpio` | external base provider | patches interrupt provider cell |
| C | `display_pins` | local definition/reference | local relocation and local fixup |
| C | `display0` | external base target | base symbol resolution |
| C | `expansion_clk` | external base provider | base symbol resolution |

The base must export `expansion_gpio`, `expansion_pinctrl`, `expansion_spi`, `display0`, and `expansion_clk` for these label-based artifacts. A must apply and export `module_3v3` before B resolves.

### Failure Classification

Failure 1 is external resolution: the deployed base lacks `__symbols__` because it was built without `-@`. Inspect `fdtdump base.dtb` and B's `__fixups__`. Recompiling B cannot add metadata to the base; rebuild/deploy a compatible symbol-bearing base or use a deliberately managed path target.

Failure 2 is also external resolution, caused by a missing prerequisite/order. `module_3v3` is external from B's perspective and does not exist until A applies. Reject B-without-A before mutation, then apply A followed by B from a pristine working tree.

Neither failure is local relocation: `temp_spi_pins` is defined inside B and its reference should be covered by `__local_fixups__`.

### Conceptual Relocation Trace

For A:

```text
base maximum = 80
module_3v3 local phandle 1 -> 81
regulator gpio provider placeholder -> base expansion_gpio phandle 22
exported module_3v3 symbol -> path of applied regulator node with phandle 81
```

For B after A:

```text
live maximum = 81
temp_spi_pins local phandle 1 -> 82
pinctrl-0 local reference 1 -> 82
expansion_pinctrl target -> base target phandle
expansion_spi target -> base phandle 31
vdd-supply -> A's module_3v3 phandle 81
interrupt provider -> base expansion_gpio phandle 22
```

Actual relocation uses the live maximum and tool implementation. Hard-coded phandles would collide as bases and overlay order change.

### Dependency And Conflict Graph

```text
requires(B, A)                 B consumes module_3v3
before(A, B)                   A must export symbol before B resolves
conflicts(B, C)                shared pins 12, 13, 15; mutually exclusive mux
owns(A, gpio1-line14)          regulator enable
owns(A, module-3v3-rail)       but ownership is violated by base EEPROM dependency
owns(B, spi2-cs0)
owns(B, gpio1-line12-irq)
owns(B, pins12/13/15-spi)
owns(C, pins12/13/15-display)
owns(C, display0-status)
```

The resolver cannot see the pin conflict. Both B and C can apply and each target a different consumer while requesting incompatible muxes. Product selection must reject the pair before application.

The downstream base EEPROM reference makes A part of base power topology, not a removable optional provider. Either move the regulator into the appropriate base DTB and redefine A, or declare A boot-only/permanent for that configuration. A cannot be removed merely because no later overlay cookie remains.

### Apply And Removal Policy

| Selected set | Policy |
|---|---|
| A | allow only when a supported module/base profile needs the rail; boot-only under current EEPROM dependency |
| A + B | apply A then B; supported boot composition |
| C | apply C to pristine base if display's other resources validate |
| A + C | allow only if A has a legitimate independent consumer and no electrical conflict; otherwise omit unused A |
| B without A | reject missing prerequisite |
| A + B + C | reject pin/function conflict |

From A+B, logical dependency order is remove B before A. Current runtime behavior is still unsafe:

1. stop the monitoring service and prevent new opens
2. unbind/quiesce the temperature device, drain SPI/IRQ/work, unregister hwmon
3. fix and verify the OF notifier so it retains no overlay pointer beyond post-remove
4. remove B
5. keep A because the base EEPROM consumes it, or migrate the EEPROM supply to a base-owned provider before considering A removal

Until those conditions are implemented and stress-tested, classify B as boot-only or apply-only with reboot-required removal. A is non-removable in this base composition.

### Base ABI Manifest

```text
base_abi: falcon-expansion-v2
root_compatible:
  - acme,falcon-revb
exported_symbols:
  expansion_spi: SPI controller; CS0 and CS1 policy-managed
  expansion_gpio: GPIO/interrupt provider; #gpio-cells contract fixed
  expansion_pinctrl: pin controller; approved groups only
  expansion_clk: fixed 24 MHz provider; #clock-cells = 0
  display0: display controller target
resource_groups:
  connector-pins-12-13-15: [spi-function, display-function] exactly one
supported:
  module-power-v1 -> boot-only
  temp-module-v2 -> requires module-power-v1; boot-only
  display-module-v3 -> conflicts temp-module-v2
application:
  authenticated manifest; canonical order; pristine working base
versions:
  exact supported U-Boot/libfdt and kernel release ranges
```

If `#gpio-cells` changes, the same numeric tail after the phandle is decoded differently. Keeping `expansion_gpio` spelled the same would turn a linkage success into a semantic ABI break.

### Validation Workflow

```bash
sha256sum base.dtb module-power.dtbo temp-module.dtbo display-module.dtbo

fdtdump base.dtb
fdtdump module-power.dtbo
fdtdump temp-module.dtbo
fdtdump display-module.dtbo

fdtoverlay -i base.dtb -o merged-power-temp.dtb \
  module-power.dtbo temp-module.dtbo

fdtoverlay -i base.dtb -o merged-display.dtb display-module.dtbo

dtc -I dtb -O dts -o merged-power-temp.dts merged-power-temp.dtb
dtc -I dtb -O dts -o merged-display.dts merged-display.dtb
```

Do not invoke `fdtoverlay` for A+B+C after the manifest detects the conflict. A test may still demonstrate that structural application succeeds, proving why semantic conflict checks are necessary, but production must reject before mutation.

Validate merged DTBs with the project's current `dt-schema` workflow, then run custom ownership checks for pins, GPIOs, CS values, regulators, aliases, and graph topology. Capture the bootloader's post-overlay FDT and compare normalized semantic content with host results.

Negative fixtures should independently alter base symbols, order B before A, change provider cells, duplicate B, choose the wrong root compatible, and corrupt a DTBO hash. Each must fail at its declared gate.

### Incident Root Causes

1. Missing symbol: base build/deployment contract omitted `-@` and exported symbols.
2. B before A: manifest failed to encode a hard symbol/supply prerequisite.
3. B+C malfunction: product conflict policy omitted shared pin ownership; structural success was mistaken for hardware validity.
4. A removal rejection: expected stack protection because B depends on A.
5. B removal use-after-free: downstream notifier violated overlay pointer lifetime; userspace/driver teardown was not coordinated.
6. EEPROM power loss: an allegedly optional provider became a base-device supplier, invalidating A's removal classification and ownership model.

### Product Architecture Decision

A defensible near-term design is a hybrid:

- use separate canonical product profiles or complete base DTBs for mutually exclusive SPI and display assemblies
- if binary overlays remain necessary, apply authenticated A+B or C only in the bootloader
- make A base-owned whenever the EEPROM consumes its rail
- expose no runtime removal for A, B, or C
- permit future runtime removal only after physical hotplug rules, userspace coordination, drivers, notifiers, supplier links, and stress tests satisfy the teardown contract

This keeps optional module composition while preventing a partially supported runtime path from being advertised as safe.

## Completion Criteria

You have completed the lab when you can:

- identify every local and external fixup dependency
- trace phandle relocation without relying on fixed values
- reject B without A and B with C before application
- compute dependency-safe removal order while recognizing that order alone is insufficient
- define the exported base ABI beyond label names
- validate the complete merged trees and negative combinations
- explain every field failure at its correct layer
- justify a boot-only or separate-DTB architecture from lifecycle evidence

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [Linux Devicetree Changesets](https://docs.kernel.org/devicetree/changesets.html)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)

## Next Step

Continue with [Build And Diagnostic Tools](../build-and-diagnostic-tools.md), where the compiler, dump, query, mutation, overlay, and libfdt tools are treated as a unified engineering workflow.
