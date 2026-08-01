---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Endpoint Links And Interface Contracts

`remote-endpoint` establishes connectivity; endpoint properties describe how that interface is wired. Both ends must form one coherent electrical and protocol contract.

## Reciprocal Links

```dts
tx_out: endpoint {
        remote-endpoint = <&rx_in>;
};

rx_in: endpoint {
        remote-endpoint = <&tx_out>;
};
```

Treat reciprocity as required even where a parser could follow one direction. It enables independent traversal from either component and allows validation to detect dangling or mismatched edges.

A phandle that resolves is not sufficient. Confirm that both endpoints exist in the final tree, point to each other, belong to compatible port directions, and are enabled through their ancestor devices.

## Shared Versus Local Properties

Some endpoint properties describe the physical link and must agree; others describe one side's hardware behavior. The video-interface binding explicitly allows some properties to differ when the hardware compensates—for example, a transmitter and receiver may use opposite signal polarity descriptions according to local inversion.

Review each property through its binding rather than applying a blanket “both endpoints must be identical” rule.

## Parallel Video Example

```dts
endpoint {
        remote-endpoint = <&receiver>;
        bus-type = <5>;
        bus-width = <8>;
        data-shift = <0>;
        hsync-active = <1>;
        vsync-active = <1>;
        pclk-sample = <1>;
};
```

The numeric `bus-type` comes from the generic video binding, but device schemas can constrain allowed types. `bus-width` and `data-shift` describe connected data lines. Polarity and sample-edge settings must match transmitter behavior, board inversions, and receiver capability.

Do not infer pixel format from bus width alone. An 8-bit bus might carry raw Bayer, YUV, RGB components over multiple cycles, or another device-defined encoding.

## CSI-2 And Lane Maps

```dts
sensor_out: endpoint {
        remote-endpoint = <&csi_in>;
        clock-lanes = <0>;
        data-lanes = <1 2>;
        link-frequencies = /bits/ 64 <400000000>;
};
```

Lane indices describe physical lane mapping according to the binding. Array order can represent logical lane order. Verify lane swaps and polarity support on both devices; PCB swaps are valid only if the receiver/transmitter can compensate and the binding describes them.

`link-frequencies` describes supported bus link frequencies, not pixel rate. Derive required throughput from format, resolution, blanking/overhead, frame rate, lane count, and protocol encoding. A sensor clock, pixel rate control, and CSI-2 link frequency are related but not interchangeable.

## Format Negotiation And Constraints

The graph states possible hardware connectivity. At runtime, subsystem drivers negotiate media-bus codes, dimensions, timings, colorspace, and link rates. A connected graph can still have no mutually supported format.

For each edge, create an interface matrix:

| Constraint | Source supports | Sink supports | Intersection |
|---|---|---|---|
| bus type | ... | ... | ... |
| lanes/width | ... | ... | ... |
| formats | ... | ... | ... |
| clock/rate | ... | ... | ... |
| polarity/timing | ... | ... | ... |

No intersection means the hardware cannot stream in that configuration, regardless of schema success.

## Bandwidth Is End-To-End

Validate the slowest stage: source generation, serial link, bridge conversion, memory interface, DMA, display/capture engine, and system interconnect. DT may contain clocks, OPPs, interconnect paths, or memory bandwidth constraints outside the endpoint nodes. Graph review must follow those provider edges too.

## Authoritative References

- [Linux video interface binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/media/video-interfaces.yaml)
- [Linux V4L2 fwnode API](https://docs.kernel.org/driver-api/media/v4l2-fwnode.html)
- [Linux media bus format definitions](https://github.com/torvalds/linux/blob/master/include/uapi/linux/media-bus-format.h)

## Continue

Proceed to [Display Pipelines: Controllers, Bridges, Panels, And Connectors](display-pipelines-controllers-bridges-panels-and-connectors.md).
