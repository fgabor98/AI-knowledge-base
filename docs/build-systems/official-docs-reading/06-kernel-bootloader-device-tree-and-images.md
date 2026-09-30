---
status: active
reviewed: false
domain: build-systems
difficulty: advanced
last_reviewed: null
---

# Kernel, Bootloader, Device Tree, And Images

Official reading for the build products that make an embedded Linux board boot.

## Official Reading

- [ ] **P0** [Linux kernel Kbuild documentation](https://docs.kernel.org/kbuild/): source tree, out-of-tree builds, Kconfig, modules, device trees, install targets, and reproducibility.
- [ ] **P0** [Linux kernel Kconfig language](https://docs.kernel.org/kbuild/kconfig-language.html): symbols, dependencies, defaults, choices, and generated configuration.
- [ ] **P0** [U-Boot documentation](https://docs.u-boot.org/en/latest/): build system, board configuration, SPL/TPL, environment, FIT images, and verified boot.
- [ ] **P0** [Devicetree Specification](https://devicetree-specification.readthedocs.io/en/stable/): source and blob model, nodes, properties, phandles, and bootloader handoff.
- [ ] **P1** [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html): how a built DTB is consumed by the kernel.
- [ ] **P1** [U-Boot FIT documentation](https://docs.u-boot.org/en/latest/usage/fit/howto.html): FIT and other boot artifact construction.

## Practice And Evidence

- [ ] Cross-build a kernel, DTB, modules, U-Boot, and one boot image for a
  documented board configuration.
- [ ] Trace one source or configuration change through generated files and the
  final artifact that carries it.
- [ ] Inspect kernel release artifacts, module metadata, DTB contents, FIT
  signatures, and the bootloader-to-kernel handoff.
- [ ] Record which build system owns each artifact and which system only
  packages or signs it.

## Exit Criteria

- [ ] I can distinguish source, generated configuration, intermediate object,
  deployable artifact, and signed boot bundle.
- [ ] I can reproduce a board image and prove which kernel, DTB, bootloader, and
  configuration it contains.
