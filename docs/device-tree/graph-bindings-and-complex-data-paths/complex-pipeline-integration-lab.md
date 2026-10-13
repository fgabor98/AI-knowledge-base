---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Complex Pipeline Integration Lab

This lab reviews three real media pipelines on the TI AM62x SK (`k3-am625-sk.dts`): HDMI display through a SiI9022 bridge, analog audio through a TLV320AIC3106 codec, and CSI-2 capture from an OV5640 camera added by an overlay. You will inventory every graph edge, separate graph edges from resource edges, check interface contracts, and diagnose five deliberate faults. Steps 1–4 and the fault predictions need only the DTBs; Steps 5 and 6 need the board and camera module.

## Inputs

Build the base DTB and the camera overlay, then merge them. With Kbuild, `make ARCH=arm64 ti/k3-am625-sk-csi2-ov5640.dtb` produces the composed DTB defined in `arch/arm64/boot/dts/ti/Makefile`. By hand, run both through `cpp` as in the [First Device Tree Lab](../foundations/first-device-tree-lab.md#step-10-build-the-real-board-dtb), then:

```sh
dtc -@ -I dts -O dtb -o k3-am625-sk.dtb k3-am625-sk.pp.dts
dtc -@ -I dts -O dtb -o ov5640.dtbo k3-am62x-sk-csi2-ov5640.pp.dts
fdtoverlay -i k3-am625-sk.dtb -o sk-ov5640.dtb ov5640.dtbo
M=sk-ov5640.dtb
```

Sources: `k3-am62x-sk-common.dtsi`, `k3-am62-main.dtsi`, `k3-am62x-sk-csi2-ov5640.dtso`, `k3-am62x-sk-csi2-imx219.dtso`.

## Step 1: Capture The Artifacts

Record the kernel revision, `dtc` version, hashes of all three artifacts, and every warning. Compiling the overlay alone prints:

```text
Warning (graph_child_address): /fragment@2/__overlay__/ports: graph node has single child node 'port@0', #address-cells/#size-cells are not necessary
```

Explain why this is a false positive: in the merged tree, `ports` has five `port@N` children from the SoC file. Checks on an isolated overlay see only the fragment.

## Step 2: Inventory The Graph

Six endpoints form three edges:

```sh
for ep in \
    /bus@f0000/dss@30200000/ports/port@1/endpoint \
    /bus@f0000/i2c@20010000/bridge-hdmi@3b/ports/port@0/endpoint \
    /bus@f0000/i2c@20010000/bridge-hdmi@3b/ports/port@1/endpoint \
    /connector-hdmi/port/endpoint \
    /bus@f0000/i2c@20020000/i2c-mux@71/i2c@1/camera@3c/port/endpoint \
    /bus@f0000/ticsi2rx@30102000/csi-bridge@30101000/ports/port@0/endpoint
do
        echo "$ep phandle=$(fdtget -tx $M $ep phandle) remote=$(fdtget -tx $M $ep remote-endpoint)"
done
```

Each `remote` value must equal the partner's `phandle`, and vice versa. Then draw the topology with the binding-defined direction of each port (see [Port Direction Is Binding-Defined](graph-vocabulary-containers-and-numbering.md#port-direction-is-binding-defined)):

```text
DSS port@1 (VP2 DPI out) -> SiI9022 port@0 (RGB in) -> SiI9022 port@1 (HDMI out) -> hdmi-connector
OV5640 port (CSI-2 out)  -> Cadence CSI2RX port@0 (CSI-2 in) ~~ child of ~~> TI CSI2RX shim -> BCDMA
McASP1 <- sound-dai (simple-audio-card, no graph) -> TLV320AIC3106
```

The camera path ends in containment, not an edge: the shim finds the Cadence bridge as its `csi-bridge` child. The audio card uses phandles instead of a graph.

## Step 3: List The Resource Edges

For each vertex, record what the graph does not show:

| Vertex | Control bus | Clocks | Power | GPIO/IRQ |
|---|---|---|---|---|
| DSS | MMIO | `k3_clks 186 6`, `dss_vp1_clk`, `k3_clks 186 2` | `k3_pds 186` | pinctrl for DPI |
| SiI9022 | `main_i2c1` @ `0x3b` | none in DT | none in DT | IRQ on `exp1` line 16 |
| HDMI connector | none | none | none | none; EDID and HPD go through the bridge |
| OV5640 | `main_i2c2` → mux `0x71` ch 1 → `0x3c` | 12 MHz `fixed-clock` | three fixed regulators | `powerdown-gpios` on `exp1` line 13 |
| CSI2RX shim/bridge | MMIO | `k3_clks 182 *` | `k3_pds 182`; D-PHY `dphy0` | — |
| TLV320AIC3106 | `main_i2c1` @ `0x1b` | 12.288 MHz MCLK | three supplies | — |

Count the dependencies on `exp1` and `main_i2c1`. A failure there removes display hotplug, the camera, and the codec at once.

## Step 4: Check The Interface Contracts

1. DPI: `sii902x` reads `bus-width` from its `port@0` endpoint and defaults to 24. Neither endpoint sets it. Confirm that the DSS pinctrl group carries 24 data lines plus clock and syncs.
2. CSI-2: both endpoints declare clock lane 0 and data lanes 1–2. Compute whether two lanes can carry the OV5640 mode you plan to use.
3. Audio: write down the DAI contract from [Audio Graphs](audio-graphs-dai-links-and-routing.md#dai-link-contract): format, master, inversion, slots, MCLK.

## Step 5: Collect Runtime Evidence

On the SK with the overlay applied:

```sh
modetest -M tidss -c
cat /sys/class/drm/card*-HDMI-A-*/status
media-ctl -p
v4l2-ctl --list-devices
aplay -l
cat /sys/kernel/debug/devices_deferred
dmesg | grep -Ei 'tidss|sii902x|ov5640|csi2rx|tlv320|asoc|deferred'
```

Map each runtime object back to a vertex: the HDMI connector to `connector-hdmi`, media entities to `camera@3c` and `csi-bridge@30101000`, and the ALSA card `AM62x-SKEVM` to the `sound` node.

## Step 6: Exercise The Lifecycle

Test cold boot with HDMI unplugged, then hotplug; repeated mode changes; capture start/stop; concurrent playback, capture, and display; and suspend/resume with each pipeline active. Note which failures show up only when pipelines run together.

## Step 7: Diagnose Deliberate Faults

Apply each fault to a copy of `$M`, run `dtc -I dtb -O dts <copy> >/dev/null`, and record the warnings. The phandles below come from a mainline build (DSS endpoint `0x1a`); confirm yours from Step 2.

### Fault A: A One-Sided Edge

```sh
cp $M a.dtb
fdtput -t x a.dtb /connector-hdmi/port/endpoint remote-endpoint 1a
```

`dtc` reports two `graph connection ... is not bidirectional` warnings. Predict the runtime effect: probably none. `sii902x` finds its next bridge by following its own `port@1` endpoint, which still points at the connector. Explain why an error that drivers tolerate is still a defect, and which future change (a new driver walking the graph from the connector) would expose it.

### Fault B: The Connector Is Disabled

```sh
cp $M b.dtb
fdtput -t s b.dtb /connector-hdmi status disabled
```

`dtc` is silent. Predict the chain: `sii902x` logs `port@1 remote device is disabled` and fails with `-ENODEV`. `tidss` then finds an available bridge node with no registered bridge, `drm_of_find_panel_or_bridge()` returns `-EPROBE_DEFER`, and the whole display controller stays deferred. Disabling the leaf of the graph removes the display entirely.

### Fault C: The Bridge Loses Its Interrupt

```sh
cp $M c.dtb
fdtput -d c.dtb /bus@f0000/i2c@20010000/bridge-hdmi@3b interrupts
```

Predict that display still works, but `sii902x` switches the connector from `DRM_CONNECTOR_POLL_HPD` to periodic connect polling and drops `DRM_BRIDGE_OP_HPD`. Measure the hotplug latency before and after.

### Fault D: The Receiver Uses One Lane

```sh
cp $M d.dtb
fdtput -t u d.dtb /bus@f0000/ticsi2rx@30102000/csi-bridge@30101000/ports/port@0/endpoint data-lanes 1
```

`dtc` is silent and every driver probes: `cdns-csi2rx` accepts one lane, and the sensor still has two. Predict that the media graph looks complete but streaming fails or produces corrupt frames. Explain why lane counts must be compared across the edge, and where in Step 4 you would have caught it.

### Fault E: Two Camera Overlays

```sh
fdtoverlay -i k3-am625-sk.dtb -o e.dtb ov5640.dtbo imx219.dtbo
```

`fdtoverlay` succeeds. Both sensors now sit on mux channel 1 (`camera@3c` and `camera@10`). The second overlay rewrote the receiver endpoint, so it points at the IMX219, and its new `phandle` leaves the OV5640's `remote-endpoint` dangling. `dtc` reports `graph phandle is not valid`. Explain why the overlay tool cannot reject this and where a product manifest should.

## Exit Review

The lab is complete when you can present:

- artifact hashes and an explanation of every build warning
- the six-endpoint reciprocity table and the annotated topology
- the resource-edge table with shared suppliers highlighted
- the DPI, CSI-2, and audio interface contracts
- runtime evidence mapping DRM, media, and ALSA objects to DT vertices
- predicted and observed outcomes for each fault, including which ones `dtc` reported

## Authoritative References

- [Linux generic graph binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/graph.yaml)
- [Linux video interface binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/media/video-interfaces.yaml)
- [Linux DRM/KMS documentation](https://docs.kernel.org/gpu/drm-kms.html)
- [Linux DRM mode-setting helpers](https://docs.kernel.org/gpu/drm-kms-helpers.html)

## Continue

Proceed to [Memory, Firmware, And Heterogeneous SoCs](../memory-firmware-and-heterogeneous-socs.md).
