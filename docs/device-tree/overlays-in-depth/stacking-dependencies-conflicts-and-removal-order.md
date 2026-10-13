---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Stacking, Dependencies, Conflicts, And Removal Order

Multiple overlays form an ordered mutation graph. A stack is valid only when all symbol dependencies, target preconditions, hardware resources, and lifecycle dependencies agree. Application success is not conflict detection.

## Model Four Relations

```text
requires(A, B)   A needs nodes/symbols/resources introduced by B
before(A, B)     both can apply, but A must precede B
conflicts(A, B)  no supported final tree contains both
owns(A, R)       A has exclusive authority over resource/property R
```

Examples:

Mainline TI sources contain all four:

```text
k3-am62x-sk-csi2-ov5640 conflicts with k3-am62x-sk-csi2-imx219     (same csi2rx0 endpoint)
k3-am62x-sk-hdmi-audio owns mcasp1, and disables codec_audio       (one McASP, one card)
k3-am654-icssg2 before k3-am654-idk                                 (alias numbering, below)
a sensor overlay requires a module-regulator overlay                (pattern; no mainline TI case)
```

Do not infer these relations solely from filenames or directory order.

## Structural Stack Dependency

Overlay A can introduce and export a label, and overlay B can consume it. The HDMI audio overlay exports `hdmi_audio` and `hdmi_dailink_master`; a hypothetical follow-on overlay could then write:

```dts
&hdmi_audio {
        simple-audio-card,name = "Product-HDMI";
};
```

B's external fixup can resolve only after A has applied and exported `module_reg` into the target symbol namespace. Therefore:

```text
apply:  A -> B
remove: B -> A
```

Linux overlay removal will not permit removing an overlay that is stacked beneath another dependent overlay. Product tooling should reject the invalid request before invoking the core.

## Semantic Dependency Without A Symbol

Some requirements are invisible to the resolver. The AM654 IDK DTB is built in Kbuild as a stack:

```make
k3-am654-evm-dtbs := k3-am654-base-board.dtb k3-am654-icssg2.dtbo
k3-am654-idk-dtbs := k3-am654-evm.dtb k3-am654-idk.dtbo k3-am654-pcie-usb2.dtbo
```

Compiling the three overlays and checking their `__fixups__` against the base board's `__symbols__` shows no overlay uses a symbol from another, so the resolver sees no ordering. Their `aliases` still form a contract: the base owns `ethernet0`, `k3-am654-icssg2.dtso` sets `ethernet1` and `ethernet2`, and `k3-am654-idk.dtso` sets `ethernet3` through `ethernet6`. Give both overlays the same alias number and every layer still applies, but the later one silently wins. U-Boot's `fdt_fixup_ethernet()` then writes `eth1addr` and friends to different ports than intended, because it maps `ethernetN` aliases to `ethNaddr` variables.

Other common invisible requirements:

- a pinmux overlay must apply before a device is enabled
- a power overlay raises a voltage required by a module
- one overlay reserves a chip select that another assumes is free
- firmware must be loaded before an exposed remote core starts
- an overlay changes an alias consumed by boot policy

Encode these in the manifest. The absence of a phandle edge does not mean independence.

## Property Collision

Two overlays can target the same property:

```text
overlay display: pinctrl-0 = <display_pins>
overlay sensor:  pinctrl-0 = <sensor_pins>
```

Possible policies:

- conflict: reject both together
- ordered override: one declared layer owns the final property
- merge redesign: describe independent pin groups through a binding-supported structure
- separate base/product: stop composing this combination dynamically

Do not accept accidental last-writer-wins behavior. Record the expected prior value before each authorized replacement.

## Hardware Conflict Inventory

Check at least:

- pin functions and GPIO lines
- I2C addresses, SPI chip selects, PCI functions, and aliases
- clock parents/rates/exclusive users
- regulator voltage/current/load constraints
- resets and power domains
- interrupt lines and trigger types
- DMA channels and IOMMU stream IDs
- MMIO and reserved-memory ranges
- connector lanes and graph endpoints
- wakeup ownership and suspend states
- secure-world/firmware-owned resources

Schema validation catches many shapes but does not solve all global exclusivity or electrical constraints.

## Canonical Application Order

Given an acyclic dependency graph:

1. validate every selected overlay and manifest entry
2. add implicit prerequisites
3. reject conflicts and duplicate stable IDs
4. topologically sort hard requirements
5. apply declared layer priorities only where the graph leaves a choice
6. produce one canonical ordered list
7. log IDs, versions, hashes, and order

If several valid topological orders produce different final trees, the ownership model is incomplete. Add an explicit order or remove the collision.

## Removal Order

Removal is reverse dependency order, not arbitrary user choice:

```text
apply:  base -> regulator -> sensor -> policy
remove: policy -> sensor -> regulator
```

Before removing A, determine:

- later overlays structurally stacked on A
- semantic consumers of resources introduced by A
- devices populated from A's nodes
- userspace handles and subsystem links
- references cached by drivers or notifiers
- asynchronous activity still targeting A's devices/data

The kernel's stack check is necessary but does not know every semantic consumer.

## Partial Failure Policy

For boot-time flat-blob composition, apply to a disposable working copy and discard it on any failure. For Linux live overlays, the API can return after partial side effects in some error paths; retain the returned overlay changeset identifier as documented and execute the required cleanup path.

Never continue by applying “the rest” of a product stack after a mandatory overlay fails. Recompute the allowed configuration or enter a defined recovery state from a known tree.

## Idempotence

Overlay application is not generally idempotent:

- the same child may merge twice or create duplicate state
- local phandles are relocated anew
- notifiers and device population can repeat side effects
- property prior-state assumptions no longer hold
- stack records and removal cookies are distinct

Reject duplicate stable IDs. A retry must start from a pristine base or explicitly remove/verify the previous application according to the lifecycle contract.

## Combination Explosion

For N apparently independent overlays there can be up to `2^N` selections and many orders. Reduce the state space by design:

- model only physically supported modules
- declare conflicts and prerequisites
- group factory-fixed combinations into base DTBs
- publish a small number of canonical profiles
- make order deterministic
- avoid overlays that merely tune policy

Then test every supported composition plus representative rejected combinations. If the supported graph remains unbounded, overlays are the wrong product abstraction.

## Stack Ledger

At runtime or boot, preserve:

| Sequence | Cookie/handle | Stable ID | Hash | Requires | Owns | Removable? |
|---:|---|---|---|---|---|---|
| 1 | 41 | module-power-v2 | ... | base-v3 | regulator-module | yes after dependents |
| 2 | 42 | temp-sensor-v4 | ... | module-power-v2 | SPI CS0 | boot-only |

Linux kernel cookies are runtime handles, not persistent product IDs. Map both in logs.

## Test The Negative Graph

Missing prerequisites, cycles, conflicts in both directions, duplicates, wrong order, removal of a lower stacked overlay, and mid-sequence failures must all be rejected safely; see [Validate Rejections](validation-security-and-product-architecture.md#validate-rejections). The safe rejection path is part of the product behavior.

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Changesets](https://docs.kernel.org/devicetree/changesets.html)
- [U-Boot Device Tree Overlays](https://docs.u-boot.org/en/latest/usage/fdt_overlays.html)

## Continue

Proceed to [Linux Runtime Overlays, Devices, Notifiers, And Lifetime](linux-runtime-overlays-devices-notifiers-and-lifetime.md).
