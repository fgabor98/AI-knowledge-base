---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Upstream Development And Downstream Patch-Stack Discipline

Downstream work is sometimes necessary for product schedules, unreleased hardware, or integration constraints. It becomes dangerous when it is an unowned alternate history. Every carried DT change needs provenance, upstream disposition, rebase evidence, and an exit plan.

## Choose The Upstream-First Shape

Design changes against current upstream contracts even when product integration starts on an older branch:

- describe hardware, not the current downstream driver workaround
- use documented compatibles and common properties
- add a schema before using a new compatible
- preserve old binding behavior where required
- separate binding, driver, DTS, and product policy
- keep board DTS changes in the platform/SoC path
- avoid dependencies on downstream-only labels or file layout when possible

An upstreamable design reduces later translation even if exact patches need backport adaptation.

## Understand Patch Routing

Linux DT work can cross maintainership domains:

```text
binding/schema -> DT + subsystem review, often travels with driver
driver          -> subsystem maintainer tree
DTS/DTSI        -> platform/SoC maintainer tree
dt-bindings header -> coordinated with binding/driver consumers
```

Current upstream guidance expects compatible strings to be documented before DTS use and DTS patches to be separate from driver patches because they normally travel through different trees. Query current `MAINTAINERS` and `scripts/get_maintainer.pl`; do not reuse an old mailing list from memory.

## Build A Bisectable Series

Example dependency plan:

```text
Series A (binding + driver tree)
  1/3 dt-bindings: document acme,axc-capture-v4
  2/3 media: axc: accept v4 compatible, preserve v3 fallback
  3/3 media: axc: use optional calibration supply

Series B (SoC tree; references Series A discussion)
  1/2 arm64: dts: acme: add capture block to ax9
  2/2 arm64: dts: acme: enable capture on axc200 revD
```

No intermediate commit should break builds or existing boards. If DTS can merge before driver support because it uses a fallback compatible, explain that property. If not, state the dependency and expected merge strategy.

## Maintain A Downstream Patch Ledger

For every patch record:

```yaml
patch_id: DT-0042
subject: "arm64: dts: acme: correct revC capture stream ID"
owner: product-platform
reason: field correctness fix
origin: downstream
upstream_status: submitted-v2
upstream_link: https://lore.kernel.org/...
first_release: product-6.6.31-r7
affected_products: [axc200-revC, axc210-revA]
binding_impact: none
dependencies: []
conflict_notes: "upstream file renamed after v6.10"
validation: [dtbs_check, semantic-diff, capture-dma-hw]
drop_when: "upstream commit <full-id> is in merge base"
```

The ledger should be generated or checked against the actual patch series so it cannot silently drift.

## Classify Carried Patches

- **backport:** upstream commit adapted to older release
- **pending upstream:** submitted or accepted but not in base
- **product-only:** legitimate product integration not appropriate upstream
- **temporary workaround:** bounded defect workaround with removal trigger
- **rejected redesign required:** upstream feedback invalidated current approach
- **legacy debt:** no longer justified; prioritize elimination

“Vendor patch” is not a useful lifecycle state.

## Rebase With Intent

For each upstream/base update:

1. Freeze old base, patch stack, artifact manifest, and validation results.
2. Identify patches now upstream and drop them by upstream commit identity.
3. Reapply remaining logical changes, reading every conflict.
4. Re-run schema/build checks and compare all generated DT artifacts.
5. Explain unexpected semantic drift.
6. Run impact-derived hardware coverage.
7. Update conflict notes and upstream status.

Never resolve a DTS conflict by choosing “ours” or “theirs” without reconstructing the final hardware description.

## Measure Divergence

Useful metrics:

```text
total carried patches
DT/binding patches by lifecycle class
median and maximum patch age
patches without owner/upstream link/drop condition
files with repeated rebase conflicts
semantic delta from upstream for each product DTB
warnings/suppressions introduced downstream
accepted upstream commits not yet integrated
```

Optimize for risk and age, not a cosmetic patch-count target. One patch redefining a binding is riskier than several board additions.

## Avoid Dual Sources Of Truth

Common failure patterns:

- binding documented only in an internal wiki while downstream DTS uses it
- generated vendor tree and hand-edited kernel tree both considered authoritative
- bootloader carries a forked DTS copy with untracked differences
- overlays live in a release repository without the base compatibility contract
- fixes applied independently to multiple long-term branches

Choose a canonical source and automate synchronization or artifact comparison where duplication cannot be eliminated.

## Upstream Submission Discipline

Before posting:

```bash
make dt_binding_check DT_SCHEMA_FILES=/acme,axc-capture/
make ARCH=arm64 dtbs_check DT_SCHEMA_FILES=/acme,axc-capture/
scripts/checkpatch.pl --strict 000*.patch
scripts/get_maintainer.pl 0001-*.patch
```

Also:

- use current maintainer tree as the patch base
- describe the hardware problem and user-visible impact
- keep the series self-contained
- include version-to-version changelog outside the commit message
- retain reviewers on later revisions
- link public discussion and use `Fixes:`/stable routing when appropriate
- do not expose confidential hardware information without authorization

## Integrate Upstream Results Back Downstream

When upstream accepts a different design:

1. Compare binding and semantics, not only patch text.
2. Adapt the downstream driver to accept deployed old DTBs if required.
3. migrate source toward upstream representation
4. retain compatibility during the supported field window
5. update manifests, CI, and documentation
6. drop the superseded carry patch only after artifacts and hardware qualify

Upstream acceptance and product release are separate gates with traceable linkage.

## Patch-Stack Review Checklist

```text
[ ] canonical source and target upstream base are named
[ ] every patch has owner, class, origin, and drop condition
[ ] new bindings follow upstream design and schema conventions
[ ] binding/driver/DTS routing and dependencies are explicit
[ ] each intermediate patch is buildable and reviewable
[ ] rebase compares generated artifacts, not only conflicts
[ ] upstreamed patches are dropped by identity
[ ] duplicate source trees are compared or eliminated
[ ] divergence age/risk is visible to release decisions
[ ] product validation gates upstream integration
```

## Further Reading

- [Submitting Devicetree binding patches](https://docs.kernel.org/devicetree/bindings/submitting-patches.html)
- [Linux patch submission guide](https://docs.kernel.org/process/submitting-patches.html)
- [Linux SoC maintainer process](https://docs.kernel.org/process/maintainer-soc.html)
- [Binding Evolution, Deprecation, And Migration](binding-evolution-deprecation-and-migration.md)
