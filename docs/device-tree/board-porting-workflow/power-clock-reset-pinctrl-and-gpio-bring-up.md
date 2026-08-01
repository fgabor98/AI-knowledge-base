---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Power, Clock, Reset, Pinctrl, And GPIO Bring-Up

Most peripheral failures begin in a supplier chain, not the peripheral node. Before enabling a consumer, prove its parent bus, pins, rails, clocks, resets, GPIO enables, and interrupts in the order the hardware requires.

## Draw The Dependency Graph

For each consumer create a graph from schematics and bindings:

```text
PMIC input rail
  -> regulator provider
     -> GPIO expander supply
        -> expander GPIO enable
           -> peripheral regulator
              -> peripheral supply

oscillator -> clock controller -> peripheral clock
reset controller/GPIO -> peripheral reset
pinctrl controller -> default/sleep state
interrupt controller/GPIO -> interrupt parent/specifier
parent bus -> consumer address/chip select
```

Annotate provider cell counts, polarity, voltage, startup delay, shared ownership, and boot state.

## Start With Always-On Foundations

Enable and verify in this general order:

1. PMIC/system-controller communication needed for other rails
2. primary fixed clocks/oscillators
3. pin controller and GPIO controllers
4. interrupt and reset controllers
5. board-level fixed regulators and enables
6. parent buses and expanders
7. switched rails/clocks/resets
8. consumers

Actual order follows platform architecture. A GPIO-controlled regulator cannot become usable before the GPIO controller and its own supply are available.

## Model Regulators From The Power Tree

For each rail verify:

- provider binding and address
- input supply (`*-supply`) when represented
- voltage range and whether it is fixed or programmable
- enable GPIO controller, line, and active polarity
- startup and off-on delay
- always-on or boot-on requirement based on hardware/boot ownership
- consumers and supply property names from their bindings
- suspend state and shared users

Schematic example:

```dts
vcc_periph_3v3: regulator-periph-3v3 {
    compatible = "regulator-fixed";
    regulator-name = "periph_3v3";
    regulator-min-microvolt = <3300000>;
    regulator-max-microvolt = <3300000>;
    gpio = <&gpio2 11 GPIO_ACTIVE_HIGH>;
    enable-active-high;
    startup-delay-us = <5000>;
    vin-supply = <&vcc_main_5v0>;
};
```

Check the current binding: accepted GPIO property spelling and polarity representation can vary by binding/version. Never infer `GPIO_ACTIVE_*` from a signal name alone.

## Treat Pinctrl As Electrical Configuration

Verify:

- correct package ball/pad and mux function
- pad voltage domain matches external level
- pull direction and whether an external resistor already defines it
- drive strength and slew appropriate to the interface
- open-drain/open-source requirements
- input enable and Schmitt behavior if applicable
- default, init, sleep, and idle states
- ownership conflicts with boot straps, JTAG, or another peripheral

Pinctrl can be selected before driver probe. An unsafe default can drive against another device before the consumer logs anything.

## Bring Up GPIOs By Meaning

Decode a GPIO specifier with the provider's `#gpio-cells` binding. Record:

```text
controller and power domain
line/offset and package pin
active logical level versus electrical level
direction at boot, probe, runtime, suspend
external pull and safe inactive state
shared/wired behavior
consumer ownership
```

Avoid exporting or manually toggling a line already owned by a kernel driver. For experiments, use a disposable lab board, verify ownership in gpio debug state, and return to a known-safe state.

## Verify Clocks And Resets

For clocks:

- provider and specifier cell meaning
- source/parent and expected rate
- `clock-names` ordering
- assigned parent/rate only when the binding/platform requires board policy
- shared clock impact on other consumers
- bootloader-to-kernel handoff state

For resets:

- controller, ID, polarity/semantics
- shared versus exclusive reset
- assertion state and pulse/delay requirements
- relationship to power and clock sequencing
- child devices affected by a shared reset

A clock present in debugfs or a deasserted reset does not prove the consumer received a valid reference signal at its pin.

## Bring Up Interrupts Last In The Local Chain

Before enabling interrupt-driven operation:

1. prove device identity through register access or bus response
2. verify interrupt parent and hierarchy
3. decode every interrupt cell using the parent binding
4. confirm schematic polarity and trigger behavior
5. inspect pin mux/input/bias for GPIO interrupts
6. clear latched device status before unmasking
7. measure interrupt count under one controlled event
8. test absence of storms and correct wake behavior

Do not switch trigger type merely until logs stop; that can hide a wiring or device-status problem.

## Use Runtime Evidence

Paths depend on configuration and subsystem:

```bash
sudo cat /sys/kernel/debug/devices_deferred 2>/dev/null
sudo cat /sys/kernel/debug/gpio 2>/dev/null
sudo cat /sys/kernel/debug/clk/clk_summary 2>/dev/null
cat /proc/interrupts
dmesg --color=never | grep -Ei 'regulator|clock|reset|pinctrl|gpio|irq|defer'
```

Also map the consumer's live DT properties to actual provider nodes and Linux devices. Debugfs is diagnostic, not a stable userspace ABI; absence may reflect configuration or mount policy.

## Diagnose One Provider Edge At A Time

```text
consumer property exists?
  -> phandle resolves to intended live provider?
  -> provider specifier cell count/value correct?
  -> provider node available and populated?
  -> provider driver bound?
  -> physical resource enabled/configured?
  -> consumer probe retries/succeeds?
  -> subsystem function works?
```

Persistent deferral often names only the nearest unavailable resource. Follow dependencies recursively.

## Safe Enablement Patch Pattern

Separate changes:

1. binding/compatible support if missing
2. provider node correction/addition
3. board pinctrl state
4. board regulator/reset/clock relationship
5. consumer node and supplies/resources while disabled
6. final `status = "okay"`
7. optional performance modes after conservative function

This gives review and bisection meaningful boundaries.

## Stage Exit Gate

```text
[ ] every enabled consumer has a reviewed dependency graph
[ ] voltage/polarity/pinmux/drive values trace to hardware evidence
[ ] provider specifiers decode correctly against live providers
[ ] no critical persistent deferred probes remain
[ ] clock/reset/regulator states match expected lifecycle
[ ] controlled interrupts increment without storms
[ ] suspend/reset/power-cycle behavior is safe for stage
[ ] logs and measurements prove physical operation, not only binding
```

## Further Reading

- [Provider-Consumer Relationships](../provider-consumer-relationships.md)
- [Clocks, Resets, Regulators, And Power](../clocks-resets-regulators-and-power.md)
- [Pinctrl, GPIOs, And Interrupts](../pinctrl-gpios-and-interrupts.md)
- [Linux pin control documentation](https://docs.kernel.org/driver-api/pin-control.html)
- [Buses, Storage, Networking, And Peripheral Enablement](buses-storage-networking-and-peripheral-enablement.md)
