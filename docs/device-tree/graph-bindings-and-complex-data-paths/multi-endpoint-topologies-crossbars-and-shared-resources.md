---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Multi-Endpoint Topologies, Crossbars, And Shared Resources

Real media hardware branches, selects, aggregates, and shares resources. A graph can describe possible physical paths, but the device binding and subsystem determine which combinations can operate simultaneously.

## Multiple Endpoints Are Not Automatic Fan-Out

The Allwinner A64 timing controller TCON0 accepts pixels from either display mixer. From `sun50i-a64.dtsi`:

```dts
tcon0_in: port@0 {
        #address-cells = <1>;
        #size-cells = <0>;
        reg = <0>;

        tcon0_in_mixer0: endpoint@0 {
                reg = <0>;
                remote-endpoint = <&mixer0_out_tcon0>;
        };

        tcon0_in_mixer1: endpoint@1 {
                reg = <1>;
                remote-endpoint = <&mixer1_out_tcon0>;
        };
};
```

These are two selectable connections to one hardware port; together with the matching pairs on TCON1 and both mixers, they form a 2×2 crossbar. It does not imply the port can merge or stream both inputs concurrently. Endpoint cardinality, routing registers, and simultaneous-use constraints are device-specific.

Likewise, two output endpoints do not prove the hardware duplicates a stream. The component may contain a mux, crossbar, splitter, or independently driven outputs. Name the behavior in the binding, not by graph shape alone.

## Crossbars And Routes

An SoC display or media block can connect several inputs to several outputs. DT describes hardwired availability; runtime atomic state or media routing selects a legal route. Review:

- which input/output pairs are physically possible
- which pairs share an internal resource
- whether routes can change while streaming
- whether formats and clocks must match across grouped routes
- whether one source can feed multiple sinks
- whether routing state survives suspend or reset

Do not omit a real connection merely because only one product use case enables it. Do not add every logical IP possibility when package pins or board wiring remove paths.

## Shared PLLs, PHYs, And Lanes

Two graph paths can share a PLL, D-PHY, serializer, DMA engine, or memory port. The graph alone does not express all arbitration. Ordinary `clocks`, `phys`, power domains, interconnects, and resets reveal shared suppliers; driver state enforces exclusivity and rate compatibility.

Examples from real SoC files:

- both AM625 OLDI transmitters take `clocks = <&k3_clks 186 0>` as their serial clock, so dual-link and independent single-link configurations must agree on one rate
- the AM625 DSS's `vp1` clock is `dss_vp1_clk`, so VP1's pixel clock depends on the OLDI path, while VP2 (DPI) uses its own TI-SCI clock
- the AM62x SK's camera powerdown line, HDMI interrupt, and SD supply enable all come from the same I²C expander, so a stuck `main_i2c1` takes out three unrelated pipelines
- audio links that share an MCLK family cannot mix 44.1 kHz and 48 kHz rates

Create a resource-conflict matrix during review. Single-pipeline tests will not reveal these conflicts.

## Bridges With Multiple Functions

A multifunction bridge may expose video, audio, AUX/DDC, GPIO, and interrupt functions. Only the high-bandwidth data path belongs in graph endpoints. Control buses and sideband services use their own bindings.

The SiI9022 on the AM62x SK is an example: one I²C node with video ports, `#sound-dai-cells` for audio, an interrupt for hotplug, and an internal DDC bypass. For HDMI, audio can be logically associated with the display encoder without a separate physical endpoint graph. For camera modules, flash/lens control may use phandles or firmware references separate from the pixel path. Follow subsystem bindings instead of forcing every relationship into `remote-endpoint`.

## Connectors And Hotplug

Graph topology can end at a connector whose remote external device is unknown until runtime. Connector orientation, alternate-mode muxes, hotplug detection, and EDID/control paths may add graph and non-graph relationships.

Hot-unplug requires all users to stop streaming and release resources before the path disappears. A fixed DT edge to a connector does not guarantee a sink is present.

## Topology Variants And Source Layering

Product variants often populate a different panel, sensor, bridge, or connector. Mainline ships three camera overlays for the AM62x SK, `k3-am62x-sk-csi2-ov5640.dtso`, `-tevi-ov5640.dtso`, and `-imx219.dtso`. Each one writes the `csi2rx0_in_sensor` endpoint on the same Cadence `port@0`, so they are mutually exclusive: applying two would leave the receiver pointing at the second sensor and the first sensor's endpoint dangling. The kernel `Makefile` composes each one separately with `k3-am625-sk.dtb`. Board variant files should enable one coherent path and disable or remove incompatible endpoints. Leaving two mutually exclusive panel endpoints enabled can create ambiguous discovery or permanent deferral.

Review the final DTB for each shipping variant. Include-order reasoning is insufficient when labels and amendments replace ports across multiple files.

## Senior Review Checklist

1. Which graph edges can be active simultaneously?
2. Which clocks, PHYs, lanes, DMA engines, and domains are shared?
3. Who arbitrates each shared resource?
4. Are endpoint counts and routes supported by the binding and driver?
5. What happens during dynamic route change or hot-unplug?
6. Does every product variant leave exactly its populated topology enabled?

## Authoritative References

- [Linux generic graph binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/graph.yaml)
- [Linux DRM atomic modesetting](https://docs.kernel.org/gpu/drm-kms.html#atomic-mode-setting)
- [Linux media-controller userspace API](https://docs.kernel.org/userspace-api/media/mediactl/media-controller.html)
- [Linux Dynamic PCM documentation](https://docs.kernel.org/sound/soc/dpcm.html)

## Continue

Proceed to [Lifecycle, Ownership, And Pipeline Power](lifecycle-ownership-and-pipeline-power.md).
