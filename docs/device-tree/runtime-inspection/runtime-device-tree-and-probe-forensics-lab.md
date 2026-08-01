---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Runtime Device Tree And Probe Forensics Lab

This lab diagnoses a capture engine whose node exists, module is loaded, and device is registered, but no driver is bound. The visible consumer failure is two supplier levels away and was introduced by a bootloader overlay. The correct fix is not a manual bind or a live binary edit.

## Incident

Acme Falcon revision B contains an AXC200 capture engine powered by a fixed regulator. The regulator enable is connected to GPIO line 3 of an I2C expander.

After a product-profile update:

- `/sys/firmware/devicetree/base/soc/capture@48000000` exists
- `ax_capture` appears in `lsmod`
- no capture interface appears in `/sys/class/acme-capture`
- the platform device exists but has no `driver` symlink
- an engineer proposes forcing a bind and then editing the live tree

## Intended Source

```dts
/ {
        model = "Acme Falcon revision B";
        compatible = "acme,falcon-revb", "acme,falcon";

        capture_3v3: regulator-capture-3v3 {
                compatible = "regulator-fixed";
                regulator-name = "capture-3v3";
                regulator-min-microvolt = <3300000>;
                regulator-max-microvolt = <3300000>;
                gpio = <&ioexp 3 0>;
                enable-active-high;
        };
};

&i2c2 {
        status = "okay";

        ioexp: gpio@20 {
                compatible = "acme,gpio16";
                reg = <0x20>;
                gpio-controller;
                #gpio-cells = <2>;
        };
};

&capture0 {
        compatible = "acme,axc200";
        interrupts = <80>, <81>;
        interrupt-names = "completion", "error";
        clocks = <&ccu 12>, <&capture_clk>;
        clock-names = "bus", "sample";
        resets = <&resetc 7>;
        reset-names = "core";
        vdd-supply = <&capture_3v3>;
        status = "okay";
};
```

Assume provider cell formats and omitted controller resources are valid in the real binding.

## Boot Composition

The product applies:

```text
falcon-revb.dtb
  -> factory-camera.dtbo
  -> low-power-profile.dtbo
  -> board identity and /chosen fixups
```

The low-power profile was intended for a product without the expander. It contains:

```dts
/dts-v1/;
/plugin/;

&i2c2 {
        status = "disabled";
};
```

The manifest incorrectly permits it with `factory-camera.dtbo`.

## Collected Runtime Observations

Root:

```text
/model: Acme Falcon revision B
/compatible: acme,falcon-revb, acme,falcon
```

Live nodes:

```text
/soc/capture@48000000/status = okay
/soc/i2c@30000000/status = disabled
/soc/i2c@30000000/gpio@20/status = absent
/regulator-capture-3v3/status = absent
```

Device-model observations:

```text
platform device 48000000.capture exists
48000000.capture/of_node resolves to /soc/capture@48000000
48000000.capture/modalias is an OF alias for acme,axc200
48000000.capture/driver is absent

platform device regulator-capture-3v3 exists
regulator-capture-3v3/driver is absent

no I2C client device exists for address 0x20 on the expected bus
the I2C2 controller platform device has no bound driver because the node is unavailable
```

Module observations:

```text
modprobe --resolve-alias <capture modalias> -> ax_capture
ax_capture is loaded
the AXC200 compatible appears in modinfo -F alias ax_capture
```

Deferred diagnostics, with wording simplified:

```text
regulator-capture-3v3  GPIO provider not ready
48000000.capture       supplier regulator not ready
```

Relevant log sequence:

```text
reg-fixed-voltage regulator-capture-3v3: error -EPROBE_DEFER: failed to get enable GPIO
ax-capture 48000000.capture: error -EPROBE_DEFER: failed to get vdd supply
```

Hashes:

```text
built revision-B DTB                   H_BUILD
host merge: base + factory-camera      H_CAMERA
host merge: base + camera + low-power  H_LOWPOWER
U-Boot pre-handoff FDT                 H_HANDOFF
/sys/firmware/fdt                      H_HANDOFF
reconstructed live-tree.dtb            H_REBUILT_LIVE
```

`H_HANDOFF` equals `H_LOWPOWER` except for documented identity and `/chosen` fixups. `H_REBUILT_LIVE` is different at byte level.

## Lab Objectives

Produce:

1. a pre-mutation runtime evidence bundle
2. binary-safe property decodes for the consumer and suppliers
3. a boot-FDT versus live-tree comparison
4. a node-to-device-to-driver state table
5. the recursive deferred-supplier trace
6. a decision on manual bind/unbind
7. the first divergent checkpoint and root cause
8. a corrected manifest and regression matrix

## Task 1: Preserve Runtime Evidence

List exact commands/artifacts to capture before reboot, module reload, bind attempt, or overlay change:

- kernel and boot identity
- raw boot FDT when available
- reconstructed live tree
- target raw property files
- device paths and resolved symlinks
- modalias/module metadata
- deferred list and device links
- complete kernel log
- subsystem/debugfs evidence
- U-Boot/pre-handoff and build manifest data

State which values require privacy/security redaction.

## Task 2: Decode The Consumer

Decode from the live tree:

- ordered compatible strings
- status and its absence semantics
- two interrupts and their names
- two clocks and their names
- reset and reset name
- `vdd-supply`

For each phandle array, name the provider cell count/binding needed. Do not equate DT interrupt cells with Linux IRQ numbers.

## Task 3: Resolve The Supply Chain

Starting from `capture@48000000/vdd-supply`:

1. resolve the regulator node
2. find its Linux platform device
3. inspect its driver/deferred state
4. decode its enable GPIO
5. resolve the GPIO expander live node
6. inspect the expander's parent I2C node
7. determine why no I2C client exists

Build a table of every transition and the evidence proving it.

## Task 4: Compare Boot And Live Trees

Use `/sys/firmware/fdt` and `dtc -I fs` to create separate captures. Explain:

- why `H_HANDOFF` can equal the raw boot-FDT hash
- why `H_REBUILT_LIVE` need not equal either
- whether the low-power change happened before or after Linux boot
- how a runtime overlay would change the expected comparison
- why the FDT reservation map cannot be reconstructed from the live filesystem alone

## Task 5: Classify The Consumer State

Choose the exact reached state:

```text
node absent
node unavailable
device not populated
device created, no match
module not available
probe deferred
probe failed terminally
bound
functional
```

Then classify the regulator, GPIO expander, and I2C controller separately. One label cannot describe the whole chain.

## Task 6: Evaluate Manual Binding

Should the engineer write `48000000.capture` to the AX driver `bind` file? Should they set `driver_override`? Should they unbind/rebind the regulator?

For each action, state:

- what hypothesis it would test
- whether prerequisites are satisfied
- likely result
- risks
- safer read-only alternative

## Task 7: Locate First Divergence

Compare:

```text
built base
host camera merge
host camera + low-power merge
U-Boot pre-handoff
raw boot FDT
live tree
```

Name the first checkpoint at which the intended camera product becomes invalid. Distinguish the immediate consumer symptom from the product-policy root cause.

## Task 8: Define The Fix

Design:

- manifest dependency/conflict rules
- safe boot behavior for invalid profile combinations
- final-tree assertions
- runtime deferred-probe checks
- hardware functional tests
- rollback behavior

Do not “fix” the incident by making the capture driver ignore its supply.

## Reference Analysis

### Evidence Bundle

```bash
uname -a
cat /proc/version
cat /proc/cmdline
cat /proc/sys/kernel/random/boot_id

sudo cp -- /sys/firmware/fdt boot-fdt.dtb
sudo dtc -I fs -O dtb -o live-tree.dtb /sys/firmware/devicetree/base
sudo dtc -I fs -O dts -o live-tree.dts /sys/firmware/devicetree/base
sha256sum boot-fdt.dtb live-tree.dtb

journalctl -k -b 0 -o short-monotonic > kernel-boot.log
sudo cat /sys/kernel/debug/devices_deferred > devices-deferred.txt
```

Also copy/hash raw files for consumer `compatible`, `status`, `interrupts`, `interrupt-names`, `clocks`, `clock-names`, `resets`, `reset-names`, and `vdd-supply`; supplier GPIO and compatible properties; root identity; and non-sensitive `/chosen` fields.

Capture:

```bash
readlink -f /sys/bus/platform/devices/48000000.capture/of_node
readlink -f /sys/bus/platform/devices/48000000.capture/subsystem
readlink -f /sys/bus/platform/devices/48000000.capture/driver 2>/dev/null || true
cat /sys/bus/platform/devices/48000000.capture/modalias
cat /sys/bus/platform/devices/48000000.capture/uevent
```

Repeat for regulator and controller devices using discovered paths. Save module `modinfo`, alias resolution, debugfs regulator/GPIO state, `/proc/interrupts`, and supplier/consumer links. Redact seeds, secrets, and product identifiers according to policy.

### Binary-Safe Decode

Use NUL-aware output for names:

```bash
tr '\0' '\n' <"$node/compatible"
tr '\0' '\n' <"$node/interrupt-names"
tr '\0' '\n' <"$node/clock-names"
tr '\0' '\n' <"$node/reset-names"
```

Use byte grouping or the reconstructed DTB for cells:

```bash
xxd -p -c 4 "$node/interrupts"
xxd -p -c 4 "$node/clocks"
xxd -p -c 4 "$node/resets"
xxd -p -c 4 "$node/vdd-supply"

fdtget -tx live-tree.dtb /soc/capture@48000000 clocks
fdtget -tx live-tree.dtb /soc/capture@48000000 vdd-supply
```

Decode `clocks` using each referenced provider's `#clock-cells`, `resets` using `#reset-cells`, `interrupts` through effective interrupt parent and `#interrupt-cells`, and the supply as a regulator phandle. Pair entries with binding-defined names.

### Recursive Supplier Trace

| Stage | Live node | Linux device | Driver state | Evidence/conclusion |
|---|---|---|---|---|
| capture consumer | present, okay | platform device present | deferred/no link | waits on `vdd-supply` |
| fixed regulator | present, available | platform device present | deferred/no link | waits on enable GPIO provider |
| GPIO expander | present, available locally | no I2C client | none | parent bus unavailable, so child not enumerated |
| I2C2 controller | present, disabled | no bound controller | unavailable | low-power overlay changed `status` |

The expander's missing `status` normally makes it available relative to its own node, but its disabled parent prevents the bus/controller path required to create the I2C client.

The first missing runtime transition is I2C2 availability/population. Everything above it defers correctly.

### Boot Versus Live Interpretation

`H_HANDOFF == hash(/sys/firmware/fdt)` strongly supports byte-identical exported boot input. `H_REBUILT_LIVE` differs because `dtc -I fs` serializes a new blob without original header layout, padding, or FDT reservation map and may order content differently.

Because raw boot FDT already contains `i2c2/status = "disabled"`, the low-power overlay applied before Linux. With a later runtime overlay, live semantics could diverge while `/sys/firmware/fdt` remains the boot snapshot.

Compare both captures semantically with the same `dtc` version and focused property queries.

### State Classification

- capture node: present and available
- capture device: created on platform bus
- capture match/module: alias resolves and module/driver are available
- capture probe: deferred on regulator; not terminally failed and not bound
- regulator: device created, probe deferred on GPIO provider
- GPIO expander: node present/locally available, but no I2C device populated
- I2C2: node unavailable due final `status = "disabled"`; controller path not bound/populated
- subsystem: no functional capture interface, as expected downstream of deferral

### Manual Bind Decision

Do not force-bind capture. Normal matching already succeeds far enough to run probe, which deliberately defers. A manual bind retries the same missing supply and adds noise.

Do not set `driver_override`; match metadata is proven correct, and bypassing it cannot create the supplier.

Do not unbind/rebind the regulator; it is not bound. Its deferral is valid. Forcing or bypassing the enable GPIO would violate the hardware power contract.

The safe read-only test is to trace and decode the supplier chain, compare final-tree checkpoints, and reproduce the overlay composition on the host. Correct the manifest and reboot with a valid immutable final tree.

### First Divergence And Root Cause

The base plus camera overlay is valid. The first invalid checkpoint is host-equivalent composition after `low-power-profile.dtbo`, where I2C2 becomes disabled despite the camera stack requiring its GPIO expander.

Immediate symptom:

```text
capture probe defers because vdd regulator is unavailable
```

Dependency cause:

```text
regulator defers because its GPIO provider device does not exist
```

Root cause:

```text
product manifest permits conflicting camera and low-power overlays;
boot selection applies both without checking the camera -> regulator -> GPIO
expander -> I2C2 dependency
```

### Fix And Regression Gates

Manifest rules:

```text
factory-camera requires i2c2-expander and capture-3v3
low-power-no-expander conflicts factory-camera
camera composition must leave i2c2 status okay
```

Boot behavior:

- reject conflicting selection before applying any overlay
- compose from pristine authenticated base
- on mandatory overlay/assertion failure, discard working FDT
- select a defined safe recovery profile or stop normal boot

Final-tree CI assertions:

- I2C2 available
- expander child present at address `0x20`
- regulator supply phandle resolves to correct node
- regulator GPIO resolves to expander line 3 with correct flags
- capture resources/names match AXC200 schema
- every supported merged tree passes schema and product dependency checks
- rejected camera+low-power combination fails deterministically

Runtime gates:

- no target devices remain in `devices_deferred` after expected module/coldplug completion
- capture, regulator, expander, and I2C controller have expected driver links
- capture class/interface appears
- regulator enable, capture transfer, IRQ/error recovery, suspend/resume, and reboot are tested

Rollback must choose a mutually compatible base, overlay set, kernel, modules, and firmware; it must not reintroduce the invalid profile pairing.

## Completion Criteria

You have completed the lab when you can prove the consumer's DT and match data are correct, recursively locate the disabled I2C parent as the first missing runtime transition, and attribute that state to the pre-boot overlay manifest rather than forcing the capture driver.

## Authoritative References

- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Linux driver infrastructure and deferred probe](https://docs.kernel.org/driver-api/infrastructure.html)
- [Linux sysfs firmware OF ABI](https://docs.kernel.org/admin-guide/abi-testing-files.html)
- [Linux Devicetree overlay notes](https://docs.kernel.org/devicetree/overlay-notes.html)

## Next Step

Continue with [Security And Production Lifecycle](../security-and-production-lifecycle.md), treating DTB identity, overlay selection, runtime access, update compatibility, and forensic evidence as security-controlled product interfaces.
