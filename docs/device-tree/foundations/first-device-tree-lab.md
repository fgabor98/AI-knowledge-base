---
status: draft
reviewed: false
domain: device-tree
difficulty: beginner
last_reviewed: null
---

# First Device Tree Lab

## Goal

Build a small standalone tree taken from the TI AM642 SK description, inspect its binary form, make controlled changes, distinguish source syntax errors from semantic errors, and then compare the trimmed tree with the real board DTB built from mainline Linux. No development board is required.

## Expected Outcome

At the end, you will have:

- compiled `am642-sk-mini.dts` into a DTB
- decompiled the DTB into a source-like representation
- inspected strings, cells, paths, and phandles
- demonstrated that comments and labels are not preserved like ordinary properties
- introduced and diagnosed both a compiler error and a semantic error
- built the real `k3-am642-sk.dtb` and located the same UART in it
- recorded evidence that identifies a specific DTB artifact

## Requirements

Install the Device Tree Compiler package supplied by your distribution, or use the `dtc`/`fdtget` binaries from a TI Processor SDK host sysroot. Confirm the required commands:

```sh
dtc --version
command -v dtc
command -v fdtdump
command -v fdtget
sha256sum --version
```

Some packages do not install every `libfdt` utility. `dtc` is required; `fdtdump` and `fdtget` are strongly recommended. Steps 10 and 11 also need a mainline Linux checkout and a host `cpp`. Work in a disposable directory, not in a production kernel tree.

## Step 1: Create The Source

Create `am642-sk-mini.dts` from the tree in [Tree Anatomy And Vocabulary](tree-anatomy-and-vocabulary.md#start-with-one-tree). It is a trimmed excerpt of `k3-am64.dtsi`, `k3-am64-main.dtsi`, and `k3-am642-sk.dts` with macros expanded.

Before compiling, predict:

- the path of the console UART
- the number of address/size pairs in its `reg`
- the path stored in the `serial2` alias and in `/chosen/stdout-path`
- which node will receive a generated phandle

## Step 2: Compile The DTB

```sh
dtc -I dts -O dtb -o am642-sk-mini.dtb am642-sk-mini.dts
```

Confirm that the output exists and capture its identity:

```sh
ls -l am642-sk-mini.dts am642-sk-mini.dtb
sha256sum am642-sk-mini.dtb
```

No compiler output usually means syntax compilation succeeded. It does not mean that the trimmed tree satisfies the real bindings: `8250_omap.yaml`, for example, requires `interrupts`, which this excerpt omits.

## Step 3: Decompile It

```sh
dtc -I dtb -O dts -o am642-sk-mini.decompiled.dts am642-sk-mini.dtb
sed -n '1,200p' am642-sk-mini.decompiled.dts
```

Look for these transformations:

- source comments such as `/* First peripheral window */` are absent
- cell values are printed in hexadecimal, so `48000000` becomes `0x2dc6c00`
- the string list prints as `"ti,am64-uart\0ti,am654-uart"`
- `&main_uart0` in `/aliases` and `/chosen` became the path string `/bus@f4000/serial@2800000`
- `<&k3_clks 146 0>` became `<0x01 0x92 0x00>`
- the `clock-controller` node gained `phandle = <0x01>`

Do not treat the decompiled file as the maintainable original source. It is evidence of what the DTB contains.

## Step 4: Inspect The Flattened Layout

```sh
fdtdump am642-sk-mini.dtb | sed -n '1,20p'
```

Find the header fields—`magic`, `totalsize`, `off_dt_struct`, `off_dt_strings`, `off_mem_rsvmap`, `version`—and relate them to the header, memory reservation block, structure block, and strings block.

Then view the first bytes without parsing the whole blob manually:

```sh
od -Ax -tx1z -N 64 am642-sk-mini.dtb
```

The first four bytes are the FDT magic `d0 0d fe ed` in big-endian order. The rest of the header is also big-endian; raw host-order interpretation is unsafe.

## Step 5: Query Individual Properties

```sh
fdtget am642-sk-mini.dtb / model
fdtget am642-sk-mini.dtb / compatible
fdtget am642-sk-mini.dtb /aliases serial2
fdtget am642-sk-mini.dtb /chosen stdout-path

fdtget -tx am642-sk-mini.dtb /bus@f4000/serial@2800000 reg
fdtget -tu am642-sk-mini.dtb /bus@f4000/serial@2800000 clock-frequency
fdtget -tx am642-sk-mini.dtb /bus@f4000/serial@2800000 clocks

fdtget -l am642-sk-mini.dtb /bus@f4000
fdtget -p am642-sk-mini.dtb /bus@f4000/serial@2800000
```

`clocks` prints `1 92 0`: phandle 1, TI-SCI device `0x92` (146), clock 0. If your `fdtget` version uses different option spelling, consult `fdtget --help`.

## Step 6: Prove Label, Path, Alias, And Phandle Roles

Record all four UART identities:

| Form | Expected value | Where it matters |
|---|---|---|
| source label | `main_uart0` | source references from `k3-am642-sk.dts` |
| node name | `serial@2800000` | child identity under `bus@f4000` |
| full path | `/bus@f4000/serial@2800000` | logical tree lookup |
| alias | `serial2` | shorthand understood by consumers |

Record the clock controller's numeric phandle and the first cell of the UART's `clocks` value. They match within this blob. In Step 11 you will see that the full board DTB assigns a different number to the same node.

## Step 7: Make A Controlled Hardware Change

Suppose a custom board uses UART1 as its console. Add the node exactly as `k3-am64-main.dtsi` defines it, inside `bus@f4000`:

```dts
main_uart1: serial@2810000 {
        compatible = "ti,am64-uart", "ti,am654-uart";
        reg = <0x00 0x02810000 0x00 0x100>;
        clock-frequency = <48000000>;
        clocks = <&k3_clks 152 0>;
        clock-names = "fclk";
        status = "okay";
};
```

Point the console at it:

```dts
chosen {
        stdout-path = &main_uart1;
};
```

Recompile to a second artifact and compare:

```sh
dtc -I dts -O dtb -o am642-uart1.dtb am642-sk-mini.dts
sha256sum am642-sk-mini.dtb am642-uart1.dtb
fdtget am642-uart1.dtb /chosen stdout-path
fdtget -tx am642-uart1.dtb /bus@f4000/serial@2810000 clocks
```

Expected result:

- the hashes differ
- `stdout-path` is now `/bus@f4000/serial@2810000`
- the new UART's clock specifier is `1 98 0` (device 152)

The source changed only label references; the compiler produced the paths and phandles.

## Step 8: Diagnose A Syntax Error

Temporarily remove the semicolon after `status = "okay"` in `serial@2800000` and compile:

```sh
dtc -I dts -O dtb -o broken.dtb am642-sk-mini.dts
```

`dtc` reports `syntax error` at the line of the following `};`, not at the line where the semicolon is missing, and exits with `FATAL ERROR: Unable to parse input tree`. Record whether `broken.dtb` was produced. Restore the semicolon.

## Step 9: Diagnose A Semantic Error

Change the UART register property to a one-cell-per-field form:

```dts
reg = <0x02800000 0x100>;
```

The parent declares two address cells and two size cells, so these two cells form only half of an address/size pair. Compile it:

```sh
dtc -I dts -O dtb -o bad-reg.dtb am642-sk-mini.dts
echo "exit status: $?"
```

`dtc` prints `Warning (reg_format): ... property has invalid length (8 bytes) (#address-cells == 2, #size-cells == 2)` plus dependent warnings, yet exits 0 and writes `bad-reg.dtb`. This is the lesson: a blob can be produced from a structurally wrong description. Restore the property.

Binding schemas catch far more than `dtc` can; that workflow is covered in [Writing And Validating Binding Schemas](../writing-and-validating-binding-schemas.md).

## Step 10: Build The Real Board DTB

From a mainline Linux checkout, the normal command is:

```sh
make ARCH=arm64 defconfig
make ARCH=arm64 ti/k3-am642-sk.dtb
```

If the full Kbuild prerequisites (for example `flex` and `bison`) are unavailable, reproduce the essential pipeline by hand:

```sh
cd linux
cpp -nostdinc -I scripts/dtc/include-prefixes -I arch/arm64/boot/dts/ti \
    -undef -D__DTS__ -x assembler-with-cpp \
    -o /tmp/k3-am642-sk.pp.dts arch/arm64/boot/dts/ti/k3-am642-sk.dts
dtc -@ -I dts -O dtb -o /tmp/k3-am642-sk.dtb /tmp/k3-am642-sk.pp.dts
```

The manual command is illustrative; Kbuild adds dependency tracking and warning flags. Record which method produced your artifact.

## Step 11: Compare Trimmed And Real Trees

```sh
fdtget /tmp/k3-am642-sk.dtb / compatible
fdtget /tmp/k3-am642-sk.dtb /chosen stdout-path
fdtget -tx /tmp/k3-am642-sk.dtb /bus@f4000/serial@2800000 clocks
fdtget -tx /tmp/k3-am642-sk.dtb /bus@f4000/serial@2800000 interrupts
fdtget -l /tmp/k3-am642-sk.dtb /bus@f4000 | head -20
```

Observe:

- the root compatible and console path are identical to the trimmed tree
- `clocks` still decodes to device 146, clock 0, but the phandle number differs because the full tree contains more nodes
- `interrupts` is `0 b2 4`: `GIC_SPI`, SPI 178, `IRQ_TYPE_LEVEL_HIGH`, decoded with the GIC's `#interrupt-cells = <3>`
- the real `bus@f4000` contains dozens of controllers that the trimmed tree omits

## Step 12: Build An Evidence Record

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
UART clock specifier (decoded):
```

A useful debugging statement is “the running tree contains X from artifact hash Y,” not merely “I changed the DTS.”

## Optional Exercise: Compile With Symbols

```sh
dtc -@ -I dts -O dtb -o am642-sk-mini-symbols.dtb am642-sk-mini.dts
fdtget -p am642-sk-mini-symbols.dtb /__symbols__
fdtget am642-sk-mini-symbols.dtb /__symbols__ main_uart0
```

Compare it with the blob built without `-@`. Explain why overlays that target `&main_uart0` need this symbol table in the base blob.

## Troubleshooting

### `dtc: command not found`

Install your distribution's Device Tree Compiler package or use the one in a TI SDK host sysroot. Do not substitute a random prebuilt binary in a production workflow without recording its version and provenance.

### `fdtget: command not found`

Your package may split `libfdt` utilities or omit them. Complete the compile/decompile steps with `dtc`, then install the matching utilities package if available.

### Real Board Source Fails With Standalone `dtc`

`k3-am642-sk.dts` relies on C preprocessing and headers such as `dt-bindings/gpio/gpio.h` and `k3-pinctrl.h`. Run the `cpp` step from Step 10 rather than deleting includes until standalone `dtc` accepts it.

## Completion Checklist

- [ ] I can compile and decompile a DTB.
- [ ] I can distinguish a source label from a runtime path and alias.
- [ ] I can inspect string and cell properties safely.
- [ ] I can explain why decompilation is not a source round trip.
- [ ] I can identify the flattened DTB's major blocks.
- [ ] I can demonstrate a case that compiles but is semantically wrong.
- [ ] I can build the real AM642 SK DTB and find the same node in it.
- [ ] I can record enough evidence to identify one exact build artifact.

## References And Next Steps

- [Devicetree Specification](https://www.devicetree.org/specifications/)
- [Device Tree Compiler source and documentation](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)
- [Device Tree Builds](../../build-systems/advanced/linux-kernel/device-tree-builds.md)
- [Build And Diagnostic Tools](../build-and-diagnostic-tools.md)
- [Runtime Inspection](../runtime-inspection.md)

Return to [Device Tree Foundations](../foundations.md), or continue with [Syntax, Values, And Source Composition](../syntax-values-and-source-composition.md).
