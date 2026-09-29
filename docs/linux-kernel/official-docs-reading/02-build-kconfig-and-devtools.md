---
status: active
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# 2. Build System, Kconfig, And Development Tools

Official sections: [Kernel Build System](https://docs.kernel.org/kbuild/index.html) and
[Development tools](https://docs.kernel.org/dev-tools/index.html)

Knowledge-guide companion: [Stage 2](knowledge-guide-companion.md#stage-2-build-system-kconfig-and-development-tools)

## Kbuild And External Modules

- [x] **P0** [Kernel build system](https://docs.kernel.org/kbuild/index.html)
- [x] **P0** [Makefiles](https://docs.kernel.org/kbuild/makefiles.html)
- [x] **P0** [Building external modules](https://docs.kernel.org/kbuild/modules.html)
- [x] **P0** [Kbuild modules-only variables](https://docs.kernel.org/kbuild/modules.html#options)
- [x] **P0** [Kbuild make variables](https://docs.kernel.org/kbuild/kbuild.html)
- [x] **P0** [Kconfig language](https://docs.kernel.org/kbuild/kconfig-language.html)
- [x] **P0** [Kconfig macro language](https://docs.kernel.org/kbuild/kconfig-macro-language.html)
- [x] **P0** [LLVM builds](https://docs.kernel.org/kbuild/llvm.html)
- [x] **P1** [Reproducible builds](https://docs.kernel.org/kbuild/reproducible-builds.html)
- [x] **P1** [GCC plug-ins](https://docs.kernel.org/kbuild/gcc-plugins.html)
- [x] **P1** [Kbuild issues](https://docs.kernel.org/kbuild/issues.html)

## Embedded And BSP Build Exercises

- [x] Build the selected upstream kernel out of tree for ARM or arm64.
- [x] Build one external module against the exact target build directory.
- [x] Identify `vmlinux`, `Image`/`zImage`, DTBs, modules, `Module.symvers`, and generated headers.
- [x] Explain `ARCH`, `CROSS_COMPILE`, `O=`, `M=`, and `INSTALL_MOD_PATH` from observed output.
- [x] Trace one driver Kconfig symbol through a TI vendor defconfig and Yocto configuration fragments.
- [x] Compare upstream, vendor, and running-target `.config` values.
- [ ] Run Device Tree schema and DTB checks for a changed binding/node.
- [ ] Record how the Yocto kernel recipe selects source, config, patches, DTBs, and modules.

## Static Analysis And Patch Quality

- [-] **P0** [Checkpatch](https://docs.kernel.org/dev-tools/checkpatch.html)
- [-] **P0** [Sparse](https://docs.kernel.org/dev-tools/sparse.html)
- [-] **P0** [Coccinelle](https://docs.kernel.org/dev-tools/coccinelle.html)
- [-] **P1** [Smatch](https://docs.kernel.org/dev-tools/smatch.html)
- [-] **P1** Run `W=1` and `C=1` builds on one driver.
- [-] **P1** Run checkpatch on both a C-driver patch and a binding/Device Tree patch.
- [-] **P1** Explain at least one Sparse address-space warning involving `__iomem` or `__user`.

## Runtime Bug-Finding Tools: Orientation

Detailed labs are tracked in checklist 06.

- [->] **P0** [KASAN](https://docs.kernel.org/dev-tools/kasan.html)
- [->] **P0** [KCSAN](https://docs.kernel.org/dev-tools/kcsan.html)
- [->] **P0** [Kernel memory leak detector](https://docs.kernel.org/dev-tools/kmemleak.html)
- [->] **P1** [UBSAN](https://docs.kernel.org/dev-tools/ubsan.html)
- [->] **P1** [KMSAN](https://docs.kernel.org/dev-tools/kmsan.html)
- [->] **P1** [Lockdep design](https://docs.kernel.org/locking/lockdep-design.html)

## Completion Notes

```text
Kernel/version:
Toolchain:
Board and Yocto/vendor baseline:
Build commands verified:
Warnings investigated:
Questions to revisit:
```
