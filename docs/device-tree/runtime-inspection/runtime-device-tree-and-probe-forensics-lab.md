---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Runtime Device Tree And Probe Forensics Lab

This lab diagnoses an SD card controller on a TI AM642 SK whose node exists, driver is built in, and device is registered, but no driver is bound. The visible consumer failure is two supplier levels away and was introduced by a bootloader overlay. The correct fix is not a manual bind or a live binary edit.

The board, nodes, drivers, and log messages are real (mainline `k3-am642-sk.dts`, `sdhci_am654`, `reg-fixed-voltage`, `pca953x`). The product overlays and manifest are a hypothetical product built on that board.

## Incident

On the AM642 SK, the SD card slot (`sdhci1`, `mmc@fa00000`) is powered by a TPS2051BD load switch modeled as the fixed regulator `vdd_mmc1`. Its enable input is line 3 of the PCA9538 GPIO expander `exp1` at I2C address `0x70` on `main_i2c1`.

The lab unit boots with an NFS root filesystem. After a product-profile update:

- `/sys/firmware/devicetree/base/bus@f4000/mmc@fa00000` exists
- `sdhci_am654` is built into the kernel (`CONFIG_MMC_SDHCI_AM654=y`) and `/sys/bus/platform/drivers/sdhci-am654` exists
- no `mmcblk1` device appears when a card is inserted
- the platform device `fa00000.mmc` exists but has no `driver` symlink
- an engineer proposes forcing a bind and then editing the live tree

## Intended Source

From `arch/arm64/boot/dts/ti/k3-am642-sk.dts` (trimmed):

```dts
/ {
        compatible = "ti,am642-sk", "ti,am642";
        model = "Texas Instruments AM642 SK";

        vdd_mmc1: regulator-2 {
                /* TPS2051BD */
                compatible = "regulator-fixed";
                regulator-name = "vdd_mmc1";
                regulator-min-microvolt = <3300000>;
                regulator-max-microvolt = <3300000>;
                regulator-boot-on;
                enable-active-high;
                vin-supply = <&vcc_3v3_sys>;
                gpio = <&exp1 3 GPIO_ACTIVE_HIGH>;
        };
};

&main_i2c1 {
        status = "okay";
        clock-frequency = <400000>;

        exp1: gpio@70 {
                compatible = "nxp,pca9538";
                reg = <0x70>;
                gpio-controller;
                #gpio-cells = <2>;
                gpio-line-names = "GPIO_CPSW2_RST", "GPIO_CPSW1_RST",
                                  "PRU_DETECT", "MMC1_SD_EN", /* ... */;
        };
};

&sdhci1 {
        status = "okay";
        vmmc-supply = <&vdd_mmc1>;
        pinctrl-names = "default";
        pinctrl-0 = <&main_mmc1_pins_default>;
        disable-wp;
};
```

The SoC node in `k3-am64-main.dtsi` supplies `compatible = "ti,am64-sdhci-4bit"`, two `reg` windows, one interrupt, `clocks = <&k3_clks 58 3>, <&k3_clks 58 4>` with names `clk_ahb` and `clk_xin`, and `power-domains = <&k3_pds 58 TI_SCI_PD_EXCLUSIVE>`.

## Boot Composition

The product applies (hypothetical overlays):

```text
k3-am642-sk.dtb
  -> product-sdcard.dtbo
  -> low-power-profile.dtbo
  -> board identity and /chosen fixups
```

The low-power profile was written for a product variant without the expander. It contains:

```dts
/dts-v1/;
/plugin/;

&main_i2c1 {
        status = "disabled";
};
```

The manifest incorrectly permits it together with `product-sdcard.dtbo`.

## Collected Runtime Observations

Root:

```text
/model: Texas Instruments AM642 SK
/compatible: ti,am642-sk, ti,am642
```

Live nodes:

```text
/bus@f4000/mmc@fa00000/status = okay
/bus@f4000/i2c@20010000/status = disabled
/bus@f4000/i2c@20010000/gpio@70/status = absent
/regulator-2/status = absent
```

Device-model observations:

```text
platform device fa00000.mmc exists
fa00000.mmc/of_node resolves to /bus@f4000/mmc@fa00000
fa00000.mmc/modalias is of:NmmcT(null)Cti,am64-sdhci-4bit
fa00000.mmc/driver is absent

platform device regulator-2 exists
regulator-2/driver is absent

no I2C client 1-0070 exists, and no 1-0048/1-0049 temperature sensors either
platform device 20010000.i2c is not created because its node is disabled
```

Driver observations:

```text
/sys/bus/platform/drivers/sdhci-am654 exists (built in)
/sys/bus/platform/drivers/reg-fixed-voltage exists (built in)
/sys/bus/i2c/drivers/pca953x exists (built in)
```

Deferred diagnostics (`/sys/kernel/debug/devices_deferred`; exact text depends on kernel version and `fw_devlink`):

```text
regulator-2     can't get GPIO
fa00000.mmc     vmmc regulator not available
```

The reasons come from `dev_err_probe()` calls in `drivers/regulator/fixed.c` and `drivers/mmc/core/regulator.c`.

Hashes:

```text
built k3-am642-sk.dtb                   H_BUILD
host merge: base + product-sdcard       H_SDCARD
host merge: base + sdcard + low-power   H_LOWPOWER
U-Boot pre-handoff FDT                  H_HANDOFF
/sys/firmware/fdt                       H_HANDOFF
reconstructed live-tree.dtb             H_REBUILT_LIVE
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
- the interrupt
- two clocks and their names
- the power domain
- `vmmc-supply`

For each phandle array, name the provider cell count/binding needed. Do not equate DT interrupt cells with Linux IRQ numbers.

## Task 3: Resolve The Supply Chain

Starting from `mmc@fa00000/vmmc-supply`:

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

Should the engineer write `fa00000.mmc` to the `sdhci-am654` driver's `bind` file? Should they set `driver_override`? Should they unbind/rebind the regulator?

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
host SD-card merge
host SD-card + low-power merge
U-Boot pre-handoff
raw boot FDT
live tree
```

Name the first checkpoint at which the intended SD-card product becomes invalid. Distinguish the immediate consumer symptom from the product-policy root cause.

## Task 8: Define The Fix

Design:

- manifest dependency/conflict rules
- safe boot behavior for invalid profile combinations
- final-tree assertions
- runtime deferred-probe checks
- hardware functional tests
- rollback behavior

Do not “fix” the incident by removing `vmmc-supply`, marking the regulator always-on, or making the driver ignore its supply.

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

Also copy/hash raw files for consumer `compatible`, `status`, `interrupts`, `clocks`, `clock-names`, `power-domains`, and `vmmc-supply`; supplier GPIO and compatible properties; root identity; and non-sensitive `/chosen` fields.

Capture:

```bash
readlink -f /sys/bus/platform/devices/fa00000.mmc/of_node
readlink -f /sys/bus/platform/devices/fa00000.mmc/subsystem
readlink -f /sys/bus/platform/devices/fa00000.mmc/driver 2>/dev/null || true
cat /sys/bus/platform/devices/fa00000.mmc/modalias
cat /sys/bus/platform/devices/fa00000.mmc/uevent
```

Repeat for regulator and controller devices using discovered paths. Save driver registration evidence, debugfs regulator/GPIO state, `/proc/interrupts`, and supplier/consumer links. Redact seeds, secrets, and product identifiers according to policy.

### Binary-Safe Decode

Use NUL-aware output for names:

```bash
node=/sys/firmware/devicetree/base/bus@f4000/mmc@fa00000
tr '\0' '\n' <"$node/compatible"
tr '\0' '\n' <"$node/clock-names"
```

Use byte grouping or the reconstructed DTB for cells:

```bash
xxd -p -c 4 "$node/interrupts"
xxd -p -c 4 "$node/clocks"
xxd -p -c 4 "$node/power-domains"
xxd -p -c 4 "$node/vmmc-supply"

fdtget -tx live-tree.dtb /bus@f4000/mmc@fa00000 clocks
fdtget -tx live-tree.dtb /bus@f4000/mmc@fa00000 vmmc-supply
```

Decode `clocks` with `k3_clks`' `#clock-cells = <2>` (device 58, clocks 3 and 4), `power-domains` with `k3_pds`, `interrupts` through the inherited GIC parent and its three cells, and the supply as a bare regulator phandle. Pair entries with binding-defined names.

### Recursive Supplier Trace

| Stage | Live node | Linux device | Driver state | Evidence/conclusion |
|---|---|---|---|---|
| SD controller `fa00000.mmc` | present, okay | platform device present | deferred/no link | “vmmc regulator not available” |
| `vdd_mmc1` (`regulator-2`) | present, available | platform device present | deferred/no link | “can't get GPIO”: `exp1` does not exist |
| `exp1` (`gpio@70`) | present, available locally | no I2C client `1-0070` | none | parent bus unavailable, so child not enumerated |
| `main_i2c1` (`i2c@20010000`) | present, disabled | no platform device | unavailable | low-power overlay changed `status` |

The expander's missing `status` normally makes it available relative to its own node, but its disabled parent prevents the bus/controller path required to create the I2C client.

The first missing runtime transition is `main_i2c1` availability/population. Everything above it defers correctly.

### Boot Versus Live Interpretation

`H_HANDOFF == hash(/sys/firmware/fdt)` strongly supports byte-identical exported boot input. `H_REBUILT_LIVE` differs because `dtc -I fs` serializes a new blob without original header layout, padding, or FDT reservation map and may order content differently.

Because the raw boot FDT already contains `status = "disabled"` on `i2c@20010000`, the low-power overlay applied before Linux. With a later runtime overlay, live semantics could diverge while `/sys/firmware/fdt` remains the boot snapshot.

Compare both captures semantically with the same `dtc` version and focused property queries.

### State Classification

- SD controller node: present and available
- SD controller device: created on the platform bus
- SD controller match: the built-in `sdhci-am654` driver matches `ti,am64-sdhci-4bit`
- SD controller probe: deferred on its vmmc regulator; not terminally failed and not bound
- regulator: device created, probe deferred on its GPIO provider
- GPIO expander: node present and locally available, but no I2C device populated
- `main_i2c1`: node unavailable due to final `status = "disabled"`; no controller device
- subsystem: no MMC host and no `mmcblk1`, as expected downstream of deferral; the TMP100 sensors and the LED expander on the same bus are missing too

### Manual Bind Decision

Do not force-bind the SD controller. Normal matching already succeeds far enough to run probe, which deliberately defers. A manual bind retries the same missing supply and adds noise.

Do not set `driver_override`; match metadata is proven correct, and bypassing it cannot create the supplier.

Do not unbind/rebind the regulator; it is not bound. Its deferral is valid. Forcing or bypassing the enable GPIO would leave the card unpowered or violate the hardware power contract.

The safe read-only test is to trace and decode the supplier chain, compare final-tree checkpoints, and reproduce the overlay composition on the host. Correct the manifest and reboot with a valid immutable final tree.

### First Divergence And Root Cause

The base plus SD-card overlay is valid. The first invalid checkpoint is host-equivalent composition after `low-power-profile.dtbo`, where `main_i2c1` becomes disabled although the SD card power path requires its GPIO expander.

Immediate symptom:

```text
SD controller probe defers because its vmmc regulator is unavailable
```

Dependency cause:

```text
regulator defers because its GPIO provider device does not exist
```

Root cause:

```text
product manifest permits conflicting SD-card and low-power overlays;
boot selection applies both without checking the sdhci1 -> vdd_mmc1 -> exp1
-> main_i2c1 dependency
```

### Fix And Regression Gates

Manifest rules:

```text
product-sdcard requires main_i2c1, exp1, and vdd_mmc1
low-power-no-expander conflicts product-sdcard
SD-card composition must leave main_i2c1 status okay
```

Boot behavior:

- reject conflicting selection before applying any overlay
- compose from pristine authenticated base
- on mandatory overlay/assertion failure, discard working FDT
- select a defined safe recovery profile or stop normal boot

Final-tree CI assertions:

- `main_i2c1` available
- expander child present at address `0x70`
- `vmmc-supply` resolves to `regulator-2`
- regulator GPIO resolves to `exp1` line 3, `GPIO_ACTIVE_HIGH`
- SD controller resources and names pass the `sdhci-am654.yaml` schema
- every supported merged tree passes schema and product dependency checks
- the rejected SD-card + low-power combination fails deterministically

Runtime gates:

- no target devices remain in `devices_deferred` after expected module/coldplug completion
- SD controller, regulator, expander, and I2C controller have expected driver links
- an MMC host appears and an inserted card enumerates as `mmcblk1`
- card power-up, read/write integrity, hot insertion, suspend/resume, and reboot are tested

Rollback must choose a mutually compatible base, overlay set, kernel, modules, and firmware; it must not reintroduce the invalid profile pairing.

## Completion Criteria

You have completed the lab when you can prove the consumer's DT and match data are correct, recursively locate the disabled I2C parent as the first missing runtime transition, and attribute that state to the pre-boot overlay manifest rather than forcing the SD controller driver.

## Authoritative References

- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Linux driver infrastructure and deferred probe](https://docs.kernel.org/driver-api/infrastructure.html)
- [Linux sysfs firmware OF ABI](https://docs.kernel.org/admin-guide/abi-testing-files.html)
- [Linux Devicetree overlay notes](https://docs.kernel.org/devicetree/overlay-notes.html)

## Next Step

Continue with [Pinctrl, GPIOs, And Interrupts](../pinctrl-gpios-and-interrupts.md), using this module's live-tree, device-model, and deferred-probe evidence during pin, GPIO, and interrupt bring-up.
