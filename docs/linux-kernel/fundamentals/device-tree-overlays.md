---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Device Tree Overlays

## What Problem Does This Solve?

Device Tree overlays describe hardware additions, capes, hats, mezzanine cards, FPGA loads, or board variants without replacing the entire base Device Tree.

An overlay is a patch applied to a base tree:

```text
base DTB
+ overlay DTBO
-> effective runtime Device Tree
```

Overlays are useful when the base board is stable but optional hardware changes. They are risky when used to hide unclear board ownership, because an overlay must match the exact base tree it targets.

## Core Concepts

- overlay source
- `.dtso`
- `.dtbo`
- `/plugin/`
- fragment
- `target`
- `target-path`
- `__overlay__`
- labels
- symbols
- `-@` compilation
- bootloader-applied overlay
- kernel-applied overlay
- overlay order
- board variant policy

## Mental Model

An overlay is not a separate hardware universe. It mutates an existing tree.

```text
base tree has labels and nodes
overlay references a base node
overlay adds, changes, or enables child nodes/properties
driver sees only final runtime tree
```

Therefore, an overlay depends on:

- the base DTB version
- the labels or paths it targets
- overlay application order
- bootloader or kernel overlay support
- provider nodes already existing in the base tree

## Minimal Overlay Example

Overlay source:

```dts
/dts-v1/;
/plugin/;

&i2c2 {
    status = "okay";

    temp_sensor: temperature-sensor@48 {
        compatible = "example,tmp102";
        reg = <0x48>;
        status = "okay";
    };
};
```

This targets the base tree label `i2c2` and adds an I2C child device.

Compile:

```sh
dtc -@ -I dts -O dtb -o temp-sensor.dtbo temp-sensor.dtso
```

In kernel builds, overlay build rules are often integrated with `make dtbs`, depending on architecture and BSP.

## Fragment Syntax

Some overlays use explicit fragments:

```dts
/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target = <&i2c2>;

        __overlay__ {
            status = "okay";

            temperature-sensor@48 {
                compatible = "example,tmp102";
                reg = <0x48>;
            };
        };
    };
};
```

`target = <&i2c2>;` requires the base DTB to expose the `i2c2` label as a symbol. That usually means the base was compiled with symbol support.

If labels are not available, use `target-path`:

```dts
fragment@0 {
    target-path = "/soc/i2c@2000000";

    __overlay__ {
        status = "okay";
    };
};
```

`target-path` is more brittle because paths can change across base DTB revisions.

## What Overlays Can Do

Common overlay operations:

- enable a disabled bus
- add a child device
- add pinctrl settings
- add GPIO hogs
- add regulators
- add display panels
- add sound-card wiring
- add FPGA-attached devices
- change selected properties for a board variant

Example enabling a bus and adding a device:

```dts
&spi1 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&spi1_pins>;

    adc@0 {
        compatible = "example,spi-adc";
        reg = <0>;
        spi-max-frequency = <1000000>;
    };
};
```

## What Overlays Should Not Hide

Avoid using overlays to avoid making a real product decision about:

- base board ownership
- permanent hardware revisions
- security policy
- regulator constraints
- incompatible pinmux choices
- production boot flow
- driver binding design

If every product always uses the hardware, put it in the base board DTS. Use overlays for actual optionality or deployment mechanisms that require them.

## Overlay Application Paths

Overlays may be applied by:

| Layer | Example |
| --- | --- |
| Bootloader | U-Boot `fdt apply`, board config files, extlinux overlay entries |
| Firmware | Raspberry Pi-style firmware flows, vendor firmware |
| Kernel | configfs overlay support on systems that enable it |
| Build system | pre-composed DTB/FIT image with selected overlays |

Know which one your board uses. The driver only sees the final tree.

## U-Boot-Style Flow

A typical bootloader flow:

```text
load base DTB
load overlay DTBO
apply overlay to base
boot kernel with modified DTB
```

Debug from U-Boot:

```text
fdt addr ${fdt_addr_r}
fdt print
fdt apply ${overlay_addr_r}
```

Exact commands depend on board environment and U-Boot configuration.

## Kernel Configfs Overlay Flow

Some systems support runtime overlays through configfs:

```sh
mount -t configfs none /sys/kernel/config
mkdir /sys/kernel/config/device-tree/overlays/temp
cat temp-sensor.dtbo > /sys/kernel/config/device-tree/overlays/temp/dtbo
```

Removal:

```sh
rmdir /sys/kernel/config/device-tree/overlays/temp
```

Use runtime overlays carefully. Removing an overlay while drivers own devices can be complex and board-specific. Many products prefer boot-time overlay application.

## Build Requirements

Label-based overlays need symbols in the base DTB. This usually requires compiling with:

```sh
dtc -@
```

In kernel builds, the overlay support and symbol generation are controlled by kernel and architecture build rules.

Common build outputs:

```text
*.dtb
*.dtbo
```

Find them:

```sh
find build-arm64 -name '*.dtb' -o -name '*.dtbo'
```

## Runtime Verification

After boot, check the final tree:

```sh
find /proc/device-tree -name compatible -print
tr -d '\0' < /proc/device-tree/path/to/node/compatible
```

Check logs:

```sh
dmesg | grep -i -E 'overlay|of:|probe|defer'
```

Check whether the device exists:

```sh
find /sys/bus -name '*modalias*' -exec grep -H 'example,tmp102' {} \; 2>/dev/null
```

If the node appears but the driver does not probe, the overlay applied successfully and you should debug matching/configuration/resources. If the node does not appear, debug overlay application first.

## Overlay Compatibility

An overlay can break when the base DTB changes:

- target label removed or renamed
- node path changed
- provider phandle changed
- bus disabled differently
- address/size cells changed
- pinctrl label changed
- regulator node renamed
- conflicting overlay applied earlier

Treat overlays as versioned with the base DTB.

For product releases, record:

```text
base_dtb:
overlay_dtbo:
application_order:
bootloader_version:
kernel_release:
```

## Example: Add A GPIO Button

```dts
/dts-v1/;
/plugin/;

&{/} {
    gpio-keys {
        compatible = "gpio-keys";

        button-user {
            label = "user";
            gpios = <&gpio1 12 GPIO_ACTIVE_LOW>;
            linux,code = <KEY_ENTER>;
            debounce-interval = <10>;
        };
    };
};
```

This uses an existing kernel binding and input subsystem driver. You do not need a custom driver for a standard GPIO key.

## Example: Enable A Disabled Device

Base:

```dts
&uart3 {
    status = "disabled";
};
```

Overlay:

```dts
/dts-v1/;
/plugin/;

&uart3 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart3_pins>;
    status = "okay";
};
```

This is common for optional board headers. It is only safe if the pins and electrical wiring are valid for the mounted hardware.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| Overlay application fails | missing symbols, bad target, incompatible base | bootloader logs, `dtc -@`, labels |
| Overlay applies but node absent | wrong boot path or not selected | `/proc/device-tree`, boot config |
| Node appears but driver does not probe | config/matching/resource issue | compatible, `.config`, `dmesg` |
| Wrong device probes | overly generic compatible or duplicate address | binding, bus node |
| Probe defers forever | provider added by another missing overlay | regulator/clock/pinctrl nodes |
| Board becomes unstable | overlay conflicts with pinmux or power policy | pinctrl, regulators, schematic |

## Common Mistakes

- Applying an overlay built for another base DTB.
- Using label targets when the base DTB lacks symbols.
- Applying overlays in the wrong order.
- Forgetting to deploy the `.dtbo`.
- Editing overlay source but booting a precomposed FIT image that does not include it.
- Treating runtime overlay removal as always safe.
- Using overlays for permanent hardware that belongs in the base board DTS.
- Ignoring binding validation.

## Practice Exercises

### Exercise 1: Confirm Overlay Presence

Boot a system with an overlay and check:

```sh
dmesg | grep -i overlay
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'your-node|your-compatible' /tmp/running.dts
```

### Exercise 2: Use A Standard Binding

Add a `gpio-keys` overlay for a spare GPIO button. Confirm the input device appears:

```sh
cat /proc/bus/input/devices
```

### Exercise 3: Break The Target Deliberately

Change a target label to a nonexistent label and observe the bootloader or kernel error. This teaches where overlay application errors are reported on your board.

## Debugging Checklist

- Which layer applies overlays?
- Is the overlay selected in the boot configuration?
- Does the base DTB expose labels/symbols if the overlay uses label targets?
- Does the overlay target the correct base node?
- Are overlays applied in the intended order?
- Does `/proc/device-tree` show the final node/property?
- Does the driver match the resulting `compatible`?
- Are provider nodes and resources present?

## Related Topics

- [Device Tree Hardware Description](device-tree-hardware-description.md)
- [Device Tree Builds](../../build-systems/advanced/linux-kernel/device-tree-builds.md)
- [Device Tree Matching From Drivers](device-tree-matching.md)
- [Device Tree Binding Validation](../../build-systems/advanced/linux-kernel/device-tree-binding-validation.md)

## Official References

- [Linux and the Devicetree](https://docs.kernel.org/devicetree/usage-model.html)
- [Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Devicetree Bindings](https://docs.kernel.org/devicetree/bindings/index.html)
