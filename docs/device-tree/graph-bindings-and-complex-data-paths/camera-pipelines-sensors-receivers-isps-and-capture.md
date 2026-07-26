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

```dts
&i2c2 {
        camera@36 {
                compatible = "vendor,image-sensor";
                reg = <0x36>;
                clocks = <&camera_clock>;
                avdd-supply = <&reg_cam_2v8>;
                dvdd-supply = <&reg_cam_1v2>;
                reset-gpios = <&gpio4 2 GPIO_ACTIVE_LOW>;

                port {
                        sensor_out: endpoint {
                                remote-endpoint = <&csi_in>;
                                clock-lanes = <0>;
                                data-lanes = <1 2>;
                                link-frequencies = /bits/ 64 <400000000>;
                        };
                };
        };
};
```

The I²C address says how to configure the sensor. The endpoint says where image data travels. Its external clock is not the CSI-2 link frequency or pixel rate.

## Receiver And Downstream Processing

```dts
&csi_receiver {
        ports {
                #address-cells = <1>;
                #size-cells = <0>;

                port@0 {
                        reg = <0>;
                        csi_in: endpoint {
                                remote-endpoint = <&sensor_out>;
                                clock-lanes = <0>;
                                data-lanes = <1 2>;
                        };
                };

                port@1 {
                        reg = <1>;
                        csi_out: endpoint {
                                remote-endpoint = <&isp_in>;
                        };
                };
        };
};
```

The receiver may expose multiple virtual channels, inputs, or source pads. Port and endpoint numbering are provider-defined. A remote-endpoint connection does not select a virtual-channel policy unless the binding includes such a property.

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
