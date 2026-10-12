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

A consumer references states generically. The AM62x LP SK MCU CAN controller has a normal and a wake-up state (`k3-am62-lp-sk.dts`):

```dts
&mcu_mcan0 {
        pinctrl-names = "default", "wakeup";
        pinctrl-0 = <&mcu_mcan0_tx_pins_default>, <&mcu_mcan0_rx_pins_default>;
        pinctrl-1 = <&mcu_mcan0_tx_pins_default>, <&mcu_mcan0_rx_pins_wakeup>;
};
```

The provider-side state syntax varies by controller. TI K3 SoCs use `pinctrl-single`, where each entry is a pad-register offset and a raw configuration value built with macros from `k3-pinctrl.h`:

```dts
mcu_mcan0_rx_pins_default: mcu-mcan0-rx-default-pins {
        pinctrl-single,pins = <
                AM62X_IOPAD(0x038, PIN_INPUT, 0) /* (B3) MCU_MCAN0_RX */
        >;
};

mcu_mcan0_rx_pins_wakeup: mcu-mcan0-rx-wakeup-pins {
        pinctrl-single,pins = <
                AM62X_IOPAD(0x038, PIN_INPUT | PIN_WKUP_EN, 0) /* (B3) MCU_MCAN0_RX */
        >;
};
```

Qualcomm SoCs use generic named pins and functions instead (`arch/arm64/boot/dts/qcom/sm8250.dtsi`):

```dts
qup_uart12_default: qup-uart12-default-state {
        pins = "gpio34", "gpio35";
        function = "qup12";
};
```

Other bindings use `groups`, nested mux/config nodes, or vendor properties. Never copy provider syntax across SoC families without reading that pin controller's schema.

## Positional State Mapping

`pinctrl-names` maps positionally to `pinctrl-N` properties:

| Name index | Property | Meaning on `mcu_mcan0` |
|---:|---|---|
| 0 | `pinctrl-0` | state named `default` |
| 1 | `pinctrl-1` | state named `wakeup` |

Each `pinctrl-N` can reference more than one configuration node when several groups make up one logical state, as both CAN states do with separate TX and RX nodes. The AM642 SK Ethernet does the same across two RGMII groups:

```dts
&cpsw3g {
        pinctrl-names = "default";
        pinctrl-0 = <&rgmii1_pins_default>, <&rgmii2_pins_default>;
};
```

The state is the entire referenced set. Partial selection can leave a clock, data, or control line in its boot mux.

## Standard State Roles

Linux recognizes conventional names:

- `default`: normal operating configuration, normally selected before probe
- `init`: temporary configuration selected before probe when both `init` and `default` exist
- `idle`: lower-activity runtime state selected through PM helpers
- `sleep`: system/runtime sleep configuration selected through PM helpers

Other names are driver-defined. The `wakeup` state on the AM62x MCU CAN above is not a core state; the M_CAN driver (`drivers/net/can/m_can/m_can.c`) looks it up at probe and selects it when wake-up is enabled through the ethtool Wake-on-LAN setting, returning to `default` when it is disabled. Names do not guarantee automatic use in every driver and power path. The device core binds standard states, but drivers and buses must invoke appropriate PM transitions. Verify actual selection at runtime.

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
