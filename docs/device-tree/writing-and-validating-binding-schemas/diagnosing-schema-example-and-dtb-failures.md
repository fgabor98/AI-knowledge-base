---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Diagnosing Schema, Example, And DTB Failures

Validation output often contains both the instance path and schema path, followed by a cascade of secondary errors. Debugging becomes predictable when you identify the earliest failing pipeline stage and reduce the instance/schema pair before editing either side.

## Classify The Failure First

| Stage | Typical symptom | First place to inspect |
|---|---|---|
| YAML parse | indentation, duplicate key, malformed scalar | exact source line and whitespace |
| meta-schema | disallowed keyword, missing description/type, bad top-level structure | binding path reported by `dt_binding_check` |
| reference resolution | cannot resolve `$ref`, duplicate `$id` | referenced URI and current `$id` base |
| example preprocessing | missing include or macro | example headers and DTS fragment |
| `dtc` | syntax, unit address, cells, phandle | compiled example/platform source |
| schema selection | expected binding produces no error | compatible/name selector and processed schema |
| DTB validation | required, type, cardinality, closure, branch failure | instance path plus schema path |

Do not change DTS to fix a meta-schema error, and do not weaken a schema to hide malformed DTS.

## Read Both Paths

A diagnostic conceptually says:

```text
/soc/capture@48000000: interrupt-names: ['completion'] is too short
from schema: .../media/acme,ax-capture.yaml
```

Extract:

- instance: exact node/property in the DTB
- value: normalized data that failed
- rule: keyword such as `required`, `maxItems`, `enum`, or closure
- schema: exact binding and branch owning the rule

When several schemas select the same node, the filename is essential. The error may come from a bus/common schema rather than the device binding you just edited.

## YAML And Meta-Schema Failures

Common causes:

- tabs or wrong indentation
- duplicate mapping keys
- an unquoted scalar interpreted as boolean or number
- `$id` not matching the required URI structure
- vendor property lacks a type reference or description
- top-level closure missing
- unsupported JSON Schema keyword
- `required` names a property never declared

Fix the schema document until targeted `dt_binding_check` passes. Do not proceed to platform interpretation while the binding is skipped.

## Reference And Composition Failures

For an unresolved reference:

1. resolve relative URI against current `$id`
2. confirm target filename and fragment
3. confirm target schema participates in the build set
4. check case and path spelling
5. look for copied IDs or moved files

For unexpected closure errors, create an evaluation table:

```text
property        local schema   referenced common schema   evaluated?
compatible      yes            core                       yes
reg             yes            core                       yes
ports           no             graph schema               only if referenced
vendor,foo      yes            no                         yes
```

If a legitimate property is unevaluated, add the missing owning schema or local definition. Do not open the entire object without understanding ownership.

## `oneOf` Diagnostics

`oneOf` often emits a summary plus errors from every branch. Find why the intended branch failed and whether another branch also matched.

Typical causes:

- branches overlap because discriminating properties are not required
- compatible lists are encoded in the wrong order
- a branch accepts too broad a type
- common top-level constraints contradict a branch
- the instance matches zero branches due to one missing resource

Reduce the instance to the discriminator and add fields back. Consider replacing complex `oneOf` structures with a broad top-level definition plus compatible-specific `if`/`then` narrowing.

## Required Versus Undocumented

These errors mean different things:

```text
'resets' is a required property
```

The schema selected and the applicable branch requires a missing hardware relationship. Check DTS completeness or whether the compatible/condition is wrong.

```text
'acme,foo' does not match any of the regexes / additional property not allowed
```

The node contains data no active schema owns. It could be:

- a DTS typo
- an undocumented downstream property
- a legitimate property omitted from this binding
- a property owned by a common schema that was not referenced
- the result of using `additionalProperties: false` where composed closure needs `unevaluatedProperties: false`

Do not automatically add the property to the schema. Re-run the binding-design placement test.

## Cardinality And Encoding Errors

When a list is too long or short:

- inspect preprocessed DTS, not only `.dts` source
- inspect provider `#*-cells` values
- align the resource list with its `*-names` list
- check variant condition selection
- remember that a multi-cell specifier is one phandle-array entry, not several logical resources
- inspect the normalized value printed in the diagnostic

A schema-side `maxItems` change is correct only if hardware permits the additional resource and its ordering is defined.

## Example-Only Failures

If real DTBs pass but an example fails, check:

- missing `#include`
- wrong default address/size-cell assumption
- undefined label/provider
- stale resource names after review edits
- unrelated wrapper nodes selecting other schemas
- YAML block indentation

Keep the example minimal, but do not remove the property that exposes a real schema bug.

## Prove A Schema Was Not Skipped

Use a controlled negative mutation in a representative node or example:

1. remove a property uniquely required by the target schema
2. run the targeted check
3. confirm the target schema reports the expected error
4. restore the valid source
5. confirm the error disappears

Do not commit the mutation. This is a coverage probe, analogous to testing that an alarm is connected.

## Baseline Management

Large trees can contain existing warnings. Capture before and after logs with identical:

- source revision except for the patch
- tool versions
- architecture/configuration
- schema filters
- DTB targets
- warning flags

Classify new, resolved, and unchanged diagnostics. “Pre-existing” is not credible without a comparable baseline.

## Minimal Reproducer Strategy

Reduce in this order:

1. one schema file via `DT_SCHEMA_FILES`
2. one architecture/DTB target
3. one node or one schema example
4. one compatible branch
5. one failing property

Preserve the parent bus or referenced schema context needed to reproduce the issue. Over-reduction can make selection disappear and create a false pass.

## Root-Cause Checklist

- Which pipeline stage failed first?
- Did the intended schema validate successfully itself?
- Did it select the intended node?
- Which normalized instance value failed?
- Which exact schema branch and keyword rejected it?
- Is the source wrong, the schema wrong, or the hardware contract unresolved?
- Does the proposed fix retain negative coverage?
- Did the fix expose a second real error previously hidden by the first?
- Does a clean rerun use fresh generated artifacts?

## Authoritative References

- [Linux schema-writing guide: running checks](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [`dt-schema` project](https://github.com/devicetree-org/dt-schema)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to the [Binding Schema Authoring And Validation Lab](binding-schema-authoring-and-validation-lab.md).
