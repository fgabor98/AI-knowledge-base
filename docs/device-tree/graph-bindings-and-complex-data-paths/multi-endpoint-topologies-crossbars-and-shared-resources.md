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

```dts
port@0 {
        reg = <0>;
        #address-cells = <1>;
        #size-cells = <0>;

        input_a: endpoint@0 {
                reg = <0>;
                remote-endpoint = <&source_a>;
        };

        input_b: endpoint@1 {
                reg = <1>;
                remote-endpoint = <&source_b>;
        };
};
```

This can represent two selectable connections to one hardware port if the binding defines it. It does not imply the port can merge or stream both inputs concurrently. Endpoint cardinality, routing registers, and simultaneous-use constraints are device-specific.

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

Examples:

- two DSI hosts share one PLL and cannot use unrelated lane rates
- CSI receiver ports share four lanes that can be split 4+0 or 2+2
- HDMI and eDP outputs share one encoder
- audio links share an MCLK family and cannot mix 44.1/48 kHz rates

Create a resource-conflict matrix during review. Single-pipeline tests will not reveal these conflicts.

## Bridges With Multiple Functions

A multifunction bridge may expose video, audio, AUX/DDC, GPIO, and interrupt functions. Only the high-bandwidth data path belongs in graph endpoints. Control buses and sideband services use their own bindings.

For HDMI, audio can be logically associated with the display encoder without a separate physical endpoint graph. For camera modules, flash/lens control may use phandles or firmware references separate from the pixel path. Follow subsystem bindings instead of forcing every relationship into `remote-endpoint`.

## Connectors And Hotplug

Graph topology can end at a connector whose remote external device is unknown until runtime. Connector orientation, alternate-mode muxes, hotplug detection, and EDID/control paths may add graph and non-graph relationships.

Hot-unplug requires all users to stop streaming and release resources before the path disappears. A fixed DT edge to a connector does not guarantee a sink is present.

## Topology Variants And Source Layering

Product variants often populate a different panel, sensor, bridge, or connector. Board variant DTS files should enable one coherent path and disable/remove incompatible endpoints. Leaving two mutually exclusive panel endpoints enabled can create ambiguous discovery or permanent deferral.

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
