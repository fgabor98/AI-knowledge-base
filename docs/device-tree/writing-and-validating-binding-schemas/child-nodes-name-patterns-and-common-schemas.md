---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Child Nodes, Name Patterns, And Common Schemas

Child nodes are object-valued properties in the schema data model. Their names, contents, required fields, and ownership all need constraints. A parent-level `additionalProperties: false` does not automatically close each nested object.

## Fixed-Name Child Nodes

Use `properties` when a child has one exact semantic name. From `display/ti/ti,am65x-dss.yaml`:

```yaml
properties:
  oldi-transmitters:
    description:
      Child node under the DSS, to describe all the OLDI transmitters connected
      to the DSS videoports.
    type: object
    additionalProperties: false

    properties:
      "#address-cells":
        const: 1

      "#size-cells":
        const: 0

    patternProperties:
      '^oldi@[0-1]$':
        $ref: ti,am625-oldi.yaml#
        description: OLDI transmitters connected to the DSS VPs
```

`oldi-transmitters` is a binding-defined container for real hardware, and it is closed with its own `additionalProperties: false`. Do not create configuration containers merely to group unrelated driver parameters.

## Unit-Addressed Or Repeated Children

Use `patternProperties` for names that follow a defined regex. The pattern above, `'^oldi@[0-1]$'`, allows exactly two children because the SoC has two OLDI transmitters. `remoteproc/ti,k3-r5f-rproc.yaml` uses `"^r5f@[a-f0-9]+$"` for the cores of an R5F cluster, and `mailbox/ti,omap-mailbox.yaml` uses `"^mbox-[a-z0-9-]+$"` for sub-mailboxes.

Anchor patterns with `^` and `$` unless substring matching is intentional, because JSON Schema regex matching is not implicitly anchored. `media/ti,j721e-csi2rx-shim.yaml` deliberately uses `"^csi-bridge@"` with no end anchor, so any unit address matches; the price is that `csi-bridge@zzz-anything` matches too.

## Pattern The Name, Constrain The Address

A regex constrains spelling; the child's `reg` constrains its encoded address or identifier. Both matter. `ti,am625-oldi.yaml` limits `reg` only to `maxItems: 1`, so the name pattern `'^oldi@[0-1]$'` is the only bound. Changing `oldi@1`'s `reg` to `<5>` in the AM625 SK DTB passes both `dt-validate` and `dtc`. A value constraint closes that gap:

```yaml
patternProperties:
  '^oldi@[0-1]$':
    $ref: ti,am625-oldi.yaml#
    properties:
      reg:
        maximum: 1
```

With this added to a scratch copy of the schema, the modified DTB reports `oldi-transmitters:oldi@1:reg:0:0: 5 is greater than the maximum of 1`, and the unmodified SK DTB still passes.

Do not try to prove with regex alone that the unit address equals `reg`. Keep the regex broad enough for canonical hexadecimal spelling and make the property constraint exact.

## Bound Child Count

When hardware has a maximum number of children, constrain it. A name pattern over a small fixed set, such as `oldi@[0-1]`, bounds the count by construction. For open-ended unit addresses, `minProperties`/`maxProperties` on a dedicated container can count children, but standard properties such as `#address-cells` also count as object properties. Account for them rather than treating `maxProperties` as the number of children.

The R5F cluster binding states its count in prose (“should define two R5F child nodes … For AM62x … only one”) rather than in schema keywords. Prose is not enforced. Decide whether a count belongs in the schema, and if it varies by SoC, express it in an `if`/`then` on the compatible.

## Local Definitions

Use `$defs` for a repeated structure within one binding. `ti,omap-mailbox.yaml` defines the three-cell descriptor once and the sub-mailbox node once (trimmed):

```yaml
$defs:
  omap-mbox-descriptor:
    $ref: /schemas/types.yaml#/definitions/uint32-array
    items:
      - description: mailbox fifo id (fifo_id)
      - description: irq identifier index (irq_id)
      - description: mailbox user id (usr_id)

  omap-sub-mailbox:
    type: object
    properties:
      ti,mbox-tx:
        $ref: "#/$defs/omap-mbox-descriptor"
      ti,mbox-rx:
        $ref: "#/$defs/omap-mbox-descriptor"
      ti,mbox-send-noirq:
        type: boolean
    required:
      - ti,mbox-tx
      - ti,mbox-rx

patternProperties:
  "^mbox-[a-z0-9-]+$":
    $ref: "#/$defs/omap-sub-mailbox"
```

The sub-mailbox definition has no `additionalProperties: false`, so it is open. Adding a misspelled boolean such as `ti,mbox-send-noirqq;` to the AM642 SK's `mbox-main-r5fss0-core0` passes `dt-validate` silently. A misspelled valued property such as `ti,mbox-rxx = <0 0 2>;` is reported only indirectly, as `is not of type 'object', 'integer', 'array', 'boolean', 'null'`, because no schema gives it a type. Close every `$defs` object you write.

Use an external common schema when multiple bindings share a genuine hardware-class contract. Do not extract a helper merely to reduce a few lines if its semantics have no independent meaning.

## Child With Its Own Compatible

Sometimes a child is an independently bound device whose own schema validates it. `ti,j721e-csi2rx-shim.yaml` hands its child entirely to the Cadence binding:

```yaml
patternProperties:
  "^csi-bridge@":
    type: object
    description: CSI2 bridge node.
    $ref: cdns,csi2rx.yaml#
```

The `$ref` makes the parent apply the child schema directly, so the child is validated even if its own `select` would not have matched. The weaker alternative, constraining only `compatible` with `additionalProperties: true` and trusting the child schema to be selected, leaves an unvalidated hole whenever that selection fails.

If the parent also owns integration-specific child properties, decide whether they belong in the child binding, a referenced common schema, or the parent. Two schemas must not silently give conflicting meanings to the same field.

## Standard Child Structures

Prefer standard schemas for established structures:

- graph `ports`, `port`, and `endpoint`
- GPIO, interrupt, clock, reset, and power controllers
- I2C/SPI bus children
- fixed partitions
- reserved-memory children
- connector and PHY graph nodes

Reference the owning schema and add only device-specific bounds. Copying standard graph or bus properties into a vendor schema creates incomplete local versions.

## Bus Children Versus Functional Children

A controller can both have registers of its own and enumerate child devices on a bus. In that case:

- the controller binding references the bus-controller schema
- child name patterns follow bus conventions
- child devices are validated by their own compatible schemas
- controller-specific child constraints remain narrow
- closure must allow the properties evaluated by the referenced bus schema

Do not validate arbitrary bus children by listing every possible child compatible in the controller binding.

## `properties` And `patternProperties` Can Both Apply

If a name matches both an exact property and a regex, both schemas apply. This can be useful, but overlapping patterns often cause accidental contradictions.

Audit each possible child name against every pattern. Prefer mutually clear patterns and place broad common behavior in a shared definition rather than stacking several near-duplicate regexes.

## Forbid Unknown Children

Close each nested object:

```yaml
additionalProperties: false
```

or, when composed with referenced schemas:

```yaml
unevaluatedProperties: false
```

Without nested closure, a typo such as `channe1@0` or an undocumented child property may pass even though the parent itself is closed.

## Child-Node Review Checklist

- Does every child represent hardware or an established standard structure?
- Is the name exact or regex-bound and anchored?
- Are unit address and `reg` semantics constrained by the correct bus model?
- Is the number of children physically bounded?
- Is every nested object closed at its own level?
- If another compatible schema owns a child, was that selection proven?
- Are standard graph, bus, partition, or provider schemas reused?
- Can two patterns match the same name and conflict?
- Are required child properties distinct from merely permitted properties?

## Authoritative References

- [Linux schema-writing guide: child nodes and pattern properties](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [`dt-schema` common schemas](https://github.com/devicetree-org/dt-schema/tree/main/dtschema/schemas)
- [Devicetree Specification: node names](https://devicetree-specification.readthedocs.io/en/stable/devicetree-basics.html)

## Continue

Proceed to [Examples, Vendor Bindings, And Authoring Workflow](examples-vendor-bindings-and-authoring-workflow.md).
