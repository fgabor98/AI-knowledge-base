---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Source, Build, And Tailoring

This track covers the minimum kernel source and build orientation needed before writing or testing drivers.

Detailed build mechanics live in the [Linux Kernel Build System](../../build-systems/advanced/linux-kernel/index.md) chapter.

## Learning Materials

1. [Kernel Source Acquisition](kernel-source-acquisition.md)
2. [Kernel Configuration And Tailoring](kernel-configuration-and-tailoring.md)
3. [Kernel Build And Install Overview](kernel-build-and-install-overview.md)

## Mental Model

Driver development depends on a known kernel source tree, a matching configuration, and matching build artifacts. If those three do not line up, module loading and runtime debugging become misleading.

## Completion Criteria

- Identify the source tree used to build the running kernel.
- Explain which configuration selected a driver.
- Build the kernel or an external module against the correct tree.
- Locate kernel image, module, and Device Tree artifacts.

## Related Topics

- [Linux Kernel Build System](../../build-systems/advanced/linux-kernel/index.md)
- [Modules And External Modules](../../build-systems/advanced/linux-kernel/modules-and-external-modules.md)
- [Kernel Configuration And Platform Policy](../configuration-and-platform-policy/index.md)
