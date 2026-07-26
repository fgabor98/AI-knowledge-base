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

A simple device can place one `port` directly below itself:

```dts
bridge@2c {
        compatible = "vendor,video-bridge";
        reg = <0x2c>;

        port {
                bridge_out: endpoint {
                        remote-endpoint = <&panel_in>;
                };
        };
};
```

Multiple ports are commonly wrapped in `ports`:

```dts
ports {
        #address-cells = <1>;
        #size-cells = <0>;

        port@0 {
                reg = <0>;
        };

        port@1 {
                reg = <1>;
        };
};
```

The wrapper isolates graph address/size-cell rules from other device children. This matters when the same device also contains addressable blocks or child buses.

## Numbering Domains

`port@1/reg = <1>` selects a provider-defined port number. Inside a multi-endpoint port, `endpoint@0/reg = <0>` selects an endpoint local to that port. Neither value is a memory address unless the device binding explicitly says so.

```dts
port@1 {
        reg = <1>;
        #address-cells = <1>;
        #size-cells = <0>;

        output_a: endpoint@0 {
                reg = <0>;
        };

        output_b: endpoint@1 {
                reg = <1>;
        };
};
```

Do not add `@0` and `reg` mechanically to single ports/endpoints. Use them when required for disambiguation or by schema, and keep unit addresses consistent with `reg`.

## Port Direction Is Binding-Defined

The generic graph schema does not universally declare port 0 input and port 1 output. The device binding defines port meaning and direction. For a display bridge, port 0 may be input and port 1 output; a different IP block can use another numbering scheme.

Labels such as `bridge_in` aid source readability but do not carry direction semantics into the DTB. Driver behavior comes from node structure, `reg`, and binding contract.

## Graph Is Not Containment

A panel can be a root-level platform device while its endpoint connects to an I²C-controlled bridge. Moving the panel under the bridge to make the source look like a pipeline would falsely claim containment. Keep each device at its real firmware/bus location and connect only endpoints.

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
