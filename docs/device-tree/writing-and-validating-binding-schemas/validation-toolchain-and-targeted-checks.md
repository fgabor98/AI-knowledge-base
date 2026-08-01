---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Validation Toolchain And Targeted Checks

Binding validation has two primary targets: validate schema documents and their examples with `dt_binding_check`, then validate compiled platform trees against the processed schemas with `dtbs_check`. Run them in that order when diagnosing a change.

## Toolchain Components

The kernel workflow combines:

- a C preprocessor for DTS includes and macros
- `dtc` for compiling source and issuing structural warnings
- the `dtschema` Python package and commands such as `dt-doc-validate`, `dt-mk-schema`, and `dt-validate`
- kernel make targets that supply paths, dependencies, examples, and generated schemas
- architecture cross-compilers and build tools for platform DTBs

The upstream guide documents installation of `dtschema` and recommends `yamllint`. Pin or record tool versions in reproducible CI because validator updates can expose issues without a source change.

## Gate 1: Binding Documents And Examples

From a configured Linux kernel tree:

```bash
make dt_binding_check
```

This validates binding documents against the DT meta-schema, resolves and processes schemas, and compiles/validates examples. It is the authoritative first gate before trusting DTB validation.

The upstream workflow also supports building an individual schema target:

```bash
make Documentation/devicetree/bindings/media/acme,ax-capture.yaml
```

Exact invocation details can vary with kernel version and working directory; use the current kernel documentation and nearby build conventions.

## Targeted Binding Checks

Use `DT_SCHEMA_FILES` for quick iterations:

```bash
make dt_binding_check \
  DT_SCHEMA_FILES=Documentation/devicetree/bindings/media/acme,ax-capture.yaml
```

The value supports fixed-string partial matches. Multiple patterns are colon-separated:

```bash
make dt_binding_check \
  DT_SCHEMA_FILES=acme,ax-capture.yaml:video-interface-devices.yaml
```

A directory fragment can select a group:

```bash
make dt_binding_check DT_SCHEMA_FILES=/media/
```

Always inspect which files the pattern selected. A typo or over-specific path can produce a fast, clean run that did not exercise the intended schema.

## Gate 2: Platform DTBs

```bash
make ARCH=arm64 dtbs_check
```

This builds platform DTBs and validates their nodes against matching schemas. Provide the same architecture, configuration, output directory, and cross-compiler context used to build the affected DTBs.

For an out-of-tree build:

```bash
make O=build ARCH=arm64 dtbs_check
```

Depending on environment, set `CROSS_COMPILE` or use a native-capable compiler as the kernel build requires.

## Targeted DTB Validation

Limit schemas during iteration:

```bash
make O=build ARCH=arm64 dtbs_check \
  DT_SCHEMA_FILES=Documentation/devicetree/bindings/media/acme,ax-capture.yaml
```

This limits matching schemas, not necessarily the set of platform DTBs built. Use architecture-specific DTB targets and the current kernel build interface to reduce the instance set when needed.

Do not present a targeted run as full-tree validation. It proves only the selected schema/DTB population under that command.

## Combined Checks

Upstream supports running both primary targets:

```bash
make dt_binding_check dtbs_check
```

For debugging, separate them so the first failure stage is obvious. For CI, a combined invocation may be efficient after dependencies and logs are well understood.

## `dtc` Warnings Remain Relevant

Schema validation does not replace compiler checks. `dtc` detects structural issues such as unit-address conventions, malformed properties, graph consistency classes, and address/size problems. Kernel warning levels may expose additional checks:

```bash
make O=build ARCH=arm64 W=1 dtbs_check
```

Use the warning level expected by the affected maintainer tree. Classify warnings rather than suppressing them indiscriminately.

## Why `dt_binding_check` Must Come First

`dtbs_check` skips binding schemas that fail processing. The platform run can therefore omit the very schema being developed and still continue. A clean or reduced DTB log is not proof of participation.

Required evidence sequence:

```text
target binding check passes
  -> processed schema includes target ID
  -> representative DTB node selects target schema
  -> deliberate invalid mutation fails under target schema
  -> corrected DTB passes
```

## Build Context Matters

Record:

- kernel commit or release
- `dtschema` version
- `dtc` version
- architecture and configuration
- output directory
- exact `DT_SCHEMA_FILES` value
- exact DTB targets or platform set
- warning level

Generated schema caches and out-of-tree outputs can preserve stale artifacts. When results contradict source changes, inspect timestamps and generated paths, then use the build system's supported clean/rebuild scope rather than deleting broad directories blindly.

## Local Iteration Versus Submission Evidence

Local iteration:

- target one schema
- build one or a few relevant DTBs
- retain the first actionable error
- use deliberate invalid mutations

Pre-submission:

- run the complete binding check expected by the tree
- validate all affected architecture DTBs
- expand beyond the target schema for composition regressions
- compare warnings with a recorded baseline
- run checkpatch/subsystem checks required by the patch workflow

## Command Checklist

```text
[ ] target schema pattern matches the intended file
[ ] dt_binding_check passes before dtbs_check interpretation
[ ] examples compile and validate
[ ] affected DTBs are actually built
[ ] representative nodes select the schema
[ ] negative mutation is rejected by that schema
[ ] corrected node passes
[ ] dtc warnings are reviewed
[ ] broader non-targeted checks pass or deltas are explained
[ ] tool and source versions are recorded
```

## Authoritative References

- [Linux schema-writing guide: testing](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [`dt-schema` project and tooling](https://github.com/devicetree-org/dt-schema)
- [Linux Kbuild documentation](https://docs.kernel.org/kbuild/)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)

## Continue

Proceed to [Diagnosing Schema, Example, And DTB Failures](diagnosing-schema-example-and-dtb-failures.md).
