---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Binding Design And ABI Review Lab

This lab reviews a real binding migration from its deployed starting point to its current form: the Imagination PowerVR GPU on TI AM62x, whose compatible list changed between Linux v6.8 and v6.16 without breaking any combination of old and new DTB and kernel. You will reconstruct the original contract from git history, classify every string and property, prove the compatibility matrix, and then apply the same method to a second real case. Schema syntax is covered in the next module; here the goal is the reasoning.

## Inputs

- a mainline Linux clone with full history (`git log` must reach v6.8)
- `Documentation/devicetree/bindings/gpu/img,powervr-rogue.yaml`
- `drivers/gpu/drm/imagination/pvr_drv.c` and `pvr_power.c`
- `arch/arm64/boot/dts/ti/k3-am62-main.dtsi`

## Step 1: Recover The Deployed Contract

The GPU entered mainline in v6.8 with three commits: binding `6a85c3b14728`, driver `4babef070865`, and AM62 DTS `a5683d26e09e`.

```sh
git show 6a85c3b14728:Documentation/devicetree/bindings/gpu/img,powervr.yaml
git show 4babef070865:drivers/gpu/drm/imagination/pvr_drv.c | grep -A3 of_device_id
git show a5683d26e09e -- arch/arm64/boot/dts/ti/k3-am62-main.dtsi
```

Expected starting point:

```dts
gpu: gpu@fd00000 {
        compatible = "ti,am62-gpu", "img,img-axe";
        reg = <0x00 0x0fd00000 0x00 0x20000>;
        clocks = <&k3_clks 187 0>;
        clock-names = "core";
        interrupts = <GIC_SPI 86 IRQ_TYPE_LEVEL_HIGH>;
        power-domains = <&k3_pds 187 TI_SCI_PD_EXCLUSIVE>;
};
```

The binding allowed only this two-string form, with the comment `IMG AXE GPU model/revision is fully discoverable`, and the driver matched only `img,img-axe`. Every v6.8–v6.15 AM62x DTB carries exactly this.

## Step 2: Inventory The Hardware Facts

Read the v6.16 binding commit `2c01d9099859` (“Future-proofing enhancements”). Classify each fact by where it comes from:

| Fact | Source | Discoverable at runtime? | Where it belongs |
|---|---|---|---|
| GPU model and revision | IP core | yes, from registers | nowhere in DT |
| Rogue versus Volcanic architecture | IP core | | |
| number of power domains (one on AXE-1-16M) | IP core | | |
| number of clocks (one on AM62x) | SoC integration | | |
| which TI-SCI clock and domain IDs | SoC integration | | |
| DMA coherency | SoC integration | | |

Explain why `img,img-axe` could not carry the power-domain count: the commit calls the AXE series “primarily a marketing term,” so different AXE cores may differ.

## Step 3: Read The New Compatible List

```dts
compatible = "ti,am62-gpu", "img,img-axe-1-16m", "img,img-axe",
             "img,img-rogue";
power-domain-names = "a";
```

For each string, record which consumer matches it today, which schema rule it triggers, and why it is in that position. In particular, explain why the deprecated `img,img-axe` sits third rather than being removed, citing the binding comment “This deprecated element must be kept around to allow old kernels to work with newer dts.”

## Step 4: Prove The Four-Way Matrix

Fill in which `of_device_id` entry binds, or “none”:

| | v6.8 driver (`img,img-axe` only) | v6.16 driver (`img,img-rogue`, legacy `img,img-axe`) |
|---|---|---|
| v6.8 DTB (two strings) | | |
| v6.16 DTB (four strings) | | |

All four cells should bind. Then evaluate two counterfactuals:

1. The v6.16 DTS drops `img,img-axe`. Which cell breaks, and in which real deployment (for example, a new DTB in the boot partition with an older recovery kernel) would that appear?
2. The v6.16 driver drops its legacy `img,img-axe` entry. Which cell breaks?

## Step 5: Check Conditional Requirements

In the current binding, `power-domains` and `power-domain-names` are required only `if` the compatible contains `img,img-axe-1-16m`. The commit says the condition exists “to avoid an ABI break.” Explain which deployed DTBs an unconditional requirement would have invalidated.

Now compare the driver. `pvr_power_domains_init()` calls `of_count_phandle_with_args(..., "power-domains", ...)` and returns its error, so a node with no `power-domains` fails probe even under the deprecated two-string form, where the schema leaves the property optional. Decide whether this mismatch matters for AM62x (every DTS has the property) and how you would document or fix it for a future SoC.

## Step 6: Reconstruct The Patch Order

```sh
git log --format='%h %ad %cn %s' --date=short --author='Matt Coster' \
        --since=2025-04-01 --until=2025-05-15
```

Identify which commits went through the DRM tree and which through the TI SoC tree, and explain why the bindings come first and the DTS last. Compare with [Commit Structure](review-strategy-and-upstream-submission-order.md#commit-structure).

## Step 7: Validate

On a scratch branch:

```sh
make dt_binding_check DT_SCHEMA_FILES=gpu/img,powervr-rogue.yaml
make CHECK_DTBS=y ti/k3-am625-sk.dtb
```

Then, in a scratch copy of `k3-am62-main.dtsi`, try three variants and record the schema output for each:

1. the v6.8 two-string compatible (the deprecated alternative)
2. the four-string compatible without `power-domain-names`
3. the four-string compatible with `clocks` listing two entries

Expected: variant 2 violates the `img,img-axe-1-16m` requirement and variant 3 the `ti,am62-gpu` limit. For variant 1, note whether your `dt-schema` version reports deprecated alternatives at all.

## Step 8: Apply The Method To A Second Case

The Cadence CSI-2 receiver gained `interrupts` in its binding in April 2025 and in the AM62x DTS in August 2025 ([Describe Unsupported Resources Now](complete-bindings-and-extensible-hardware-models.md#describe-unsupported-resources-now)). Fill the same four-way matrix for “DTB with/without interrupts × driver before/after error-IRQ support,” and decide whether the binding can ever make `interrupts` required for `cdns,csi2rx`. What would a new SoC-specific compatible allow?

## Exit Review

The lab is complete when you can present:

- the v6.8 contract recovered from git, with the binding comment explaining its original choice
- the fact classification table with a destination for every fact
- the per-string explanation of the v6.16 compatible list
- the completed four-way matrix and both counterfactuals
- the conditional-requirement analysis, including the driver/schema mismatch
- the reconstructed patch order by tree
- schema results for the three variants
- the CSI-2 matrix and your decision on making `interrupts` required

## Authoritative References

- [Linux binding design guidelines](https://docs.kernel.org/devicetree/bindings/writing-bindings.html)
- [Linux submitting Devicetree patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Linux schema-writing guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/)

## Next Step

Continue with [Writing And Validating Binding Schemas](../writing-and-validating-binding-schemas.md), where binding contracts like this one are encoded as YAML and exercised with schema tooling.
