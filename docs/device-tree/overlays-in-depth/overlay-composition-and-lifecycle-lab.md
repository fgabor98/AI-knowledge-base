---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Overlay Composition And Lifecycle Lab

This lab composes real mainline overlays for the TI AM62x SK: `k3-am62x-sk-hdmi-audio.dtso`, `k3-am62x-sk-csi2-ov5640.dtso`, and `k3-am62x-sk-csi2-imx219.dtso` on `k3-am625-sk.dtb`. You will audit each overlay's base contract from its compiled metadata, predict and verify merged trees, and reproduce five failure modes with their real tool output. Every expected value below comes from mainline v6.19-rc1 built with `dtc` and `fdtoverlay` from DTC v1.7.0.

## Inputs

- `arch/arm64/boot/dts/ti/k3-am625-sk.dts` and the three `.dtso` files
- `arch/arm64/boot/dts/ti/Makefile` (the `*-dtbs` compositions)
- `dtc`, `fdtoverlay`, `fdtget`; optionally `dt-validate`
- for Step 8, an AM62x SK with U-Boot

Build each source through `cpp` as in the [First Device Tree Lab](../foundations/first-device-tree-lab.md#step-10-build-the-real-board-dtb), adding `-I arch/arm64/boot/dts/ti` for `k3-pinctrl.h`, then:

```sh
dtc -@ -I dts -O dtb -o k3-am625-sk.dtb k3-am625-sk.pp.dts
dtc -I dts -O dtb -o nosym.dtb k3-am625-sk.pp.dts
for o in hdmi-audio csi2-ov5640 csi2-imx219; do
        dtc -@ -I dts -O dtb -o $o.dtbo k3-am62x-sk-$o.pp.dts
done
```

## Step 1: Measure What `-@` Adds

```sh
stat -c '%s %n' nosym.dtb k3-am625-sk.dtb
fdtget -p k3-am625-sk.dtb /__symbols__ | wc -l
```

Expected: 46,668 bytes without symbols and 61,786 with them. Kbuild adds `-@` only to bases listed in a `*-dtbs` composition (`scripts/Makefile.dtbs`: “Set -@ if the target is a base DTB that overlay is applied onto”). Find the composition lines for `k3-am625-sk` in the TI `Makefile` and explain why a base without one would not support these overlays at all.

## Step 2: Extract Each Overlay's Base Contract

```sh
for o in hdmi-audio csi2-ov5640 csi2-imx219; do
        echo "$o needs: $(fdtget -p $o.dtbo /__fixups__ | tr '\n' ' ')"
        echo "$o exports: $(fdtget -p $o.dtbo /__symbols__ | tr '\n' ' ')"
done
```

Expected, for the first two:

```text
hdmi-audio needs: mcasp1 sii9022 codec_audio
csi2-ov5640 needs: vcc_3v3_sys main_i2c2 exp1 cdns_csi2rx0 ti_csi2rx0 dphy0
```

Check every needed symbol against the base, and record the path it resolves to:

```sh
for sym in $(fdtget -p hdmi-audio.dtbo /__fixups__); do
        echo "$sym -> $(fdtget k3-am625-sk.dtb /__symbols__ $sym)"
done
```

Then write the semantic contract that the paths do not show, for example: “`mcasp1` is the McASP whose I2S lines reach the SiI9022” and “`codec_audio` is the analog card that also uses `mcasp1`.”

## Step 3: Predict, Then Verify, One Overlay

Before applying the HDMI audio overlay, predict:

1. the path of the new card
2. the `status` of `/sound` afterwards
3. the phandles of the new card and its `simple-audio-card,cpu` child, given that the base's highest phandle is `0xd6`
4. the value of `sound-dai` in that child

```sh
fdtoverlay -i k3-am625-sk.dtb -o hdmi.dtb hdmi-audio.dtbo
fdtget hdmi.dtb /sound status
fdtget -tx hdmi.dtb /sound-sii9022 phandle
fdtget -tx hdmi.dtb /sound-sii9022/simple-audio-card,cpu phandle sound-dai
fdtget hdmi.dtb /__symbols__ hdmi_audio
```

Expected: `/sound-sii9022`; `disabled`; `0xd8` and `0xd7` (local phandles 2 and 1 plus `0xd6`); `0x4a`, the phandle of `mcasp1`. The merged tree also gains `hdmi_audio = "/sound-sii9022"`.

## Step 4: Compose A Supported Stack

```sh
fdtoverlay -i k3-am625-sk.dtb -o stack.dtb hdmi-audio.dtbo csi2-ov5640.dtbo
dtc -I dtb -O dts stack.dtb >/dev/null
dt-validate -s processed-schema.json stack.dtb
```

The two overlays touch disjoint targets, so order does not matter for the final tree. Prove it by applying them in the opposite order and comparing decompiled output. Expect two kinds of difference: the order of the new root-level nodes, and the numeric phandles assigned to overlay-local nodes. Explain why neither changes the hardware description.

## Step 5: Diagnose Deliberate Faults

### Fault A: The Base Has No Symbols

```sh
fdtoverlay -i nosym.dtb -o a.dtb hdmi-audio.dtbo
```

```text
Failed to apply 'k3-am62x-sk-hdmi-audio.dtbo': FDT_ERR_NOTFOUND
```

### Fault B: The Wrong Base

```sh
fdtoverlay -i k3-am642-sk.dtb -o b.dtb hdmi-audio.dtbo
```

The output is identical to Fault A. The AM64x SK has symbols but no `sii9022` or `codec_audio`. Explain why the same message has two different causes, and write the two-command check (`fdtget -p base.dtb /__symbols__` and the Step 2 loop) that tells them apart.

### Fault C: Two Cameras

```sh
fdtoverlay -i k3-am625-sk.dtb -o c.dtb csi2-ov5640.dtbo csi2-imx219.dtbo
dtc -I dtb -O dts c.dtb 2>&1 >/dev/null | grep graph
```

`fdtoverlay` succeeds. Both sensors now sit on mux channel 1 (`camera@3c` and `camera@10`), the receiver endpoint points at the IMX219, and `dtc` reports that the OV5640 endpoint's `remote-endpoint` is `graph phandle is not valid`. Explain why the overlay format cannot express “these two conflict” and where that rule must live.

### Fault D: Applied Twice

```sh
fdtoverlay -i k3-am625-sk.dtb -o d.dtb hdmi-audio.dtbo hdmi-audio.dtbo
diff <(dtc -I dtb -O dts hdmi.dtb) <(dtc -I dtb -O dts d.dtb)
```

It succeeds, and the only differences are renumbered phandles (`0xd9`/`0xda` instead of `0xd7`/`0xd8`), consistently updated in every reference. Explain why this overlay happens to be safe to apply twice, and construct a change to it that would not be (hint: append to a list property, or add a child with a different unit address).

### Fault E: A Renamed Base Label

In a scratch copy of `k3-am62x-sk-common.dtsi`, rename the label `codec_audio` to `analog_audio`, rebuild the base with `-@`, and apply the unchanged overlay. Predict the result from Fault A, then explain why a label rename is an ABI break for every shipped DTBO even though the base DTB is otherwise byte-for-byte equivalent at runtime.

## Step 6: Write The Product Manifest

Using [Compatibility Identifiers](base-compatibility-versioning-and-overlay-abi.md#compatibility-identifiers) as a model, write a manifest for these three overlays that records, for each: the bases it supports (all three AM62x-family SKs for HDMI audio, per the `Makefile`), the symbols it requires, the conflicts (the two camera overlays), the resources it owns (`mcasp1`), and whether it may be applied at runtime or only at boot.

## Step 7: Validate The Final Trees

For each supported combination from Step 6, build the merged DTB and run `dtc` and `dt-validate`. Record which tool would have caught each fault in Step 5; for several, the answer is “none.”

## Step 8: Apply At Boot

On the SK, apply the HDMI audio overlay in U-Boot, either with `fdt apply` on the working FDT or through an extlinux `fdtoverlays` line. Confirm in Linux:

```sh
cat /proc/asound/cards
cat /sys/firmware/devicetree/base/sound/status
ls /sys/firmware/devicetree/base/ | grep sound
```

Expected: one card named `AM62x-Sil9022-HDMI`, `/sound` disabled, and both `sound` and `sound-sii9022` present in the live tree.

## Exit Review

The lab is complete when you can present:

- the `-@` size comparison and the Kbuild rule that controls it
- the needed/exported symbol table for all three overlays, with semantic notes
- verified predictions for Step 3, including the phandle arithmetic
- the order-independence proof for the supported stack
- tool output and root cause for each fault, including the two identical messages with different causes
- the product manifest and validation results
- boot-time evidence from the board

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [Linux Devicetree Changesets](https://docs.kernel.org/devicetree/changesets.html)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)

## Next Step

Continue with [U-Boot And Bootloader Device Tree](../u-boot-and-bootloader-device-tree.md), where overlays are applied to the bootloader's working FDT before Linux starts.
