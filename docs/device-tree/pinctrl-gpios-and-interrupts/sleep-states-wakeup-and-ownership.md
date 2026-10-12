---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Sleep States, Wakeup, And Ownership

Wakeup succeeds only when the signal remains electrically valid, its pad and interrupt path stay powered, the controller can recognize the event in the selected sleep state, and Linux arms the device as a wake source.

## `wakeup-source` Expresses Capability

Two real forms. The Toradex Verdin AM62 module marks one GPIO key as a wake source (`k3-am62-verdin.dtsi`), and the PHYTEC phyCORE-AM64x marks its RTC (`k3-am64-phycore-som.dtsi`):

```dts
verdin_gpio_keys: gpio-keys {
        compatible = "gpio-keys";
        pinctrl-names = "default";
        pinctrl-0 = <&pinctrl_ctrl_wake1_mico>;

        verdin_key_wakeup: key-wakeup {
                debounce-interval = <10>;
                /* Verdin CTRL_WAKE1_MICO# (SODIMM 252) */
                gpios = <&main_gpio0 32 GPIO_ACTIVE_LOW>;
                label = "Wake-Up";
                linux,code = <KEY_WAKEUP>;
                wakeup-source;
        };
};

i2c_som_rtc: rtc@52 {
        compatible = "microcrystal,rv3028";
        reg = <0x52>;
        interrupt-parent = <&main_gpio1>;
        interrupts = <70 IRQ_TYPE_EDGE_FALLING>;
        wakeup-source;
};
```

TI's AM62x boards also use a phandle-list form that names the system low-power states in which a device can wake the system (`k3-am62-lp-sk.dts`, with the states defined in `k3-am62.dtsi`):

```dts
&mcu_mcan0 {
        wakeup-source = <&system_partial_io>,
                        <&system_deep_sleep>,
                        <&system_mcu_only>,
                        <&system_standby>;
};
```

`wakeup-source` says the device is capable of waking the system under its binding. It does not guarantee that wake is enabled by policy, that the driver calls the relevant PM/IRQ APIs, or that the controller remains powered.

Separate:

- hardware wake capability
- DT declaration
- driver initialization of wakeup support
- userspace enable/disable policy
- IRQ wake arming for the current suspend transition

## Active And Sleep Pinctrl States

The AM62x MCU CAN wake state from [Pinmux, Pin Configuration, And States](pinmux-pin-configuration-and-states.md#provider-syntax-is-binding-specific) keeps the RX pad muxed to CAN and adds `PIN_WKUP_EN` so the always-on I/O logic can detect bus activity while the SoC sleeps:

```dts
AM62X_IOPAD(0x038, PIN_INPUT | PIN_WKUP_EN, 0) /* (B3) MCU_MCAN0_RX */
```

The sleep state must preserve the signal path needed for wake. Common mistakes are:

- muxing the pad to an unrelated low-power function
- disabling input buffers
- removing the pull that defines the inactive level
- driving against an external source
- powering down the pin bank or level shifter
- changing polarity relative to the armed trigger

A low-leakage state is not correct if it prevents a required wake event.

## Wake Controller Topology

Some SoCs route normal runtime interrupts through one controller and wake events through an always-on controller or a special wake pin subset. The binding may provide a separate wake interrupt or mapping.

Trace both paths:

```text
runtime: device → GPIO bank → main interrupt controller → CPU
wake:    device → always-on pad/controller → PM logic → resume
```

They may share the physical line but differ after the pad. Test that the resume path restores the runtime controller before pending status is lost or generates a storm.

## GPIO Persistence And Power Loss

GPIO flag encodings may express persistent versus transitory behavior, but actual retention depends on controller power, silicon retention, firmware, and driver restore support. A line on an unpowered I2C expander cannot retain a software-requested output merely because DT calls it persistent.

For enables and resets, define the safe state during:

- runtime suspend
- shallow system sleep
- deep/off sleep
- warm reset
- watchdog reset
- bootloader handoff
- kernel crash

External pulls often provide the only guaranteed state when all software-controlled domains are off.

## Ordering Suspend And Resume

A typical suspend sequence must coordinate:

1. quiesce device traffic
2. program device wake condition
3. clear stale pending status
4. select sleep pins and biases
5. arm IRQ wake
6. power down permitted domains

Resume reverses dependencies carefully, but pending wake status may need capture before normal initialization clears it. The exact sequence is device- and platform-specific.

Race examples include an edge between clearing status and enabling wake, or a level already active when wake is armed. Tests must inject events at transition boundaries, not only after the system is fully asleep.

## Shared Ownership Across Firmware

Boot firmware, secure firmware, management processors, and the kernel may all touch pads or wake controllers. Define ownership per state:

| Phase | Owner | Required handoff evidence |
|---|---|---|
| reset/boot | ROM/bootloader | reset defaults and boot mux |
| runtime | Linux driver/pinctrl | selected state and consumer ownership |
| deep sleep | secure/PM firmware | wake configuration and retained context |
| resume | firmware then Linux | ordering and status preservation |

If firmware rewrites pinctrl registers behind Linux, the runtime DT alone does not describe effective state. Capture firmware versions and controller registers where supported.

## Security And Safety

Wake lines can cross trust boundaries. A noisy or attacker-controlled GPIO can drain batteries or create denial of service. Debug pins can expose privileged interfaces when a sleep/resume state accidentally remuxes them.

Review:

- debounce and rate limiting
- whether wake source enable is privileged
- pad ownership/firewalls in secure states
- fail-safe levels during power loss
- leakage and back-powering through unpowered devices
- production disabling of JTAG/UART alternate functions

## Test Matrix

For every required wake source, test:

- inactive and already-active conditions at suspend entry
- a pulse at each suspend transition phase
- repeated and bouncing events
- simultaneous wake sources
- shallow and deepest supported sleep states
- charger/battery/external-power variants
- cold boot after the line was held active
- resume followed immediately by another event

Collect wakeup reason, IRQ counters, pin state, power-domain state, and physical waveform.

## Authoritative References

- [Linux device power-management basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [Linux pinctrl standard states and PM helpers](https://docs.kernel.org/driver-api/pin-control.html)
- [Linux generic IRQ wake handling](https://docs.kernel.org/core-api/genericirq.html)
- [Linux GPIO consumer semantics](https://docs.kernel.org/driver-api/gpio/consumer.html)

## Next Step

Apply the full signal path in the [Pin, GPIO, And Interrupt Bring-Up Lab](pin-gpio-and-interrupt-bring-up-lab.md).
