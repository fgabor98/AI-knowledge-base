---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Overlay Source, Fragments, And Target Selection

Overlay source describes changes relative to another tree. `/plugin/` tells `dtc` that unresolved references to the base are expected and must be emitted as relocation metadata instead of treated as ordinary unresolved-label errors.

## A Real Label-Targeted Overlay

`k3-am62x-sk-hdmi-audio.dtso` switches the AM62x SK's audio from the analog codec to HDMI:

```dts
/dts-v1/;
/plugin/;

&{/} {
        hdmi_audio: sound-sii9022 {
                compatible = "simple-audio-card";
                simple-audio-card,name = "AM62x-Sil9022-HDMI";
                simple-audio-card,format = "i2s";
                simple-audio-card,bitclock-master = <&hdmi_dailink_master>;
                simple-audio-card,frame-master = <&hdmi_dailink_master>;

                hdmi_dailink_master: simple-audio-card,cpu {
                        sound-dai = <&mcasp1>;
                        system-clock-direction-out;
                };

                simple-audio-card,codec {
                        sound-dai = <&sii9022>;
                };
        };
};

&mcasp1 {
        auxclk-fs-ratio = <2177>;
};

&codec_audio {
        status = "disabled";
};
```

`mcasp1`, `sii9022`, and `codec_audio` are external symbols expected from the base. `hdmi_audio` and `hdmi_dailink_master` are local to the overlay. The overlay adds a card under `/`, changes one McASP property, and disables the analog card that also uses `mcasp1`.

The source is concise, but its contract includes three base labels and what each one means: the McASP instance wired to the bridge, the SiI9022 bridge, and the analog card that must not run at the same time.

## What The Compiler Emits

Compiled with `dtc -@`, the overlay decompiles to explicit fragments (trimmed):

```dts
/ {
        fragment@0 {
                target-path = "/";
                __overlay__ {
                        sound-sii9022 {
                                simple-audio-card,bitclock-master = <0x01>;
                                /* ... */
                                simple-audio-card,cpu {
                                        sound-dai = <0xffffffff>;
                                        phandle = <0x01>;
                                };
                        };
                };
        };

        fragment@1 {
                target = <0xffffffff>;
                __overlay__ {
                        auxclk-fs-ratio = <0x881>;
                };
        };

        fragment@2 {
                target = <0xffffffff>;
                __overlay__ {
                        status = "disabled";
                };
        };

        __fixups__ {
                mcasp1 = "/fragment@0/__overlay__/sound-sii9022/simple-audio-card,cpu:sound-dai:0\0/fragment@1:target:0";
                sii9022 = "/fragment@0/__overlay__/sound-sii9022/simple-audio-card,codec:sound-dai:0";
                codec_audio = "/fragment@2:target:0";
        };
};
```

`target` names an existing base node by phandle, `target-path` by path, and `__overlay__` holds what is merged there. `&{/}` became `target-path = "/"`. Each label reference became `0xffffffff` plus a `__fixups__` entry saying where to patch it; `mcasp1` is used twice, so its entry lists two locations. You can write fragments by hand, but the shorthand is clearer. Fragment names identify overlay records, not hardware, and are not a product ABI.

## Label Targets

`&codec_audio { status = "disabled"; };` is a label target.

Advantages:

- independent of the target node's full path
- survives path changes when the label remains exported
- expresses a named base integration point
- preferred by upstream Linux overlay documentation

Requirements:

- the base source has the label
- the deployed base DTB contains a matching `__symbols__` entry
- the label continues to mean the same hardware and binding contract
- any providers referenced by the overlay also export required symbols

A source label normally disappears from an ordinary DTB. It becomes an externally usable symbol only when the base is built with symbol generation.

## Path Targets

Path-reference shorthand, as in the HDMI audio overlay:

```dts
&{/} {
        /* ... */
};
```

Explicit equivalent, as the compiler emitted it:

```dts
fragment@0 {
        target-path = "/";

        __overlay__ {
                /* ... */
        };
};
```

Path targeting can work without an exported base label for that target. It is more tightly coupled to hierarchy, bus naming, unit addresses, and source refactoring. A stable path is still a compatibility promise.

Path targeting does not eliminate all symbol requirements. The card added under `&{/}` still refers to `&mcasp1` and `&sii9022` inside its properties.

## Target The Owning Node

Choose the node whose binding owns the change:

- add an SPI peripheral under the SPI controller
- add a pin group under the pin controller
- add a fixed regulator under the binding-defined parent or root
- attach an endpoint under the correct `port`
- set a board-specific property on the actual consumer

Do not target `/` merely because it is easy, then recreate paths or duplicate existing hardware. A new child with a slightly different name may coexist with the original and cause duplicate devices rather than replacing it.

## Multiple Targets

A module usually needs several fragments. The AM62x SK OV5640 overlay (`k3-am62x-sk-csi2-ov5640.dtso`) has five targets:

```text
&{/}            fixed clock and three fixed regulators for the camera module
&main_i2c2      PCA9543 mux and the sensor on channel 1
&cdns_csi2rx0   receiver port@0 endpoint, status okay
&ti_csi2rx0     status okay
&dphy0          status okay
```

The sensor's `powerdown-gpios` also references `&exp1`, and its regulators reference `&vcc_3v3_sys`, so its `__fixups__` lists six base symbols: `vcc_3v3_sys`, `main_i2c2`, `exp1`, `cdns_csi2rx0`, `ti_csi2rx0`, and `dphy0`. Its own labels (`clk_ov5640_fixed`, `reg_2p8v`, `csi2_cam0`, and others) are local, and the compiler must keep both kinds so the resolver can relocate local references and resolve base ones.

Source order can matter when several fragments write the same target/property, but relying on intra-overlay last-writer behavior obscures ownership. Prefer one fragment per target or non-overlapping changes.

## Properties Are Replaced, Nodes Are Merged

When an overlay supplies a property already present on the target, the merged value replaces the prior value for that applied changeset. When it supplies a child node with the same full name, content merges into that node; a differently named child is an additional node.

Before writing an overlay, record the expected prior state:

```text
target: /bus@f0000/ticsi2rx@30102000
prior status: disabled
prior child csi-bridge@30101000/ports/port@0/endpoint: absent
overlay postcondition: status okay, exactly one endpoint, linked to camera@3c
```

This turns accidental double application or a changed base into a detectable precondition failure.

## Deletion Is Not A Portable Overlay Primitive

`/delete-node/` and `/delete-property/` are source-composition directives. A normal flattened DTB does not contain a generic “delete this base item later” tombstone. Do not assume that placing source deletion syntax in an overlay creates a portable runtime deletion operation.

The AM642 EVM's `k3-am642-evm-pcie0-ep.dtso` shows the portable alternative. To turn the PCIe root complex into an endpoint, it does not delete `pcie@f102000`. It sets `&pcie0_rc { status = "disabled"; };` and adds a new `pcie-ep@f102000` node under `&cbass_main` at the same register address. Because the overlay compiler cannot see the parent's cell sizes, the fragment restates `#address-cells = <2>;` and `#size-cells = <2>;` so that the new `reg` is encoded correctly; applying it rewrites those parent properties with the values they already have.

Some ecosystems add tool-specific mechanisms. Treat those as explicit platform formats, not generic DT overlay behavior. For portable designs, prefer positive descriptions, separate base DTBs, or disabling a node when the binding and lifecycle make that safe.

Removing an entire applied live overlay is different: the overlay framework reverts the changeset it recorded. That is not an arbitrary delete operation against the base.

## Source Review Checklist

- Is `/plugin/` present?
- Does each target identify the true owner of the change?
- Are all base labels/path targets declared compatibility interfaces?
- Are local labels distinguished from external labels?
- Are provider specifiers valid for the selected base providers?
- Does each expected prior state have a postcondition?
- Could a child name create a duplicate rather than update the intended node?
- Are multiple fragments non-conflicting?
- Is any supposed deletion actually supported by the selected toolchain?
- Does the overlay describe optional physical hardware rather than product policy?

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)
- [Devicetree compiler project](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/)

## Continue

Proceed to [Symbols, Fixups, Local Fixups, And Compilation](symbols-fixups-local-fixups-and-compilation.md).
