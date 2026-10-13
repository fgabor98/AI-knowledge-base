---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Examples, Vendor Bindings, And Authoring Workflow

Binding examples are compiled DTS fragments and then validated. They are executable contract examples, not illustrative pseudocode. Good examples are minimal enough to isolate the binding and complete enough to exercise meaningful constraints.

## Start From An Accepted Neighbor

Before creating a file:

1. locate current schemas in the same subsystem
2. identify the relevant common or bus schema
3. inspect filename, title, property order, and example style
4. search for the proposed compatible and properties globally
5. confirm the vendor prefix already exists or plan its registration

Do not clone a superficially similar schema without re-deriving every field. Copied `select`, closure, conditionals, or resource names are common sources of inactive or incorrect validation.

## Vendor Prefixes

Vendor-specific compatibles and properties use a registered prefix:

```text
ti,am437x-vpfe
ti,am437x-vpfe-interface
```

The prefix identifies the organization responsible for the namespace, not necessarily the chip's distributor or board vendor. Reuse the existing registered prefix where applicable. Add a prefix entry as its own logical change when introducing a genuinely new vendor.

A prefix does not justify a custom concept. Search for standard properties and common schemas first.

## Example Scope

An example should normally include only the node being bound:

```yaml
examples:
  - |
    #include <dt-bindings/interrupt-controller/arm-gic.h>

    vpfe@0 {
        compatible = "ti,am437x-vpfe";
        reg = <0x0 0x2000>;
        interrupts = <GIC_SPI 48 IRQ_TYPE_LEVEL_HIGH>;

        port {
            endpoint {
                remote-endpoint = <&ov2659_ep>;
                ti,am437x-vpfe-interface = <0>;
                bus-width = <8>;
                hsync-active = <0>;
                vsync-active = <0>;
            };
        };
    };
```

`dt-extract-example` wraps this in `/dts-v1/; /plugin/;` so the unresolved `&ov2659_ep` becomes an overlay fixup instead of a compile error. The sensor is not part of the example.

The example build supplies surrounding scaffolding and dummy providers for ordinary references. Avoid unrelated consumer nodes, complete SoC trees, aliases, and board policy unless parent-bus context is necessary to express the binding.

## Address And Size Cells In Examples

Examples have default address/size-cell assumptions. If the node's `reg` encoding depends on a different parent bus, show the smallest appropriate parent:

The PowerVR binding's example shows the default case: it writes `reg = <0x0fd00000 0x20000>;` with one address and one size cell, although the real AM62x node uses two of each. A two-cell parent would be needed only if the binding itself depended on 64-bit addressing:

```yaml
examples:
  - |
    bus {
        #address-cells = <2>;
        #size-cells = <2>;

        gpu@fd00000 {
            compatible = "ti,am62-gpu", "img,img-axe-1-16m", "img,img-axe",
                         "img,img-rogue";
            reg = <0x0 0x0fd00000 0x0 0x20000>;
            /* ... */
        };
    };
```

Only add that wrapper when it demonstrates real required encoding. An unnecessary wrapper adds noise and may itself attract unrelated schema diagnostics.

## Includes Must Be Explicit

If an example uses symbolic constants, include the header in the DTS fragment:

The VPFE example above uses `GIC_SPI` and `IRQ_TYPE_LEVEL_HIGH`, so it includes `<dt-bindings/interrupt-controller/arm-gic.h>`. Without the `#include`, the example fails at the preprocessing and compile step, before any schema rule runs.

Do not assume includes from a real board DTS are present in the isolated example.

## Example Indentation

YAML convention uses two spaces for schema structure; DTS inside the literal block conventionally uses four spaces. Tabs inside the literal block can still violate YAML parsing.

```yaml
examples:
  - |
    vpfe@0 {
        compatible = "ti,am437x-vpfe";
        reg = <0x0 0x2000>;
    };
```

Choose `|` or another YAML block style intentionally. Whitespace and folding rules affect the text passed to the DTS compiler.

## Cover Meaningful Branches

One example is enough when variants share the same structure and constraints are straightforward. Add examples when they teach materially different valid forms:

- a fallback compatible sequence
- a variant with extra mandatory resources
- a child-node topology
- mutually exclusive operating modes
- a standard graph or bus composition

Examples do not replace negative tests. They mainly prove selected valid paths and provide canonical producer guidance.

## Keep Examples Synchronized

After every review change:

- update resource ordering and names together
- update required properties
- remove deprecated properties
- compile the example again
- compare it with real DTS users

A schema can permit one form while its example continues teaching an obsolete one.

## Authoring In Small Steps

A productive sequence is:

1. write the plain-language property contract
2. add preamble, identity, title, maintainers, and description
3. add `compatible`, `reg`, and required baseline resources
4. close the schema
5. add and compile one minimal example
6. reference common schemas
7. add variant conditions one at a time
8. validate one real DTB after each meaningful step
9. add negative mutations for boundary rules
10. run broader checks before submission

This keeps the first error close to the change that introduced it.

## Example Versus Real DTS

An example answers “can this isolated canonical node compile and satisfy the schema?” A real DTB answers “does this binding compose with actual buses, providers, inherited properties, and board data?”

Both are required. Examples can miss:

- properties inherited through DTS includes
- real address-cell widths
- competing schemas selecting the same node
- undocumented bootloader-added properties
- variant and overlay combinations

## Review Artifacts

Preserve or report:

```text
target schema path
vendor-prefix decision
common schemas referenced
example variants covered
real DTBs validated
negative cases exercised
tool versions/environment
remaining unrelated baseline warnings
```

Do not hide pre-existing warnings, but distinguish them precisely from regressions introduced by the patch.

## Authoritative References

- [Linux schema-writing guide: examples and coding style](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux annotated example schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/example-schema.yaml)
- [`dt-schema` vendor prefix registry](https://github.com/devicetree-org/dt-schema/blob/main/dtschema/schemas/vendor-prefixes.yaml)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

## Continue

Proceed to [Validation Toolchain And Targeted Checks](validation-toolchain-and-targeted-checks.md).
