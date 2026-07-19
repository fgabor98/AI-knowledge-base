---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Pinmux, Pin Configuration, And States

Pinmux chooses which internal signal reaches a package pad. Pin configuration controls electrical properties such as bias, drive strength, slew rate, input enable, and single-ended output behavior. A pinctrl state groups the settings a consumer needs for one operating phase.

## Provider Syntax Is Binding-Specific

A consumer references states generically:

```dts
&uart3 {
        pinctrl-names = "default", "sleep";
        pinctrl-0 = <&uart3_default_pins>;
        pinctrl-1 = <&uart3_sleep_pins>;
};
```

The provider-side state syntax varies by controller. An illustrative provider might use:

```dts
uart3_default_pins: uart3-default-pins {
        pins = "PA4", "PA5";
        function = "uart3";
        bias-disable;
        drive-strength = <8>;
};

uart3_sleep_pins: uart3-sleep-pins {
        pins = "PA4", "PA5";
        function = "gpio";
        bias-pull-up;
};
```

Other bindings use `groups`, numeric pin tuples, vendor macros, nested mux/config nodes, or separate function and configuration references. Never copy provider syntax across SoC families without reading that pin controller's schema.

## Positional State Mapping

`pinctrl-names` maps positionally to `pinctrl-N` properties:

| Name index | Property | Meaning |
|---:|---|---|
| 0 | `pinctrl-0` | state named `default` |
| 1 | `pinctrl-1` | state named `sleep` |

Each `pinctrl-N` can reference more than one configuration node when several controllers or groups make up one logical state:

```dts
pinctrl-0 = <&uart3_data_pins &uart3_flow_control_pins>;
```

The state is the entire referenced set. Partial selection can leave a clock, chip select, or flow-control line in its boot mux.

## Standard State Roles

Linux recognizes conventional names:

- `default`: normal operating configuration, normally selected before probe
- `init`: temporary configuration selected before probe when both `init` and `default` exist
- `idle`: lower-activity runtime state selected through PM helpers
- `sleep`: system/runtime sleep configuration selected through PM helpers

Names do not guarantee automatic use in every driver and power path. The device core binds standard states, but drivers and buses must invoke appropriate PM transitions. Verify actual selection at runtime.

`init` is useful when a driver must probe with safe pins before activating the full interface. It must not create an electrical glitch during the transition to `default`.

## Generic Pin Configuration Concepts

Common concepts include:

- `bias-disable`, `bias-pull-up`, `bias-pull-down`, and keeper modes
- `drive-strength` in units defined by the binding
- `slew-rate`
- `input-enable` or `input-disable`
- `output-high` or `output-low`
- `drive-open-drain`, `drive-open-source`, or push-pull
- Schmitt trigger and power-source/voltage selection where supported

Support is hardware- and binding-specific. Adding a generic-looking property that the provider cannot implement may fail schema validation, be ignored, or cause probe errors. External resistors and level shifters remain physical facts even when internal bias is configured.

## Mux And Electrical Configuration Are Orthogonal

A UART RX pad can be muxed correctly but float because its bias is wrong. A line can have the correct pull-up but still be connected internally to SPI rather than GPIO. Diagnose each axis independently.

For open-drain protocols, prove all of these:

- mux selects the intended controller
- output mode releases rather than drives high
- an adequate pull-up exists in the correct voltage domain
- drive strength, slew, and rise time meet the bus timing
- no second device drives the line push-pull

## State Ownership And Conflicts

The pinctrl core tracks mux ownership when the provider supports it. Two enabled peripherals selecting overlapping groups usually indicate an invalid board description. Some hardware permits GPIO observation alongside a peripheral function; other hardware requires exclusive mux control. The pinctrl provider describes and enforces that topology.

Do not resolve a conflict by deleting one device's pinctrl state while leaving both devices enabled. Decide which hardware function is actually routed on the board, disable the other consumer, and model shared/conditional ownership explicitly if the binding supports it.

## Bootloader State Is Not A Contract

A peripheral may work only because firmware configured its pins. That is a fragile hidden dependency: reset, suspend, driver reload, or a different bootloader can expose the missing state.

The OS description should establish every state it owns. When firmware must retain ownership—for example, secure pins or boot-critical memory—document that boundary and avoid conflicting Linux consumers.

## Runtime Evidence

With debugfs mounted and the relevant kernel options enabled:

```sh
cat /sys/kernel/debug/pinctrl/pinctrl-handles
cat /sys/kernel/debug/pinctrl/pinctrl-maps
cat /sys/kernel/debug/pinctrl/*/pinmux-pins
cat /sys/kernel/debug/pinctrl/*/pinconf-pins
cat /sys/kernel/debug/pinctrl/*/gpio-ranges
```

Provider output is diagnostic, not a stable userspace ABI. Correlate controller-local pin names with the datasheet, package ball, and schematic net.

Writing debugfs `pinmux-select` changes live mux state and can electrically contend with peripherals. Use read-only inspection unless a controlled experiment explicitly requires mutation.

## Review Checklist

- Does each consumer state reference every required group?
- Do state names and numeric properties align positionally?
- Is provider syntax taken from the correct SoC binding?
- Are mux and electrical properties reviewed separately?
- Do drive and bias settings agree with voltage domain and external components?
- Are boot, active, idle, sleep, reset, and error states safe?
- Can two enabled consumers claim the same pad?
- Does runtime pinctrl state match the final DTB and intended PM phase?

## Authoritative References

- [Linux pin control subsystem](https://docs.kernel.org/driver-api/pin-control.html)
- [Linux Device Tree bindings index](https://docs.kernel.org/devicetree/bindings/index.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Linux power-management device basics](https://docs.kernel.org/driver-api/pm/devices.html)

## Next Step

Continue with [GPIO Controllers, Ranges, Line Names, And Hogs](gpio-controllers-ranges-line-names-and-hogs.md).
