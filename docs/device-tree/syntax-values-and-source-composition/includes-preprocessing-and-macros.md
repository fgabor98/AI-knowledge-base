---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Includes, Preprocessing, And Macros

## Two Include Mechanisms

DTS ecosystems commonly use two different processors:

```dts
/include/ "trainer-common.dtsi"
#include "trainer-soc.dtsi"
#include <dt-bindings/gpio/gpio.h>
```

| Form | Processor | Typical use |
|---|---|---|
| `/include/ "file"` | `dtc` | native inclusion of DTS syntax |
| `#include "file"` | C preprocessor | DTSI inclusion in kernel-style builds |
| `#include <dt-bindings/...>` | C preprocessor | symbolic binding constants |

They are not interchangeable in every command. A source using `#include` normally needs preprocessing before `dtc` sees it.

## Native `/include/`

`dtc` can directly insert another source file:

```dts
/dts-v1/;
/include/ "trainer-soc.dtsi"

/ {
        model = "Trainer Native-Include Board";
};
```

Search behavior depends on the invoking command, source location, and `dtc` include directories supplied with `-i`. Always reproduce the real build command when resolving an unexpected file.

Native includes can themselves include other files. The effective result is textual/source composition before final tree construction; the DTB does not retain a normal include provenance record.

## C Preprocessing

Linux kernel builds preprocess DTS so sources can include headers and symbolic constants. A simplified standalone flow is:

```sh
cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp \
    -I include -I arch/arm64/boot/dts \
    board.dts board.preprocessed.dts

dtc -I dts -O dtb -o board.dtb board.preprocessed.dts
```

This is illustrative, not a universal replacement for a kernel build rule. Real kernel builds supply generated paths, architecture directories, dependency files, warning flags, and their in-tree `dtc`. Inspect the build's verbose output rather than guessing its flags.

## Binding Constants

Headers under `include/dt-bindings/` give readable names to values defined by bindings:

```dts
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/interrupt-controller/irq.h>

reset-gpios = <&gpio0 7 GPIO_ACTIVE_LOW>;
interrupts = <5 IRQ_TYPE_LEVEL_LOW>;
```

After preprocessing, constants become integers. The names improve reviewability but do not change the property encoding or validate the specifier.

When debugging, inspect both views:

```text
maintainable source: GPIO_ACTIVE_LOW
preprocessed source: numeric macro expansion
binding: meaning of that argument position
compiled DTB: final cells
```

## Object-Like And Function-Like Macros

Simple symbolic constants are usually easy to audit:

```c
#define TRAINER_UART_CLOCK 24000000
```

```dts
clock-frequency = <TRAINER_UART_CLOCK>;
```

Function-like macros can reduce repetition but can also hide tuple shape:

```c
#define TRAINER_REG(addr, size) <(addr) (size)>
```

```dts
reg = TRAINER_REG(0x1000, 0x100);
```

Use macros primarily for standardized constants and established architecture patterns. Avoid creating a private language that prevents reviewers from seeing addresses, cell counts, or provider arguments directly.

## Conditional Preprocessing

The C preprocessor supports `#if`, `#ifdef`, and related directives, but conditional hardware descriptions can create invisible variants:

```dts
#ifdef TRAINER_HAS_SENSOR
        sensor@48 { /* ... */ };
#endif
```

For concrete boards, separate DTS files or hardware-based DTSI layers usually make variants easier to review, validate, and release. If generated configuration truly controls a build, record the macro set and validate every emitted tree.

## Inspecting The Actual Preprocessed Source

When `dtc` reports a surprising line or token:

1. rebuild verbosely to capture the preprocessor command
2. preserve the intermediate preprocessed file
3. locate the node or property in that file
4. inspect macro definitions and include order
5. run `dtc` against the preserved intermediate input

Useful commands in a kernel tree include:

```sh
make V=1 ARCH=arm64 dtbs
find build -name '*.dts.tmp' -o -name '*.dtb.dts.tmp'
```

Intermediate names vary by kernel version and build setup. The verbose command is the source of truth.

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

A reproducible Device Tree build records:

- compiler and preprocessor versions
- full include search paths
- source revision, including headers
- macro definitions and generated inputs
- selected DTS targets
- `dtc` flags such as `-@` and warning controls
- output hashes

Include provenance is especially important when vendor SDKs carry multiple kernel or U-Boot copies of similarly named DTSI files.

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
