---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Source Composition Lab

## Goal

Build the three SolidRun AM642 HummingBoard-T DTBs from mainline Linux, then use them to trace includes, preprocessing, amendments across four layers, deletions, value encodings, and the semantic difference between variants that share one system-on-module.

## Sources

All files are in `arch/arm64/boot/dts/ti/` of a mainline Linux checkout:

```text
k3-am64.dtsi, k3-am64-main.dtsi, k3-am64-mcu.dtsi   SoC (TI)
k3-am642.dtsi                                       SoC variant (TI)
k3-am642-sr-som.dtsi                                SolidRun SoM
k3-am642-hummingboard-t.dts                         carrier, SERDES lane unused
k3-am642-hummingboard-t-pcie.dts                    includes the carrier DTS, lane -> PCIe
k3-am642-hummingboard-t-usb3.dts                    includes the carrier DTS, lane -> USB3
k3-pinctrl.h, k3-serdes.h                           TI macro headers
```

The board's single SERDES lane can serve either an M.2 PCIe slot or USB 3, selected by an on-board GPIO mux. The two variant DTS files describe the two assembly/usage choices.

## Requirements

- a mainline Linux checkout (the examples were verified against v6.19-rc1)
- `dtc`, `fdtget`, and a host C preprocessor (`cpp` or `gcc -E`)
- a disposable output directory; do not edit the checkout

## Step 1: Map The Include Graph

```sh
cd arch/arm64/boot/dts/ti
grep -n '#include' k3-am642-hummingboard-t-usb3.dts k3-am642-hummingboard-t.dts \
    k3-am642.dtsi k3-am64.dtsi k3-am642-sr-som.dtsi
```

Draw the graph. Expected shape:

```text
k3-am642-hummingboard-t-usb3.dts
├── k3-am642-hummingboard-t.dts
│   ├── <dt-bindings/leds/common.h>, <dt-bindings/phy/phy.h>
│   ├── k3-am642.dtsi
│   │   └── k3-am64.dtsi
│   │       ├── <dt-bindings/...>, k3-pinctrl.h
│   │       ├── k3-am64-thermal.dtsi
│   │       ├── k3-am64-main.dtsi
│   │       └── k3-am64-mcu.dtsi
│   └── k3-am642-sr-som.dtsi
│       └── k3-am64-ti-ipc-firmware.dtsi
└── k3-serdes.h
```

Note that the variant includes a complete `.dts` file rather than a `.dtsi`. That is legal—`/dts-v1/;` appears once in the combined unit—but it means the carrier DTS is both a buildable board and a reusable layer.

## Step 2: Build All Three DTBs

With working Kbuild prerequisites:

```sh
make ARCH=arm64 defconfig
make ARCH=arm64 ti/k3-am642-hummingboard-t.dtb \
                ti/k3-am642-hummingboard-t-pcie.dtb \
                ti/k3-am642-hummingboard-t-usb3.dtb
```

Otherwise reproduce the essential `cmd_dtc` rule from `scripts/Makefile.dtbs` from the top of the checkout:

```sh
out=/tmp/hb
mkdir -p "$out"
for v in "" -pcie -usb3; do
        n=k3-am642-hummingboard-t$v
        cpp -nostdinc -I scripts/dtc/include-prefixes -undef -D__DTS__ \
            -x assembler-with-cpp -o "$out/$n.pp.dts" arch/arm64/boot/dts/ti/$n.dts
        dtc -I dts -O dtb -b 0 -o "$out/$n.dtb" "$out/$n.pp.dts"
done
sha256sum "$out"/*.dtb
```

Record which method produced each artifact and the `dtc` version.

## Step 3: Inspect Preprocessing

Open `k3-am642-hummingboard-t-usb3.pp.dts` and find:

- the expansion of `idle-states = <AM64_SERDES0_LANE0_USB>;` (it is `<0x1>` after `k3-serdes.h`)
- any `pinctrl-single,pins` entry, where each `AM64X_IOPAD(...)` has become two cell expressions
- the `#` line markers that point back to the original file and line

```sh
grep -n 'idle-states' /tmp/hb/k3-am642-hummingboard-t-usb3.pp.dts
grep -n -m3 'pinctrl-single,pins' -A2 /tmp/hb/k3-am642-hummingboard-t-usb3.pp.dts
```

Explain why reviewers read the maintained source but debug from the preprocessed file.

## Step 4: Reconstruct One Node Across Four Layers

The USB controller node `usb0` is defined in the SoC file and amended three more times. Before querying the DTBs, fill in the predicted final values from the sources:

| Layer | File | `dr_mode` | `maximum-speed` | other changes |
|---|---|---|---|---|
| SoC | `k3-am64-main.dtsi` | `otg` | `super-speed` | `reg`, interrupts |
| SoM | `k3-am642-sr-som.dtsi` | `otg` | `high-speed` | pinctrl; `usbss0` gets `ti,usb2-only` |
| carrier | `k3-am642-hummingboard-t.dts` | `host` | — | |
| USB3 variant | `k3-am642-hummingboard-t-usb3.dts` | — | `super-speed` | `phys`, `phy-names`; deletes `ti,usb2-only` |

Now verify:

```sh
U=/bus@f4000/cdns-usb@f900000/usb@f400000
for v in "" -pcie -usb3; do
        d=/tmp/hb/k3-am642-hummingboard-t$v.dtb
        echo "$v: $(fdtget $d $U dr_mode) $(fdtget $d $U maximum-speed)"
        fdtget -p $d /bus@f4000/cdns-usb@f900000 | grep usb2-only || echo "  no ti,usb2-only"
done
```

Expected: all three are `host`; the base and PCIe variants are `high-speed` with `ti,usb2-only`; only the USB3 variant is `super-speed`, has `phys`, and lacks `ti,usb2-only`.

## Step 5: Read The Lane Selection

```sh
for v in "" -pcie -usb3; do
        d=/tmp/hb/k3-am642-hummingboard-t$v.dtb
        echo "$v: board mux idle-state=$(fdtget -tu $d /mux-controller idle-state)" \
             "SoC lane mux=$(fdtget -tx $d /bus@f4000/syscon@43000000/mux-controller@4080 idle-states 2>/dev/null)" \
             "pcie=$(fdtget $d /bus@f4000/pcie@f102000 status)"
done
```

Expected: the base board leaves the GPIO mux disabled (`2`) and PCIe disabled; the PCIe variant selects `1` and lane value `0` (`AM64_SERDES0_LANE0_PCIE0`) and enables PCIe; the USB3 variant selects `0` and lane value `1` (`AM64_SERDES0_LANE0_USB`).

## Step 6: Identify Encodings

From the USB3 DTB, query and classify each value:

```sh
d=/tmp/hb/k3-am642-hummingboard-t-usb3.dtb
fdtget $d / compatible                                    # string list
fdtget -tx $d /bus@f4000/bus@fc00000/spi@fc40000 reg      # cells, 64-bit address and size
fdtget -tbx $d /bus@f4000/ethernet@8000000/ethernet-ports/port@1 mac-address  # bytes
fdtget -p $d /bus@f4000/cdns-usb@f900000                 # ti,vbus-divider is a boolean
```

Decode the OSPI `reg`: the second entry `0x5 0x0 0x1 0x0` is a 4 GiB window at `0x5_0000_0000`. Explain why `fdtget -tx` cannot show where one `reg` entry ends and the next begins.

## Step 7: Replace Versus Delete

In a scratch copy of `k3-am642-hummingboard-t-usb3.dts`, replace

```dts
/delete-property/ ti,usb2-only;
```

with

```dts
ti,usb2-only;
```

Rebuild and prove that the property still exists. Restore the deletion and explain why a zero-length property represents presence, not false.

## Step 8: Break Include Resolution

Rerun the `cpp` command from Step 2 without `-I scripts/dtc/include-prefixes`:

```text
k3-am642-hummingboard-t.dts:12:37: error: no include path in which to search for dt-bindings/leds/common.h
```

Identify which processor reported the error and explain why adding `dtc -i scripts/dtc/include-prefixes` would not fix a `#include` failure.

## Step 9: Reveal An Ordering Hazard

Append a further amendment to a scratch copy of the USB3 variant, after its existing `&usb0` block:

```dts
&usb0 {
        maximum-speed = "high-speed";
};
```

Rebuild and observe that the later amendment wins silently. Then explain why the real sources avoid this: each layer amends only facts it owns.

## Step 10: Diff Variants Semantically

```sh
dtc -s -I dtb -O dts -o /tmp/hb/base.sorted.dts /tmp/hb/k3-am642-hummingboard-t.dtb
dtc -s -I dtb -O dts -o /tmp/hb/usb3.sorted.dts /tmp/hb/k3-am642-hummingboard-t-usb3.dtb
diff -u /tmp/hb/base.sorted.dts /tmp/hb/usb3.sorted.dts | less
```

Most of the diff is phandle renumbering: adding one `serdes0_link` node shifts every later phandle, so unrelated `pinctrl-0`, `clocks`, and `interrupt-parent` values change. Separate that noise from the real deltas: `model`, `ti,usb2-only`, `maximum-speed`, `phys`, `phy-names`, the SERDES PHY child, and the mux states.

## Step 11: Review The Architecture

Answer:

1. Which layer owns the DDR size, the eMMC, and the remote-core memory carveouts, and how can you tell from the include graph?
2. `ti,usb2-only` is set in the SoM file but removed by a carrier variant. Is that a SoM fact or a carrier fact? What would a cleaner split look like?
3. Why are PCIe and USB3 separate DTS files rather than one DTS with an overlay? What would change if the M.2 module were field-replaceable?
4. Which files must change if the USB controller's register window were wrong for every AM64x board?
5. Why is the root `compatible` of the base board still correct in the PCIe and USB3 variants, which change only `model`?

## Failure Investigation Checklist

If a build fails:

1. identify whether `cpp` or `dtc` failed
2. inspect the exact preprocessed file and its line markers
3. verify include paths and filenames
4. check that `/dts-v1/;` appears exactly once in the final compilation unit
5. locate every definition and amendment of the failing node
6. simplify the command without changing processors
7. compare with the Kbuild rule you intend to reproduce

## Completion Checklist

- [ ] I built three DTBs from shared physical layers.
- [ ] I inspected macro expansion before compilation.
- [ ] I reconstructed one node across four layers and verified it in the DTBs.
- [ ] I distinguished string lists, cells, 64-bit values, byte arrays, and booleans.
- [ ] I demonstrated replacement and deletion behavior.
- [ ] I explained why CPP and `dtc` include paths are different.
- [ ] I separated phandle-renumbering noise from real variant deltas.
- [ ] I can justify which hardware layer owns each property.

## References And Next Steps

- [Devicetree source format](https://devicetree-specification.readthedocs.io/en/stable/source-language.html)
- [Linux DTS coding style](https://docs.kernel.org/devicetree/bindings/dts-coding-style.html)
- [Device Tree Build And Validation](../../build-systems/advanced/device-tree-build-and-validation.md)

Return to [Syntax, Values, And Source Composition](../syntax-values-and-source-composition.md), or continue with [Provider-Consumer Relationships](../provider-consumer-relationships.md).
