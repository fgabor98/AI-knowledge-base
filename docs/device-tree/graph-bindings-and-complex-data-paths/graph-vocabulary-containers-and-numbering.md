---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Graph Vocabulary, Containers, And Numbering

The generic graph binding represents devices as vertices, hardware interfaces as ports, and point-to-point connections as pairs of endpoints. Numbering is local to the containing device and must follow both generic and device-specific schemas.

## Vertex, Port, Endpoint, Edge

- A **vertex** is normally the device node containing graph ports.
- A **port** represents one hardware interface on that device.
- An **endpoint** represents one connection available through a port.
- An **edge** is formed by two endpoints referencing each other with `remote-endpoint`.

One port can have one endpoint for a simple point-to-point link. A port can have multiple endpoints only when the hardware and binding support multiple connections or routes.

## Direct `port` Versus `ports`

A device with one interface can place one `port` directly below itself. The AM62x SK HDMI connector, from `k3-am62x-sk-common.dtsi`:

```dts
hdmi0: connector-hdmi {
        compatible = "hdmi-connector";
        label = "hdmi";
        type = "a";
        port {
                hdmi_connector_in: endpoint {
                        remote-endpoint = <&sii9022_out>;
                };
        };
};
```

Multiple ports are wrapped in `ports`. The SiI9022 HDMI transmitter on the same board:

```dts
sii9022: bridge-hdmi@3b {
        compatible = "sil,sii9022";
        reg = <0x3b>;
        interrupt-parent = <&exp1>;
        interrupts = <16 IRQ_TYPE_EDGE_FALLING>;
        #sound-dai-cells = <0>;
        sil,i2s-data-lanes = < 0 >;

        ports {
                #address-cells = <1>;
                #size-cells = <0>;

                port@0 {
                        reg = <0>;

                        sii9022_in: endpoint {
                                remote-endpoint = <&dpi1_out>;
                        };
                };

                port@1 {
                        reg = <1>;

                        sii9022_out: endpoint {
                                remote-endpoint = <&hdmi_connector_in>;
                        };
                };
        };
};
```

The wrapper gives the ports their own `#address-cells`/`#size-cells`, separate from the device's other children. Here that matters because the bridge is an I²C target: without `ports`, `port@0` would inherit the I²C bus's cell counts.

## Numbering Domains

`port@1/reg = <1>` selects a binding-defined port number. Inside a multi-endpoint port, `endpoint@0/reg = <0>` selects an endpoint local to that port. Neither value is a memory address.

The Allwinner A64 display engine has two mixers and two timing controllers, and either mixer can feed either controller. From `arch/arm64/boot/dts/allwinner/sun50i-a64.dtsi`:

```dts
mixer0_out: port@1 {
        #address-cells = <1>;
        #size-cells = <0>;
        reg = <1>;

        mixer0_out_tcon0: endpoint@0 {
                reg = <0>;
                remote-endpoint = <&tcon0_in_mixer0>;
        };

        mixer0_out_tcon1: endpoint@1 {
                reg = <1>;
                remote-endpoint = <&tcon1_in_mixer0>;
        };
};
```

`tcon0_in: port@0` has the mirror pair, with `endpoint@0` from mixer 0 and `endpoint@1` from mixer 1. The `#address-cells` inside the port are for endpoint numbers. TI's AM625 DSS binding (`ti,am65x-dss.yaml`) uses the same pattern for a different reason: `port@0` may carry `endpoint@0` and `endpoint@1`, one per OLDI transmitter, for dual-link LVDS.

Do not add `@0` and `reg` mechanically to single ports and endpoints. Use them when required for disambiguation or by schema, and keep unit addresses consistent with `reg`.

## Port Direction Is Binding-Defined

The generic graph schema does not declare port 0 input and port 1 output. Each device binding does:

| Device | Binding | `port@0` | Other ports |
|---|---|---|---|
| SiI9022 | `sil,sii9022.yaml` | parallel RGB input | `port@1` HDMI output, `port@3` sound input |
| AM625 DSS | `ti,am65x-dss.yaml` | VP1 output (to OLDI) | `port@1` VP2 DPI output |
| Cadence CSI-2 RX | `cdns,csi2rx.yaml` | CSI-2 input | `port@1`–`port@4` stream outputs |

The same number means input on the bridge and output on the display controller. Labels such as `sii9022_in` aid readability but do not carry direction into the DTB. Driver behavior comes from node structure, `reg`, and the binding contract.

## Graph Is Not Containment

Keep each device at its real firmware/bus location and connect only endpoints ([Three Different Structures](../graph-bindings-and-complex-data-paths.md#three-different-structures)). On the AM62x SK the HDMI connector is a root node, the SiI9022 is an I²C child of `main_i2c1`, and the DSS sits on the SoC bus. Moving the connector under the bridge to make the source read like a pipeline would falsely claim containment.

## Review Algorithm

For each vertex:

1. identify the device binding
2. list every defined port number and direction
3. list endpoint numbers within each port
4. resolve every `remote-endpoint`
5. verify the reverse reference
6. check that no endpoint is reused by two physical links
7. compare graph topology with the schematic or IP integration diagram

## Failure Patterns

- `ports` inherits unrelated parent cell counts because its own counts are missing.
- Two `port@0` nodes collide after source includes merge.
- A label suggests output while the binding defines the port as input.
- A remote link points to a device node rather than an endpoint label.
- An I²C child is moved into `ports` and loses its control-bus address.
- A source overlay replaces a port and accidentally removes another endpoint.

## Authoritative References

- [Linux generic graph binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/graph.yaml)
- [Devicetree Specification: node names and unit addresses](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)
- [Linux Devicetree coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

## Continue

Proceed to [Endpoint Links And Interface Contracts](endpoint-links-and-interface-contracts.md).
