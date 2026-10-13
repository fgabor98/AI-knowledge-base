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

The AM62x SK HDMI path has three graph vertices. The full nodes are in [Graph Vocabulary](graph-vocabulary-containers-and-numbering.md#direct-port-versus-ports); the edges are:

```text
dss port@1 (VP2, DPI)  dpi1_out <-> sii9022_in          port@0 of bridge-hdmi@3b
bridge-hdmi@3b port@1  sii9022_out <-> hdmi_connector_in  connector-hdmi
```

The edges say nothing about how each vertex is controlled or powered. Those relationships are ordinary properties, and some are absent:

- the DSS has registers, a TI-SCI power domain, three clocks (`fck`, `vp1`, `vp2`), and pinctrl for the 24-bit DPI pins
- the SiI9022 is an I²C target at `0x3b` on `main_i2c1`, and its hotplug interrupt arrives on line 16 of the expander `exp1`
- the binding allows `reset-gpios`, `iovcc-supply`, and `cvcc12-supply`, but the SK sets none, so Linux assumes the bridge is powered and out of reset
- the connector has no `ddc-i2c-bus` and no `hpd-gpios`. The `sii902x` driver reads EDID through an I²C bypass mux that it registers on its own parent bus, and reports hotplug from its interrupt

Remove the `interrupts` property and the display still works, but `sii902x` switches the connector from `DRM_CONNECTOR_POLL_HPD` to periodic connect polling. The graph is unchanged; the resource edge changed the behavior.

## Panels And Connectors

A fixed panel node can carry supplies, enable/reset GPIOs, timing/mode data, physical dimensions, orientation, and a backlight phandle according to its panel schema. A connector represents a physical port and may reference DDC, hotplug, or other connector resources.

Do not represent a removable monitor as a fixed panel. HDMI and DisplayPort monitors are discovered through hotplug and EDID; DT describes the controller, bridge, and connector wiring, as the SK's `hdmi-connector` with `type = "a"` does. Conversely, a fixed panel without usable EDID needs binding-defined timing or mode information.

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
