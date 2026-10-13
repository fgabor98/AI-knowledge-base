---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Build Pipeline, Preprocessing, And Artifact Provenance

The file an engineer edits is rarely the complete source compiled by `dtc`. Kernel-style builds combine a board entry point, DTSI includes, C-preprocessor headers/macros, generated files, architecture include paths, and build flags. Prove that chain before interpreting the output.

## Preprocessing Recap

The C preprocessor (`#include`, macros, `dt-bindings` constants) runs before `dtc`, which then consumes native `/include/` directives, so a missing or shadowed file can arise at different stages. [Includes, Preprocessing, And Macros](../syntax-values-and-source-composition/includes-preprocessing-and-macros.md) shows a simplified standalone `cpp` + `dtc` pipeline; it is illustrative only. For provenance, capture the target tree's real build rule as described below, and preserve the preprocessed output as a diagnostic artifact, not maintainable source.

## Kbuild Is The Reproducible Entry Point

Typical build:

```bash
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dtbs
```

Target one artifact using the path expected by the architecture makefiles:

```bash
make O=build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
  arch/arm64/boot/dts/ti/k3-am62-lp-sk.dtb
```

Exact target paths vary by architecture and kernel version. First find the source and its Makefile entry:

```bash
rg -n 'k3-am62-lp-sk.*\.dtb' arch/arm64/boot/dts
rg --files arch/arm64/boot/dts | rg 'k3-am62-lp-sk.*\.dts$'
```

Use `make help` and neighboring platform conventions when a direct target is rejected.

## Capture Exact Commands

```bash
make O=build ARCH=arm64 V=1 \
  arch/arm64/boot/dts/ti/k3-am62-lp-sk.dtb
```

Verbose output reveals:

- preprocessor and `dtc` executables
- include directories
- generated-header paths
- input/output paths
- warning and symbol flags
- output-tree location

If nothing rebuilds, the command may not print. Update the relevant source timestamp through a real edit, invoke the build system's supported rebuild target, or inspect generated command records; do not delete the entire output tree without need.

## Generated `.cmd` Records

Kbuild commonly stores hidden command/dependency files beside outputs or intermediates. Names vary, but they can show:

- command used for the target
- tracked prerequisites
- configuration and generated headers
- why a target is considered up to date

Find candidates:

```bash
find build/arch/arm64/boot/dts -name '*.cmd' -print
rg -n 'k3-am62-lp-sk\.dtb|k3-am62-lp-sk\.dts' build/arch/arm64/boot/dts -g '*.cmd'
```

Treat these as generated evidence tied to one build tree, not portable configuration files.

## Inspect Preprocessed Source

The most reliable approach is to copy the exact preprocessor invocation from `V=1` output and change only the final output destination for inspection. Preserve all flags and include paths.

Look for:

- which conditional branches survived
- macro-expanded numeric constants
- included node definitions and later amendments
- generated header values
- unexpected duplicate nodes/properties before `dtc` merges them
- `#line` directives that map back to source

Search narrowly:

```bash
rg -n 'bridge-hdmi@3b|tps65219|vddshv_sdio|status' board.preprocessed.dts
```

Do not edit the preprocessed file and deploy its result as a normal fix. Correct the owning source or generated input.

## Output Trees

With `O=build`, generated artifacts belong under the output tree. The source tree may also contain stale artifacts from an earlier in-tree build.

Before copying a DTB, record:

```bash
realpath build/arch/arm64/boot/dts/ti/k3-am62-lp-sk.dtb
stat build/arch/arm64/boot/dts/ti/k3-am62-lp-sk.dtb
sha256sum build/arch/arm64/boot/dts/ti/k3-am62-lp-sk.dtb
```

Never choose “the newest `*.dtb` found anywhere” as a release rule. Map one build target to one packaging input explicitly.

## Generated Inputs

DT builds can consume:

- `include/dt-bindings/` constants
- configuration-dependent generated headers
- vendor-generated pinmux or board fragments
- symlinked include-prefix trees
- external source paths in downstream build systems

Record generator version, input, output hash, and ownership. A clean DTS diff can still yield a changed DTB because a generated header or tool changed.

## Trace Into Packaging

After build, find every consumer of the output:

```bash
rg -n 'k3-am62-lp-sk\.dtb|fdtfile|FDT|devicetree' \
  Makefile scripts arch boot packaging recipes
```

The relevant next stage may:

- copy to a boot filesystem
- rename the file
- embed it in FIT
- bundle it with firmware
- sign or compress it
- select another variant through metadata

Hash the packaged/extracted DTB, not only the build output.

## Reproducibility Record

```text
source commit and dirty-state summary
entry-point DTS
ordered/generated input identities
kernel/build configuration
preprocessor and dtc commands
compiler/dtc versions
output path and hash
package path/container and extracted hash
```

Two builds with equal semantic trees but different exact bytes can indicate ordering, padding, symbol, compiler, or packaging differences. Preserve both semantic and byte-level evidence.

## Common Failures

- editing a DTS not listed in the target Makefile
- building in-tree but deploying from `O=build`, or the reverse
- a generated vendor fragment overwrites the manual edit
- a macro selects another variant
- stale output is copied because the requested target never rebuilt
- packaging uses a different filename or FIT configuration
- bootloader and Linux control-tree builds are confused
- a decompiled output is treated as the original include structure

## Authoritative References

- [Linux Kbuild documentation](https://docs.kernel.org/kbuild/)
- [Linux Devicetree source coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to [`dtc`, Symbols, Warnings, And Round Trips](dtc-symbols-warnings-and-round-trips.md).
