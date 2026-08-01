---
status: active
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# 6. U-Boot, Boot Handoff, Mutation, FIT, And Overlays

Official sections: [Devicetree in U-Boot](https://docs.u-boot.org/en/stable/develop/devicetree/index.html) and [Linux Devicetree overlays](https://docs.kernel.org/devicetree/overlay-notes.html)

Knowledge-guide companion: [Stage 6](knowledge-guide-companion.md#stage-6-u-boot-boot-handoff-mutation-fit-and-overlays)

## Distinguish Every Tree

- [ ] **P0** [Devicetree Control in U-Boot](https://docs.u-boot.org/en/stable/develop/devicetree/control.html).
- [ ] **P0** U-Boot control FDT versus working FDT passed to an operating system.
- [ ] **P0** TPL/SPL/U-Boot-proper tree subsets, relocation, and memory constraints.
- [ ] **P0** Linux kernel DTB versus U-Boot `*-u-boot.dtsi` additions and external fragments.
- [ ] **P0** `CONFIG_OF_*`, `DEFAULT_DEVICE_TREE`, `DEVICE_TREE`, and exact build artifacts from the project's U-Boot version.
- [ ] **P1** U-Boot's upstream DTS sync/rebasing mechanism and downstream divergence.
- [ ] **P1** of-platdata only for projects that use it.

## Selection And Boot Flow

- [ ] **P0** [U-Boot `fdt` command](https://docs.u-boot.org/en/stable/usage/cmd/fdt.html) and control/working selection.
- [ ] **P0** [U-Boot `bootm` command](https://docs.u-boot.org/en/stable/usage/cmd/bootm.html).
- [ ] **P0** Bootstd/extlinux/PXE configuration used by the product, including `fdt`, `fdtdir`, and configuration selection.
- [ ] **P0** Load addresses, sizes, relocation, growth room, initrd/kernel/FDT overlap, and exact handoff address.
- [ ] **P0** Architecture boot protocol such as [AArch64 booting](https://docs.kernel.org/arch/arm64/booting.html).
- [ ] **P1** Multi-DTB FIT selection in SPL only when used.
- [ ] **P1** UEFI handoff paths only for products booting Linux through UEFI.

## FIT And Packaging

- [ ] **P0** [FIT format index](https://docs.u-boot.org/en/stable/usage/fit/index.html) and the exact FIT specification/tool version.
- [ ] **P0** Image nodes, hash nodes, configuration nodes, defaults, load/entry addresses, and FDT references.
- [ ] **P0** [FIT signature verification](https://docs.u-boot.org/en/stable/usage/fit/signature.html), especially signed configurations versus separately signed images.
- [ ] **P0** [Verified boot](https://docs.u-boot.org/en/stable/usage/fit/verified-boot.html) and trust-anchor placement.
- [ ] **P1** [Binman documentation](https://docs.u-boot.org/en/stable/develop/package/binman.html) for products using DT-based image layouts.
- [ ] **P1** FIT extra configurations/overlays and board-identity mapping when used.

## Boot-Time Mutation

- [ ] **P0** Inventory every fixup affecting `/memory`, `/reserved-memory`, `/chosen`, MAC addresses, serial identity, status, or board options.
- [ ] **P0** Separate mutation authority, input source, validation, ordering, and failure behavior.
- [ ] **P0** libfdt capacity/relocation and error handling for in-place changes.
- [ ] **P0** Final working-FDT inspection before handoff.
- [ ] **P1** [Pre-relocation DT manipulation](https://docs.u-boot.org/en/stable/develop/driver-model/fdt-fixup.html) only when the platform uses it.
- [ ] **P1** Firmware/secure-monitor mutations outside U-Boot and how they are evidenced.

## Overlay Format And Resolution

- [ ] **P0** [dt-object internal format](https://git.kernel.org/pub/scm/utils/dtc/dtc.git/tree/Documentation/dt-object-internal.txt).
- [ ] **P0** `/plugin/`, fragments, targets, `__overlay__`, and label/path targeting.
- [ ] **P0** `__symbols__`, `__fixups__`, and `__local_fixups__` generated with `-@`.
- [ ] **P0** phandle relocation, external-symbol resolution, and merge semantics.
- [ ] **P0** base/overlay source ABI and why successful resolution is not semantic compatibility.
- [ ] **P1** [U-Boot Device Tree overlays](https://docs.u-boot.org/en/stable/usage/fdt_overlays.html).
- [ ] **P1** [U-Boot FIT overlay use](https://docs.u-boot.org/en/stable/usage/fit/overlay-fdt-boot.html).

## Linux Runtime Overlays

- [ ] **P0** [Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html).
- [ ] **P0** [Devicetree Dynamic Resolver Notes](https://docs.kernel.org/devicetree/dynamic-resolution-notes.html).
- [ ] **P0** [Devicetree Changesets](https://docs.kernel.org/devicetree/changesets.html).
- [ ] **P0** Apply/remove cookies, stacking order, device creation/removal, notifier lifetime, and forbidden retained pointers.
- [ ] **P1** Runtime overlay user interface only from the exact platform/kernel; do not assume configfs support is a stable generic ABI.
- [ ] **P1** Product policy for authentication, allowed targets, conflict detection, and removal safety.

## Measured And Verified Handoff

- [ ] **P0** [U-Boot measured boot](https://docs.u-boot.org/en/stable/usage/measured_boot.html) when attestation is required.
- [ ] **P0** Determine whether base DTB, overlays, selected configuration, bootargs, and final mutated FDT are authenticated and/or measured.
- [ ] **P1** TPM event-log handoff and Linux TPM event-log documentation for measured platforms.
- [ ] **P1** Distinguish authorization, integrity, measurement, freshness, and anti-rollback.

## Practical Exercises

- [ ] Prove the control FDT, selected packaged DTB, post-fixup working FDT, handoff address, boot FDT, and live Linux tree on one board.
- [ ] Apply the same overlay offline and in the actual boot path; compare final semantic trees.
- [ ] Trigger an overlay capacity or missing-symbol failure and verify safe failure behavior.
- [ ] Build a signed configuration that binds kernel, DTB, overlays, and initramfs; attempt mix-and-match negative boots.
- [ ] Inventory all boot-time mutations and identify which are reproducible, authenticated, constrained, measured, and logged.

## Stage Completion

- [ ] I can name and prove every Device Tree artifact/state across TPL/SPL/U-Boot/Linux.
- [ ] I can explain FIT selection and signed-configuration coverage without confusing hashes with authorization.
- [ ] I can decode overlay symbols/fixups and reason about composition/removal lifetime.
- [ ] I can identify every post-verification mutation and the final tree handed to Linux.
