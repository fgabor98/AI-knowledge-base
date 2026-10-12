---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Includes, Preprocessing, And Macros

## Two Include Mechanisms

DTS ecosystems commonly use two different processors. Both appear in mainline Linux:

```dts
/include/ "integrator.dtsi"            /* arch/arm/boot/dts/arm/integratorcp.dts */
#include "k3-am642.dtsi"               /* arch/arm64/boot/dts/ti/k3-am642-sk.dts */
#include <dt-bindings/gpio/gpio.h>     /* same file */
```

| Form | Processor | Typical use |
|---|---|---|
| `/include/ "file"` | `dtc` | native inclusion of DTS syntax |
| `#include "file"` | C preprocessor | DTSI inclusion in kernel-style builds |
| `#include <dt-bindings/...>` | C preprocessor | symbolic binding constants |

They are not interchangeable in every command. A source using `#include` normally needs preprocessing before `dtc` sees it.

## Native `/include/`

`dtc` can directly insert another source file, as the ARM Integrator/CP board does:

```dts
/dts-v1/;
/include/ "integrator.dtsi"

/ {
        model = "ARM Integrator/CP";
        compatible = "arm,integrator-cp";
        /* ... */
};
```

Search behavior depends on the source location and `dtc` include directories supplied with `-i`. Native includes can themselves include other files. The DTB does not retain a normal include provenance record.

## C Preprocessing

Linux kernel builds preprocess every DTS. The rule in `scripts/Makefile.dtbs` is:

```make
DTC_INCLUDE := $(srctree)/scripts/dtc/include-prefixes

dtc_cpp_flags = -Wp,-MMD,$(depfile).pre.tmp -nostdinc -I $(DTC_INCLUDE) -undef -D__DTS__

cmd_dtc =         $(HOSTCC) -E $(dtc_cpp_flags) -x assembler-with-cpp -o $(dtc-tmp) $< ;         $(DTC) -o $@ -b 0 $(addprefix -i,$(dir $<) $(DTC_INCLUDE))                $(DTC_FLAGS) -d $(depfile).dtc.tmp $(dtc-tmp) ;         ...
```

Two details matter when you reproduce it:

- `-nostdinc -I scripts/dtc/include-prefixes` means `<dt-bindings/...>` resolves only through that prefix directory, which links to `include/dt-bindings` and the architecture DTS trees. Quoted includes such as `"k3-am64.dtsi"` resolve relative to the including file.
- `DTC_FLAGS` changes with `W=1`/`W=2`, and `-@` is added automatically for base DTBs that overlays are applied to.

A standalone equivalent for the AM642 SK is shown in the [First Device Tree Lab](../foundations/first-device-tree-lab.md#step-10-build-the-real-board-dtb). Inspect `make V=1` output rather than guessing flags.

## Binding Constants

Headers under `include/dt-bindings/` give readable names to values defined by bindings. In the AM64x sources:

```dts
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <dt-bindings/soc/ti,sci_pm_domain.h>

interrupts = <GIC_SPI 178 IRQ_TYPE_LEVEL_HIGH>;               /* -> <0 178 4> */
power-domains = <&k3_pds 146 TI_SCI_PD_EXCLUSIVE>;           /* -> <&k3_pds 146 1> */
gpio = <&main_gpio0 48 GPIO_ACTIVE_HIGH>;                    /* -> <&main_gpio0 48 0> */
```

After preprocessing, constants become integers. The names improve reviewability but do not change the property encoding or validate the specifier.

When debugging, inspect all four views:

```text
maintainable source: TI_SCI_PD_EXCLUSIVE
preprocessed source: 1
binding: meaning of that argument position (exclusive vs shared TI-SCI device ownership)
compiled DTB: final cells
```

## Object-Like And Function-Like Macros

Object-like constants are easy to audit. `k3-serdes.h` names the SERDES lane-mux values used by AM64x boards:

```c
#define AM64_SERDES0_LANE0_PCIE0                0x0
#define AM64_SERDES0_LANE0_USB                  0x1
```

```dts
&serdes_ln_ctrl {
        idle-states = <AM64_SERDES0_LANE0_USB>;
};
```

Function-like macros reduce repetition but can hide tuple shape. `k3-pinctrl.h` defines:

```c
#define AM64X_IOPAD(pa, val, muxmode)   (((pa) & 0x1fff)) ((val) | (muxmode))
```

so each `AM64X_IOPAD(...)` entry inside `pinctrl-single,pins = < ... >` expands to **two** cells: the pad register offset and the combined configuration/mux value. `AM64X_IOPAD(0x0260, PIN_INPUT_PULLUP, 0)` becomes `0x260 0x64000`. A reviewer who cannot see that shape cannot check it against the `pinctrl-single` binding's `#pinctrl-cells`.

Use macros primarily for standardized constants and established architecture patterns. Avoid creating a private language that prevents reviewers from seeing addresses, cell counts, or provider arguments directly.

## Conditional Preprocessing

The C preprocessor supports `#if`, `#ifdef`, and related directives. TI's K3 board files do not use them, but some boards do. The Renesas RZ/G2L SMARC board selects mutually exclusive PMOD functions with macros edited in the source (`arch/arm64/boot/dts/renesas/r9a07g044l2-smarc.dts`):

```dts
#define PMOD1_SER0      1
#define PMOD_MTU3       0

#if (PMOD_MTU3 && PMOD1_SER0)
#error "Cannot set as PMOD_MTU3 and PMOD1_SER0 are mutually exclusive "
#endif
```

Each macro combination produces a different hardware description from the same filename, and only one combination is built and validated by default. For concrete boards, separate DTS files—like TI's `k3-am642-evm.dts` plus its `.dtso` overlays—usually make variants easier to review, validate, and release. If configuration macros truly control a build, record the macro set and validate every emitted tree.

## Inspecting The Actual Preprocessed Source

When `dtc` reports a surprising line or token:

1. rebuild verbosely to capture the preprocessor command
2. preserve the intermediate preprocessed file
3. locate the node or property in that file
4. inspect macro definitions and include order
5. run `dtc` against the preserved intermediate input

Useful commands in a kernel tree include:

```sh
make V=1 ARCH=arm64 ti/k3-am642-sk.dtb
find . -name '.k3-am642-sk.dtb.dts.tmp'
```

With current Kbuild the preprocessed file is a hidden `.<target>.dtb.dts.tmp` next to the output (see `dtc-tmp` in `scripts/Makefile.dtbs`), but names vary by kernel version. The verbose command is the source of truth.

Preprocessor line markers can confuse standalone tooling. Kernel build flags and the `dtc` parser account for their expected form; copying only half the pipeline often creates misleading errors.

## Include Resolution Failures

### File Not Found

Determine:

- which processor reported the error
- whether quotes or angle brackets were used
- the current source file's directory
- all preprocessor `-I` paths
- all `dtc -i` paths
- whether the build uses generated include directories

Do not solve it by copying a header beside the DTS; that creates an untracked fork of a binding interface.

### Macro Is Not Expanded

Likely causes:

- `dtc` was invoked directly on CPP-dependent source
- the header was not included
- the macro is guarded by another definition
- the wrong header version is on the include path
- the token is built in a way the preprocessor does not recognize

### Standalone Build Differs From Kernel Build

Compare compiler versions, preprocessing, include directories, warning flags, generated headers, and exact entry-point DTS. “Both use `dtc`” is not enough to establish equivalent builds.

## Dependency And Reproducibility Concerns

Include provenance matters as soon as a DTB ships, especially when vendor SDKs carry multiple kernel or U-Boot copies of similarly named DTSI files. [Build Pipeline, Preprocessing, And Artifact Provenance](../build-and-diagnostic-tools/build-pipeline-preprocessing-and-artifact-provenance.md#reproducibility-record) lists what a reproducible build must record.

## Exercises

1. Explain why `dtc board.dts` may reject a valid kernel source containing `#include`.
2. Find the numeric expansion of `GPIO_ACTIVE_LOW` without replacing it in maintained source.
3. List three risks of a macro that expands to several phandle-and-argument tuples.
4. Explain why a conditional node can escape validation.
5. Identify which include-path family to inspect for `/include/` versus `#include`.

## References And Next Step

- [Devicetree source format](https://devicetree-specification.readthedocs.io/en/stable/source-language.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Linux kernel Device Tree documentation](https://docs.kernel.org/devicetree/index.html)

Continue with [Hardware-Based Layering And Source Style](hardware-based-layering-and-source-style.md).
