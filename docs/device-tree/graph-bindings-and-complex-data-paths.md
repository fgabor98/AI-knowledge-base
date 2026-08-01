---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Graph Bindings And Complex Data Paths

Ordinary phandles describe a consumer using a provider. Graph bindings describe peer interfaces connected by a data path. They are used when a chain, crossbar, or multi-port component cannot be represented accurately as one provider tuple—especially in display, camera, and audio systems.

## Learning Outcomes

After completing this module, you should be able to:

- distinguish device containment from port topology and endpoint connectivity
- construct `ports`, `port`, and `endpoint` nodes with correct cell counts and unit addresses
- prove that `remote-endpoint` links are reciprocal and represent one physical connection
- separate link-wide properties from local receiver/transmitter configuration
- decode parallel and serial video endpoint properties, including lanes and bus formats
- trace DRM/KMS pipelines through controllers, encoders, bridges, panels, and connectors
- trace V4L2/media pipelines through sensors, receivers, ISPs, and capture engines
- distinguish an audio hardware graph from ASoC DAI links and DAPM signal routing
- reason about probe ordering, asynchronous component binding, streaming, power, and teardown
- validate topology semantically rather than stopping at syntactically valid phandles
- debug a missing pipeline from the final DTB through subsystem topology and physical signals

## Prerequisites

Complete [Common Peripheral Nodes](common-peripheral-nodes.md). This module assumes you can place devices on their control buses, decode provider resources, and distinguish enumeration from fixed board wiring.

## Learning Path

1. [Graph Vocabulary, Containers, And Numbering](graph-bindings-and-complex-data-paths/graph-vocabulary-containers-and-numbering.md)
2. [Endpoint Links And Interface Contracts](graph-bindings-and-complex-data-paths/endpoint-links-and-interface-contracts.md)
3. [Display Pipelines: Controllers, Bridges, Panels, And Connectors](graph-bindings-and-complex-data-paths/display-pipelines-controllers-bridges-panels-and-connectors.md)
4. [Camera Pipelines: Sensors, Receivers, ISPs, And Capture](graph-bindings-and-complex-data-paths/camera-pipelines-sensors-receivers-isps-and-capture.md)
5. [Audio Graphs, DAI Links, And Routing](graph-bindings-and-complex-data-paths/audio-graphs-dai-links-and-routing.md)
6. [Multi-Endpoint Topologies, Crossbars, And Shared Resources](graph-bindings-and-complex-data-paths/multi-endpoint-topologies-crossbars-and-shared-resources.md)
7. [Lifecycle, Ownership, And Pipeline Power](graph-bindings-and-complex-data-paths/lifecycle-ownership-and-pipeline-power.md)
8. [Graph Validation And Runtime Diagnosis](graph-bindings-and-complex-data-paths/graph-validation-and-runtime-diagnosis.md)
9. [Complex Pipeline Integration Lab](graph-bindings-and-complex-data-paths/complex-pipeline-integration-lab.md)

## Three Different Structures

Do not collapse these models:

| Structure | Meaning | Example |
|---|---|---|
| DT parent/child | firmware containment or bus addressing | I²C bridge device under an I²C controller |
| provider/consumer | resource acquisition | bridge consumes clocks, supplies, GPIOs, PHYs |
| graph edge | peer data interface | display controller output connects to bridge input |

A bridge can be an I²C child for control, a regulator consumer for power, and a graph vertex in the pixel path simultaneously. Its graph neighbor is not necessarily its DT parent.

## Generic Shape

```dts
source {
        port {
                source_out: endpoint {
                        remote-endpoint = <&sink_in>;
                };
        };
};

sink {
        port {
                sink_in: endpoint {
                        remote-endpoint = <&source_out>;
                };
        };
};
```

The two endpoint nodes describe opposite views of one physical link. Their properties must agree where they describe shared wiring, while each side can also carry local interface configuration defined by its binding.

## Graphs Describe Hardware, Not Active Routes

DT describes possible fixed connectivity and hardware constraints. Drivers and subsystem policy decide which mutually exclusive route is active, which display mode or camera format is negotiated, and when a pipeline streams. A graph edge does not by itself power devices, allocate bandwidth, or prove that every component driver exists.

## Completion Check

You are ready for [Memory, Firmware, And Heterogeneous SoCs](memory-firmware-and-heterogeneous-socs.md) when you can:

- identify every vertex, port, endpoint, and reciprocal edge in a complex pipeline
- explain why graph connectivity does not follow DT parenthood
- validate lane, polarity, timing, format, clock, and bandwidth constraints end to end
- map DT graph nodes to DRM, media-controller/V4L2, or ASoC runtime objects
- explain how asynchronous probe and component binding affect pipeline creation
- derive safe enable, stream, stop, suspend, and teardown order across the chain
- diagnose a graph that passes schema but cannot transport valid data

## Authoritative References

- [Linux generic graph binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/graph.yaml)
- [Linux video interface binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/media/video-interfaces.yaml)
- [Linux DRM/KMS documentation](https://docs.kernel.org/gpu/drm-kms.html)
- [Linux V4L2 fwnode API](https://docs.kernel.org/driver-api/media/v4l2-fwnode.html)
- [Linux ASoC overview](https://docs.kernel.org/sound/soc/overview.html)

## Related Topics

- [Provider-Consumer Relationships](provider-consumer-relationships.md)
- [Common Peripheral Nodes](common-peripheral-nodes.md)
- [Writing And Validating Binding Schemas](writing-and-validating-binding-schemas.md)
