---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Camera Pipelines: Sensors, Receivers, ISPs, And Capture

Camera systems combine a control bus with a high-bandwidth media path. The sensor may be an I²C child for register access while its endpoint connects to a CSI-2 receiver, ISP, or capture engine elsewhere in the tree.

## Control And Data Are Separate

The AM62x SK camera overlay `k3-am62x-sk-csi2-ov5640.dtso` adds an OmniVision OV5640 behind a PCA9543 mux on `main_i2c2` (trimmed):

```dts
&main_i2c2 {
        i2c-mux@71 {
                compatible = "nxp,pca9543";
                reg = <0x71>;

                /* CAM port */
                i2c@1 {
                        reg = <1>;

                        ov5640: camera@3c {
                                compatible = "ovti,ov5640";
                                reg = <0x3c>;
                                clocks = <&clk_ov5640_fixed>;
                                clock-names = "xclk";
                                AVDD-supply = <&reg_2p8v>;
                                DOVDD-supply = <&reg_1p8v>;
                                DVDD-supply = <&reg_1p5v>;
                                powerdown-gpios = <&exp1 13 GPIO_ACTIVE_LOW>;
                                port {
                                        csi2_cam0: endpoint {
                                                remote-endpoint = <&csi2rx0_in_sensor>;
                                                clock-lanes = <0>;
                                                data-lanes = <1 2>;
                                        };
                                };
                        };
                };
        };
};
```

The I²C path (bus, mux channel 1, address `0x3c`) says how to configure the sensor. The endpoint says where image data travels. Everything else is a resource edge: the overlay defines `clk_ov5640_fixed` as a 12 MHz `fixed-clock` (an oscillator on the camera module, not the CSI-2 link frequency) and three always-on fixed regulators, and `powerdown-gpios` comes from the SK's `exp1`.

## Receiver And Downstream Processing

The receiver half lives in `k3-am62-main.dtsi` (trimmed):

```dts
ti_csi2rx0: ticsi2rx@30102000 {
        compatible = "ti,j721e-csi2rx-shim";
        dmas = <&main_bcdma 0 0x4700 0>;
        dma-names = "rx0";
        reg = <0x00 0x30102000 0x00 0x1000>;
        power-domains = <&k3_pds 182 TI_SCI_PD_EXCLUSIVE>;
        ranges;
        status = "disabled";

        cdns_csi2rx0: csi-bridge@30101000 {
                compatible = "ti,j721e-csi2rx", "cdns,csi2rx";
                phys = <&dphy0>;
                phy-names = "dphy";

                ports {
                        csi0_port0: port@0 {
                                reg = <0>;
                                status = "disabled";
                        };

                        csi0_port1: port@1 {
                                reg = <1>;
                                status = "disabled";
                        };
                        /* port@2 .. port@4 */
                };
        };
};
```

The overlay sets `port@0` to `okay`, adds the `csi2rx0_in_sensor` endpoint, and enables `ti_csi2rx0` and `dphy0`. Three details are worth noting:

- ports can carry `status`. The SoC file declares all five Cadence ports disabled, and the overlay enables only the one it wires.
- the Cadence bridge's stream outputs (`port@1`–`port@4`) are not linked to anything. The TI shim finds the bridge as its `csi-bridge` child node, not through the graph, and receives frames from it over BCDMA. Containment and graph each carry part of the topology.
- `cdns-csi2rx` reads only `port@0`/`endpoint@0`. It rejects any bus type other than CSI-2 D-PHY and any `data-lanes` count above what the hardware supports, then registers an async connection to the remote sensor.

Port and endpoint numbering are binding-defined. A `remote-endpoint` does not select a virtual-channel policy unless the binding includes such a property.

## Media Controller And V4L2 Subdevices

At runtime, sensors, receivers, lens controllers, flashes, ISPs, and scalers can register as V4L2 subdevices and media entities. Pads and links form the media graph. Capture video nodes appear only after the relevant bridge/receiver driver assembles the pipeline.

Asynchronous registration is expected because the I²C sensor and platform receiver can probe in either order. V4L2 async notifiers match remote firmware endpoints and complete the pipeline when every required subdevice is available.

No `/dev/video*` does not prove sensor failure. Determine whether the sensor bound, the async connection matched, the media graph completed, and a capture node is part of that driver architecture.

## Formats And Selection

Media-bus codes describe data on links between subdevices; userspace pixel formats describe memory buffers. A Bayer sensor may output a raw media-bus format that an ISP converts to an RGB/YUV memory format. Width, height, crop, compose, and routing can differ by pad.

Use media-controller APIs to inspect and configure links/formats where the driver exposes them. Hard-coding one format in DT is normally wrong unless the endpoint binding describes immutable bus wiring.

## Clocks, Rates, And Throughput

Derive required CSI-2 throughput from active pixels, blanking, bits per sample, frame rate, embedded data, and protocol overhead. Confirm the selected link frequency and lane count support it. Then validate receiver, ISP, DMA, memory bandwidth, and buffer allocation.

The sensor driver's `pixel_rate` control may derive from link frequency, lane count, and bits per pixel. Do not assume equality between those numbers.

## Power And Privacy

Sensor power-up often sequences rails, external clock, reset, and standby. Lens and flash devices can have independent supplies. Runtime PM should power the module only when required while preserving control-bus accessibility needed for identification.

Production systems also need camera privacy and access policy. A DT graph exposes hardware topology; it does not enforce which process may stream or whether a privacy LED is trustworthy.

## Runtime Diagnosis

```sh
media-ctl -p
v4l2-ctl --list-devices
v4l2-ctl --all -d /dev/video0
dmesg | grep -Ei 'camera|sensor|csi|isp|v4l2|media|async'
```

Trace sensor ID, async binding, pad links, negotiated formats, streaming start, CSI error counters, frame interrupts, DMA completion, and buffer delivery. A correct graph can still fail electrically through lane swap, polarity, or insufficient link rate.

## Authoritative References

- [Linux camera sensor driver guidance](https://docs.kernel.org/driver-api/media/camera-sensor.html)
- [Linux V4L2 async API](https://docs.kernel.org/driver-api/media/v4l2-async.html)
- [Linux V4L2 fwnode API](https://docs.kernel.org/driver-api/media/v4l2-fwnode.html)
- [Linux media-controller userspace API](https://docs.kernel.org/userspace-api/media/mediactl/media-controller.html)

## Continue

Proceed to [Audio Graphs, DAI Links, And Routing](audio-graphs-dai-links-and-routing.md).
