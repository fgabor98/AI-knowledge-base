---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Display Pipelines: Controllers, Bridges, Panels, And Connectors

A display path can cross an SoC display controller, encoder, one or more bridges, a connector or fixed panel, backlight, and hotplug/EDID channels. Graph edges describe the pixel path; resource and control relationships remain ordinary properties.

## A Linear Pipeline

```text
framebuffer -> plane -> CRTC -> encoder -> bridge -> panel/connector
```

DRM/KMS runtime objects do not map one-to-one to DT nodes. Planes and CRTCs may be internal to one display-controller node, while external bridges and panels are separate devices. Graph topology helps drivers assemble the fixed external chain.

## Controller To Bridge

```dts
&display_controller {
        ports {
                #address-cells = <1>;
                #size-cells = <0>;

                port@1 {
                        reg = <1>;

                        display_out: endpoint {
                                remote-endpoint = <&bridge_in>;
                        };
                };
        };
};

bridge@2c {
        compatible = "vendor,display-bridge";
        reg = <0x2c>;
        vdd-supply = <&reg_bridge>;

        ports {
                #address-cells = <1>;
                #size-cells = <0>;

                port@0 {
                        reg = <0>;
                        bridge_in: endpoint {
                                remote-endpoint = <&display_out>;
                        };
                };

                port@1 {
                        reg = <1>;
                        bridge_out: endpoint {
                                remote-endpoint = <&panel_in>;
                        };
                };
        };
};
```

The bridge remains on its I²C control bus in a real tree; the shortened example focuses on its graph. Its supplies, reset, reference clock, DDC bus, AUX channel, GPIOs, and interrupts are not implied by the endpoints.

## Panels And Connectors

A fixed panel node can carry supplies, enable/reset GPIOs, timing/mode data, physical dimensions, orientation, and a backlight phandle according to its panel schema. A connector represents a physical port and may reference DDC, hotplug, or other connector resources.

Do not represent a removable monitor as a fixed panel. HDMI/DisplayPort monitors are discovered through hotplug and EDID; DT describes the controller/bridge/connector wiring. Conversely, a fixed panel without usable EDID needs binding-defined timing or mode information.

## Bridges Are Ordered Components

DRM bridge chains are linear for a given encoder path. Each stage transforms signaling or protocol: parallel RGB to LVDS, DSI to eDP, HDMI level shifting, and so on. Validate:

- input and output bus formats
- clock and lane limits
- mode constraints at every bridge
- connector/panel capabilities
- enable/disable ordering and delays
- hotplug and EDID path ownership

A bridge driver can defer until its downstream panel/bridge registers. Permanent deferral often means a missing remote endpoint, disabled device, absent driver, or incomplete resource provider.

## Mode Selection And Bandwidth

EDID or panel timings provide candidate modes. DRM atomic checking filters them through CRTC, encoder, bridge, and connector limits. A mode listed by a panel can still fail because a bridge PLL, lane rate, pixel clock, memory bandwidth, or format conversion cannot support it.

Compute active pixels and blanking pixel clock, bits per pixel, protocol overhead, lane capacity, and memory fetch bandwidth. “Native resolution” is not sufficient evidence.

## Bootloader Handoff

Seamless display handoff is a cross-owner protocol. Linux must understand inherited clocks, domains, framebuffer memory, bridge/panel state, and routing. Resetting a bridge at probe destroys handoff; preserving unknown firmware state can leave unsafe ownership.

Test cold boot with display off, boot splash handoff, modeset, suspend/resume, connector hotplug, and driver unbind where supported. Document which stage may blank and when.

## Runtime Diagnosis

```sh
ls -l /sys/class/drm
cat /sys/class/drm/card*-*/status 2>/dev/null
cat /sys/kernel/debug/dri/0/state 2>/dev/null
modetest -c -p
dmesg | grep -Ei 'drm|bridge|panel|connector|edid|modeset'
```

Map connectors and encoders to the graph, then separate discovery, component binding, mode validation, atomic commit, power sequencing, and physical signaling.

## Authoritative References

- [Linux DRM/KMS documentation](https://docs.kernel.org/gpu/drm-kms.html)
- [Linux DRM mode-setting helpers](https://docs.kernel.org/gpu/drm-kms-helpers.html)
- [Linux panel common schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/display/panel/panel-common.yaml)
- [Linux HDMI connector schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/display/connector/hdmi-connector.yaml)

## Continue

Proceed to [Camera Pipelines: Sensors, Receivers, ISPs, And Capture](camera-pipelines-sensors-receivers-isps-and-capture.md).
