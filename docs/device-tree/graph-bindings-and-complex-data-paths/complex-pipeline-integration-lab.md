---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Complex Pipeline Integration Lab

This lab integrates a fictional SoC display controller, a four-lane DSI-to-eDP bridge controlled over I²C, a fixed panel, and a PWM backlight. It combines graph edges with control-bus placement, supplies, clocks, reset, mode constraints, and lifecycle diagnosis.

## Objectives

By the end, you should be able to:

- distinguish control-bus, provider-consumer, and graph relationships
- decode every port and endpoint according to its device binding
- verify reciprocal links and interface compatibility
- calculate a first-order link bandwidth requirement
- derive component binding and enable/disable order
- correlate the DT graph with DRM runtime objects
- diagnose graph, resource, format, and physical-link failures

## Hardware Contract

Assume the board has:

- a display controller output on DSI port 1
- four DSI data lanes wired in logical order 0, 1, 2, 3
- an I²C-controlled DSI-to-eDP bridge at address `0x2c`
- a 1.2 V bridge core supply and 1.8 V I/O supply
- a 24 MHz bridge reference clock
- an active-low bridge reset
- a fixed 1920×1080 panel with 148.5 MHz pixel clock
- a PWM backlight with a separate supply
- bridge port 0 as DSI input and port 1 as eDP output

The compatible strings are illustrative. Real components must use their exact schemas and timing definitions.

## Step 1: Read The Description

```dts
#include <dt-bindings/gpio/gpio.h>

&display_controller {
        status = "okay";

        ports {
                #address-cells = <1>;
                #size-cells = <0>;

                port@1 {
                        reg = <1>;

                        dsi_out: endpoint {
                                remote-endpoint = <&bridge_dsi_in>;
                                data-lanes = <0 1 2 3>;
                        };
                };
        };
};

&i2c3 {
        status = "okay";

        bridge@2c {
                compatible = "example,dsi-edp-bridge";
                reg = <0x2c>;
                vdd-supply = <&reg_1v2_bridge>;
                vccio-supply = <&reg_1v8>;
                clocks = <&bridge_refclk>;
                clock-names = "ref";
                reset-gpios = <&gpio3 9 GPIO_ACTIVE_LOW>;

                ports {
                        #address-cells = <1>;
                        #size-cells = <0>;

                        port@0 {
                                reg = <0>;

                                bridge_dsi_in: endpoint {
                                        remote-endpoint = <&dsi_out>;
                                        data-lanes = <0 1 2 3>;
                                };
                        };

                        port@1 {
                                reg = <1>;

                                bridge_edp_out: endpoint {
                                        remote-endpoint = <&panel_in>;
                                };
                        };
                };
        };
};

panel {
        compatible = "example,fixed-edp-panel";
        power-supply = <&reg_panel>;
        backlight = <&panel_backlight>;

        port {
                panel_in: endpoint {
                        remote-endpoint = <&bridge_edp_out>;
                };
        };
};

panel_backlight: backlight {
        compatible = "pwm-backlight";
        pwms = <&pwm2 0 50000 0>;
        power-supply = <&reg_backlight>;
        brightness-levels = <0 16 32 64 128 255>;
        default-brightness-level = <4>;
};
```

The SoC layer is assumed to define controller registers, interrupts, clocks, resets, power domains, and DSI PHY. Panel timings are omitted here because a real panel-specific binding may encode them in the compatible or require a timing node.

## Step 2: Classify Every Relationship

Build this inventory:

| Relationship | Model | Meaning |
|---|---|---|
| `bridge@2c` under `i2c3` | containment/bus | bridge control-register transport |
| bridge supplies, clock, reset | provider-consumer | resources required to operate bridge |
| `dsi_out` ↔ `bridge_dsi_in` | graph edge | DSI pixel-data link |
| `bridge_edp_out` ↔ `panel_in` | graph edge | eDP pixel-data link |
| panel `backlight` | provider/functional reference | panel illumination control |
| backlight PWM and supply | provider-consumer | PWM waveform and electrical power |

The bridge's I²C parent is not upstream in the pixel path. The panel backlight is not a graph vertex in that path.

## Step 3: Audit Ports And Reciprocity

Record:

```text
display controller port 1 output -> bridge port 0 input
bridge port 1 output             -> panel input
```

For each edge, prove both endpoint phandles resolve and point back to each other. Then verify device bindings define those port numbers and directions. Labels alone are not evidence.

## Step 4: Build An Interface Matrix

For the DSI edge:

| Constraint | Controller | Bridge | Result |
|---|---|---|---|
| data lanes | 0,1,2,3 | 0,1,2,3 | match |
| lane polarity/order | schematic + binding | binding capability | must verify |
| pixel format | driver modes | bridge inputs | intersection required |
| lane rate | controller maximum | bridge maximum | choose common rate |

Repeat for eDP using link rate, lane count, color depth, panel timing, and bridge output capability. Endpoint properties need not be identical when a component binding places configuration elsewhere, but the hardware contract must be coherent.

## Step 5: Estimate Bandwidth

For 1920×1080 at a 148.5 MHz pixel clock and 24 bits per pixel, the unencoded active pixel stream is:

```text
148.5 MHz × 24 bits = 3.564 Gbit/s
3.564 Gbit/s ÷ 4 lanes = 891 Mbit/s per DSI lane
```

This is only a lower-level estimate. Add protocol encoding, packet/blanking behavior, and implementation margins according to DSI mode. Then check DSI host, PHY, bridge input, bridge PLL, eDP output, panel, and memory-fetch bandwidth.

## Step 6: Derive Lifecycle Order

A plausible DRM-managed sequence is:

1. bind display controller, bridge, panel, and backlight components asynchronously
2. acquire and enable bridge/panel resources as callbacks require
3. validate the complete mode through controller, bridge, and panel
4. prepare the panel and downstream bridge stages
5. configure the controller and link rates
6. enable the pixel path
7. enable the panel, then backlight after valid image timing exists

On stop, disable backlight early, stop the source, disable/unprepare downstream components, and release resources. Confirm the actual ordering against DRM bridge/panel callbacks and hardware datasheets.

## Step 7: Collect Runtime Evidence

```sh
modetest -c -p
cat /sys/kernel/debug/dri/0/state 2>/dev/null
cat /sys/class/drm/card*-*/status 2>/dev/null
find /sys/bus/i2c/devices -maxdepth 2 -type l -o -type d
cat /sys/kernel/debug/clk/clk_summary
cat /sys/kernel/debug/regulator/regulator_summary
dmesg | grep -Ei 'drm|dsi|edp|bridge|panel|backlight|defer'
```

Expected evidence includes a bound I²C bridge, a complete DRM bridge chain, a connector with the panel mode, successful atomic mode validation, active clocks/supplies only while needed, and backlight activation after the display path.

## Step 8: Exercise The Pipeline

Test:

1. cold boot with firmware leaving every display component off
2. bootloader splash handoff if supported
3. modeset to the native and a lower-bandwidth mode
4. repeated blank/unblank and DPMS cycles
5. runtime suspend/resume while display is disabled
6. system suspend/resume with display active
7. bridge driver unbind/rebind if safely supported
8. failure rollback when a regulator or downstream component is missing

Monitor current draw, reset, reference clock, DSI lanes, eDP training, and backlight timing where accessible.

## Step 9: Diagnose Deliberate Faults

### Fault A: Missing Reverse Link

`bridge_dsi_in` points to `dsi_out`, but `dsi_out` lacks `remote-endpoint`. Compilation may succeed. Static reciprocity audit or schema/device checks should catch it; runtime traversal from the controller can fail to find the bridge.

### Fault B: Bridge Input Uses `port@1`

The phandles remain reciprocal, but the bridge binding defines port 1 as output. This is a structurally connected output-to-output topology. Restore binding-defined port direction rather than renaming labels.

### Fault C: Bridge Declares Two Data Lanes

The controller endpoint lists four while the bridge lists two. Determine whether this is a description mismatch or a real two-lane population. Recalculate lane bandwidth; do not make arrays match without checking the schematic.

### Fault D: Panel Driver Is Missing

The I²C bridge probes, but the DRM chain remains incomplete or deferred. Confirm the panel node's compatible and kernel configuration. Raising display-controller probe priority cannot create the missing downstream component.

### Fault E: Bridge Core Supply Is Missing

Graph topology is perfect, but bridge probe defers or fails. Trace `vdd-supply` to the provider and inspect regulator state. Graph edges do not imply power dependencies.

### Fault F: Native Mode Exceeds Link Capacity

The connector appears, but atomic modeset rejects 1080p while a lower mode works. Follow mode-validation logs and calculate limits at every stage. Do not force the native mode or overclock the link through an undocumented property.

## Exit Review

The lab is complete when you can provide:

- a classified containment/resource/graph relationship map
- reciprocal endpoint and port-direction audit
- DSI and eDP interface matrices
- bandwidth calculation with documented assumptions
- component bind and power-sequencing analysis
- DRM runtime evidence for the complete chain
- cold/warm boot and suspend/resume results
- root-cause evidence for each deliberate fault

## Authoritative References

- [Linux generic graph binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/graph.yaml)
- [Linux video interface binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/media/video-interfaces.yaml)
- [Linux DRM/KMS documentation](https://docs.kernel.org/gpu/drm-kms.html)
- [Linux DRM mode-setting helpers](https://docs.kernel.org/gpu/drm-kms-helpers.html)

## Continue

Proceed to [Memory, Firmware, And Heterogeneous SoCs](../memory-firmware-and-heterogeneous-socs.md).
