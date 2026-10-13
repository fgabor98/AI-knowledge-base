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

The AM62x SK links its display controller to the HDMI bridge. In `k3-am62x-sk-common.dtsi`, the DSS side is:

```dts
&dss_ports {
        /* VP2: DPI Output */
        port@1 {
                reg = <1>;

                dpi1_out: endpoint {
                        remote-endpoint = <&sii9022_in>;
                };
        };
};
```

and the bridge side, under `bridge-hdmi@3b`, is `sii9022_in: endpoint { remote-endpoint = <&dpi1_out>; };`.

Treat reciprocity as required even where a parser could follow one direction. It enables independent traversal from either component and allows validation to detect dangling or mismatched edges.

A phandle that resolves is not sufficient. Confirm that both endpoints exist in the final tree, point to each other, belong to compatible port directions, and are enabled through their ancestor devices.

## Shared Versus Local Properties

Some endpoint properties describe the physical link and must agree; others describe one side's hardware behavior. The video-interface binding explicitly allows some properties to differ when the hardware compensates—for example, a transmitter and receiver may use opposite signal polarity descriptions according to local inversion.

Review each property through its binding rather than applying a blanket “both endpoints must be identical” rule.

## Parallel Video Example

The AM437x Starter Kit (`arch/arm/boot/dts/ti/omap/am437x-sk-evm.dts`) connects an OmniVision OV2659 sensor to the VPFE capture block over an 8-bit parallel bus:

```dts
&i2c1 {
        ov2659@30 {
                compatible = "ovti,ov2659";
                reg = <0x30>;
                clocks = <&clkout1_mux_ck>;
                clock-names = "xvclk";

                port {
                        ov2659_1: endpoint {
                                remote-endpoint = <&vpfe0_ep>;
                                link-frequencies = /bits/ 64 <70000000>;
                        };
                };
        };
};

&vpfe0 {
        /* Camera port */
        port {
                vpfe0_ep: endpoint {
                        remote-endpoint = <&ov2659_1>;
                        ti,am437x-vpfe-interface = <0>;
                        bus-width = <8>;
                        hsync-active = <0>;
                        vsync-active = <0>;
                };
        };
};
```

The two ends carry different properties because each binding asks for what its driver must configure. The receiver states bus width and sync polarities (active low) and selects its interface mode through a vendor property. The sensor states only the link frequency it should run at. No `bus-type` is given; the parallel bus is implied by the receiver binding. Polarities must match the transmitter's behavior and any board inversion.

Do not infer pixel format from bus width alone. An 8-bit bus might carry raw Bayer, YUV, or RGB components over multiple cycles.

## CSI-2 And Lane Maps

From the AM62x SK OV5640 overlay (`k3-am62x-sk-csi2-ov5640.dtso`), sensor side:

```dts
port {
        csi2_cam0: endpoint {
                remote-endpoint = <&csi2rx0_in_sensor>;
                clock-lanes = <0>;
                data-lanes = <1 2>;
        };
};
```

and receiver side, on the Cadence CSI-2 bridge:

```dts
csi2rx0_in_sensor: endpoint {
        remote-endpoint = <&csi2_cam0>;
        bus-type = <4>; /* CSI2 DPHY. */
        clock-lanes = <0>;
        data-lanes = <1 2>;
};
```

Here both ends agree on one clock lane and two data lanes; `bus-type = <4>` (MIPI CSI-2 D-PHY in `video-interfaces.yaml`) appears only on the receiver. Lane indices describe physical lane mapping. Verify lane swaps and polarity support on both devices; a PCB lane swap is valid only if one side can compensate and its binding describes it.

The OV5640 endpoint has no `link-frequencies`, while the J721E SK's IMX219 overlay sets `link-frequencies = /bits/ 64 <456000000>` because `sony,imx219.yaml` requires it and the driver checks the value against the rates it supports. `link-frequencies` lists allowed bus link frequencies, not the pixel rate. Derive required throughput from format, resolution, blanking, frame rate, lane count, and protocol encoding.

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
