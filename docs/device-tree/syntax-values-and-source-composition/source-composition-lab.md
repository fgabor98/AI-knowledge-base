---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Source Composition Lab

## Goal

Build two board DTBs from one SoC DTSI and one module DTSI, then inspect preprocessing, amendments, deletions, value encodings, and final artifact differences.

## File Layout

Work in a disposable directory:

```text
trainer-lab/
├── include/dt-bindings/trainer.h
├── trainer-soc.dtsi
├── trainer-som.dtsi
├── trainer-carrier-a.dts
└── trainer-carrier-b.dts
```

Create the header:

```c
#define TRAINER_UART_CLOCK_HZ 24000000
#define TRAINER_FEATURE_BASIC 1
#define TRAINER_FEATURE_DIAGNOSTIC 2
```

Create `trainer-soc.dtsi`:

```dts
/ {
        #address-cells = <1>;
        #size-cells = <1>;

        soc {
                compatible = "simple-bus";
                #address-cells = <1>;
                #size-cells = <1>;
                ranges;

                uart0: serial@1000 {
                        compatible = "example,trainer-uart";
                        reg = <0x1000 0x100>;
                        status = "disabled";
                };
        };
};
```

Create `trainer-som.dtsi`:

```dts
#include <dt-bindings/trainer.h>
#include "trainer-soc.dtsi"

/ {
        osc: clock-24000000 {
                compatible = "fixed-clock";
                #clock-cells = <0>;
                clock-frequency = <TRAINER_UART_CLOCK_HZ>;
        };

        module-data {
                compatible = "example,trainer-module-data";
                serial-bytes = [01 23 45 67];
                feature-level = <TRAINER_FEATURE_BASIC>;
                factory-calibrated;
        };
};

&uart0 {
        clocks = <&osc>;
};
```

Create carrier A:

```dts
/dts-v1/;

#include "trainer-som.dtsi"

/ {
        model = "Example Trainer Carrier A";
        compatible = "example,trainer-carrier-a";

        aliases {
                serial0 = &uart0;
        };
};

&uart0 {
        status = "okay";
};
```

Create carrier B:

```dts
/dts-v1/;

#include "trainer-som.dtsi"

/ {
        model = "Example Trainer Carrier B";
        compatible = "example,trainer-carrier-b";
};

&{/module-data} {
        /delete-property/ factory-calibrated;
        feature-level = <TRAINER_FEATURE_DIAGNOSTIC>;
};
```

## Predict Before Building

For each board, record:

- final UART status
- whether `serial0` exists
- whether `factory-calibrated` exists
- final numeric feature level
- oscillator frequency
- how the UART `clocks` value will be encoded

## Preprocess

Use the system C preprocessor:

```sh
cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp \
    -I include -I . trainer-carrier-a.dts carrier-a.pp.dts

cpp -nostdinc -undef -D__DTS__ -x assembler-with-cpp \
    -I include -I . trainer-carrier-b.dts carrier-b.pp.dts
```

Inspect the output:

```sh
rg -n "clock-frequency|feature-level|serial@1000|factory-calibrated" \
    carrier-a.pp.dts carrier-b.pp.dts
```

Confirm that macros expanded and includes became one compilation unit. Preprocessor line markers may remain; `dtc` normally understands the form produced by standard workflows.

## Compile And Decompile

```sh
dtc -I dts -O dtb -o trainer-carrier-a.dtb carrier-a.pp.dts
dtc -I dts -O dtb -o trainer-carrier-b.dtb carrier-b.pp.dts

dtc -I dtb -O dts -o carrier-a.final.dts trainer-carrier-a.dtb
dtc -I dtb -O dts -o carrier-b.final.dts trainer-carrier-b.dtb
```

Capture identities:

```sh
sha256sum trainer-carrier-a.dtb trainer-carrier-b.dtb
```

Inspect the final nodes rather than assuming merge order:

```sh
rg -n -A12 "serial@1000|module-data|aliases" \
    carrier-a.final.dts carrier-b.final.dts
```

## Inspect Value Types

If `fdtget` is available:

```sh
fdtget trainer-carrier-a.dtb / model
fdtget -tx trainer-carrier-a.dtb /module-data serial-bytes
fdtget -tu trainer-carrier-a.dtb /module-data feature-level
fdtget -tu trainer-carrier-a.dtb /clock-24000000 clock-frequency
fdtget -p trainer-carrier-b.dtb /module-data
```

Explain why the byte array query and integer-cell query need different output interpretations.

## Exercise 1: Add A 64-Bit Value

Add this to `module-data`:

```dts
unique-id = <0x01234567 0x89abcdef>;
```

Rebuild and query the two cells. Then add a separate explicitly sized property:

```dts
wide-samples = /bits/ 64 <0x0123456789abcdef>;
```

Compare decompiled output and raw bytes. Explain why these may contain the same eight bytes while representing different binding-level array models.

## Exercise 2: Break Include Resolution

Remove `-I include` from the preprocessing command. Identify which processor reports the missing header and why adding `dtc -i include` would not fix this `#include` failure.

Restore the command.

## Exercise 3: Replace Versus Delete

In carrier B, replace:

```dts
/delete-property/ factory-calibrated;
```

with:

```dts
factory-calibrated;
```

Rebuild and prove that the property still exists. Restore the deletion and explain why a zero-length property represents presence.

## Exercise 4: Reveal An Ordering Hazard

Create a late include that sets carrier B's `feature-level` back to basic, and include it after the amendment. Build and observe the final value. Then remove the ordering dependency by assigning the property only in the physical layer that owns the distinction.

## Exercise 5: Review The Architecture

Answer:

1. Why is the oscillator in the module DTSI?
2. Why is final UART availability in carrier A?
3. Is deleting `factory-calibrated` truthful, or would separate module variants be clearer?
4. Should the feature-level constants live in a public binding header or a private local header?
5. Which files must change if the UART register window is wrong for every product?

## Failure Investigation Checklist

If the lab does not build:

1. identify whether `cpp` or `dtc` failed
2. inspect the exact preprocessed file
3. verify include paths and filenames
4. check that `/dts-v1/;` appears in the final compilation unit
5. locate every definition and amendment of the failing node
6. simplify the command without changing processors
7. compare with the authoritative build rule you intend to reproduce

## Completion Checklist

- [ ] I built two DTBs from shared physical layers.
- [ ] I inspected macro expansion before compilation.
- [ ] I reconstructed the final node after amendments.
- [ ] I distinguished byte arrays, cells, and 64-bit representations.
- [ ] I demonstrated replacement and deletion behavior.
- [ ] I explained why CPP and `dtc` include paths are different.
- [ ] I identified and removed an ordering-dependent design.
- [ ] I can justify which hardware layer owns each property.

## References And Next Steps

- [Devicetree source format](https://devicetree-specification.readthedocs.io/en/stable/source-language.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Device Tree Build And Validation](../../build-systems/advanced/device-tree-build-and-validation.md)

Return to [Syntax, Values, And Source Composition](../syntax-values-and-source-composition.md), or continue with [Provider-Consumer Relationships](../provider-consumer-relationships.md).
