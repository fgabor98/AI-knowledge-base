---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# First Device Tree Lab

## Goal

Build a small standalone tree, inspect its binary form, make controlled changes, and distinguish source syntax errors from semantic errors. This lab does not require a development board or kernel source tree.

## Expected Outcome

At the end, you will have:

- compiled `trainer-board.dts` into `trainer-board.dtb`
- decompiled the DTB into a source-like representation
- inspected strings, cells, paths, and phandles
- demonstrated that comments and labels are not preserved like ordinary properties
- introduced and diagnosed both a compiler error and a semantic error
- recorded evidence that identifies a specific DTB artifact

## Requirements

Install the Device Tree Compiler package supplied by your development distribution. Confirm the required commands:

```sh
dtc --version
command -v dtc
command -v fdtdump
command -v fdtget
sha256sum --version
```

Some packages do not install every `libfdt` utility. `dtc` is required; `fdtdump` and `fdtget` are strongly recommended. Work in a disposable directory, not in a production kernel tree.

## Step 1: Create The Source

Create `trainer-board.dts` with this content:

```dts
/dts-v1/;

/ {
        model = "Example Trainer Board";
        compatible = "example,trainer-board";
        #address-cells = <1>;
        #size-cells = <1>;

        aliases {
                serial0 = &uart0;
        };

        chosen {
                stdout-path = "serial0:115200n8";
        };

        osc: clock-24000000 {
                compatible = "fixed-clock";
                #clock-cells = <0>;
                clock-frequency = <24000000>;
        };

        soc {
                compatible = "simple-bus";
                #address-cells = <1>;
                #size-cells = <1>;
                ranges;

                uart0: serial@1000 {
                        compatible = "example,trainer-uart";
                        reg = <0x1000 0x100>;
                        clocks = <&osc>;
                        status = "okay";
                };
        };
};
```

Before compiling, predict:

- the path of the UART
- the number of address/size pairs in `reg`
- the path stored in the `serial0` alias
- which node will receive a generated phandle

## Step 2: Compile The DTB

```sh
dtc -I dts -O dtb -o trainer-board.dtb trainer-board.dts
```

Confirm that the output exists and capture its identity:

```sh
ls -l trainer-board.dts trainer-board.dtb
sha256sum trainer-board.dtb
```

No compiler output usually means syntax compilation succeeded. It does not mean that the fictional compatible strings have real bindings or drivers.

## Step 3: Decompile It

```sh
dtc -I dtb -O dts -o trainer-board.decompiled.dts trainer-board.dtb
```

Inspect the result:

```sh
sed -n '1,200p' trainer-board.decompiled.dts
```

Look for these transformations:

- source comments are absent
- formatting may differ
- label names are not ordinary node names
- `&uart0` in `/aliases` became a path string
- `&osc` became a numeric phandle reference
- the provider node gained a numeric `phandle` property

Do not treat the decompiled file as the maintainable original source. It is useful evidence of what the DTB contains.

## Step 4: Inspect The Flattened Layout

If `fdtdump` is available:

```sh
fdtdump trainer-board.dtb
```

Find the header fields and the decoded tree. Relate the reported offsets and sizes to:

- header
- memory reservation block
- structure block
- strings block

Then view the first bytes without trying to parse the whole blob manually:

```sh
od -Ax -tx1z -N 64 trainer-board.dtb
```

The first four bytes should contain the FDT magic in big-endian order. The rest of the header contains big-endian fields; raw host-order interpretation is unsafe.

## Step 5: Query Individual Properties

Read string properties:

```sh
fdtget trainer-board.dtb / model
fdtget trainer-board.dtb / compatible
fdtget trainer-board.dtb /aliases serial0
fdtget trainer-board.dtb /chosen stdout-path
```

Read cells as hexadecimal values:

```sh
fdtget -tx trainer-board.dtb /soc/serial@1000 reg
fdtget -tx trainer-board.dtb /clock-24000000 clock-frequency
fdtget -tx trainer-board.dtb /soc/serial@1000 clocks
```

List nodes and properties:

```sh
fdtget -l trainer-board.dtb /
fdtget -l trainer-board.dtb /soc
fdtget -p trainer-board.dtb /soc/serial@1000
```

If your `fdtget` version uses different option spelling, consult `fdtget --help`; utility versions vary more than the underlying DTB concepts.

## Step 6: Prove Label, Path, Alias, And Phandle Roles

Record all four UART identities:

| Form | Expected value | Where it matters |
|---|---|---|
| source label | `uart0` | source references |
| node name | `serial@1000` | child identity under `/soc` |
| full path | `/soc/serial@1000` | logical tree lookup |
| alias | `serial0` | shorthand understood by consumers |

Now record the oscillator's numeric phandle and the UART's `clocks` value. They should match within this blob. Recompile after an unrelated source reordering and observe that relying on a particular numeric phandle would be unsafe.

## Step 7: Make A Controlled Hardware Change

Change the oscillator frequency from 24 MHz to 25 MHz:

```dts
clock-frequency = <25000000>;
```

Recompile to a second artifact:

```sh
dtc -I dts -O dtb -o trainer-board-25mhz.dtb trainer-board.dts
sha256sum trainer-board.dtb trainer-board-25mhz.dtb
fdtget -tu trainer-board-25mhz.dtb /clock-24000000 clock-frequency
```

Expected result:

- the hashes differ
- the queried value is `25000000`
- the node name remains `clock-24000000`, which is now misleading

Rename the node to `clock-25000000`, rebuild, and confirm that the UART reference still resolves because source refers to the `osc` label rather than the old path.

This demonstrates why labels are useful during source composition and why descriptive node names should remain accurate.

## Step 8: Diagnose A Syntax Error

Temporarily remove the semicolon after `status = "okay"` and compile:

```sh
dtc -I dts -O dtb -o broken.dtb trainer-board.dts
```

Record:

- the line reported by `dtc`
- whether the actual omission is on that line or immediately before it
- whether `broken.dtb` was produced

Restore the semicolon before continuing.

## Step 9: Diagnose A Semantic Error

Change the UART register property to:

```dts
reg = <0x1000>;
```

The parent declares one address cell and one size cell, so this no longer contains a complete address/size pair. Compile it.

Depending on warning options and compiler version, `dtc` may warn or may still produce a blob. This is the lesson: the compiler cannot prove all binding semantics from source syntax alone.

Restore the property, then compile with explicit warnings where supported:

```sh
dtc -I dts -O dtb -Waddress_cells_is_cell \
    -Wsize_cells_is_cell -Wreg_format \
    -o trainer-board.dtb trainer-board.dts
```

In a Linux kernel tree, schema validation adds binding knowledge that standalone syntax compilation lacks. That workflow is covered in [Writing And Validating Binding Schemas](../writing-and-validating-binding-schemas.md).

## Step 10: Build An Evidence Record

Write a short record containing:

```text
source path:
source revision or hash:
build command:
dtc version:
output path:
DTB SHA-256:
model property:
root compatible list:
UART path:
UART reg cells:
oscillator frequency:
```

This habit scales directly to deployed boards. A useful debugging statement is “the running tree contains X from artifact hash Y,” not merely “I changed the DTS.”

## Optional Exercise: Compile With Symbols

Compile with symbol generation:

```sh
dtc -@ -I dts -O dtb -o trainer-board-symbols.dtb trainer-board.dts
dtc -I dtb -O dts -o trainer-board-symbols.dts trainer-board-symbols.dtb
```

Inspect the generated `__symbols__` node and compare it with the blob built without `-@`. Explain why overlays targeting labels need symbol information in the base blob.

## Troubleshooting

### `dtc: command not found`

Install your distribution's Device Tree Compiler package. Do not substitute a random prebuilt binary in a production workflow without recording its version and provenance.

### `fdtget: command not found`

Your package may split `libfdt` utilities or omit them. Complete the compile/decompile steps with `dtc`, then install the matching utilities package if available.

### Warning About `clocks` Or A Missing Provider Property

Check that the provider has `#clock-cells = <0>` and that the consumer references it as `<&osc>` with no extra specifier cells.

### Source Builds In Kernel But Not Standalone

Kernel DTS files commonly rely on C preprocessing, generated include paths, and headers containing symbolic constants. Reproduce the kernel build rule or inspect its preprocessed source rather than deleting includes until standalone `dtc` accepts it.

## Completion Checklist

- [ ] I can compile and decompile a DTB.
- [ ] I can distinguish a source label from a runtime path and alias.
- [ ] I can inspect string and cell properties safely.
- [ ] I can explain why decompilation is not a source round trip.
- [ ] I can identify the flattened DTB's major blocks.
- [ ] I can demonstrate a case that compiles but is semantically wrong.
- [ ] I can record enough evidence to identify one exact build artifact.
- [ ] I understand why schema validation and runtime inspection are separate steps.

## References And Next Steps

- [Devicetree Specification](https://www.devicetree.org/specifications/)
- [Device Tree Compiler source and documentation](https://github.com/dgibson/dtc)
- [Device Tree Builds](../../build-systems/advanced/linux-kernel/device-tree-builds.md)
- [Build And Diagnostic Tools](../build-and-diagnostic-tools.md)
- [Runtime Inspection](../runtime-inspection.md)

Return to [Device Tree Foundations](../foundations.md), or continue with [Syntax, Values, And Source Composition](../syntax-values-and-source-composition.md).
