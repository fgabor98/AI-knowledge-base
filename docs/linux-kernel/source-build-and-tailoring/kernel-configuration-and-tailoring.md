---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Configuration And Tailoring

## What Problem Does This Solve?

Kernel configuration selects which drivers, subsystems, debug features, and platform policies are present in a kernel build.

## Core Concepts

- Kconfig
- `.config`
- defconfig
- `menuconfig`
- config fragments
- `CONFIG_*` symbols
- built-in selections
- module selections
- dependency resolution

## Mental Model

The source tree contains many drivers, but only selected Kconfig symbols become part of the build. Tailoring the kernel means intentionally choosing the runtime surface the product needs.

## Practice Skeleton

- Start from a board defconfig.
- Enable one driver as built-in.
- Enable one driver as a module.
- Compare requested options with the final `.config`.

## Debugging Checklist

- Check the final `.config`, not only the fragment.
- Look for unmet dependencies.
- Confirm built-in versus module policy.
- Keep debug-only options separate from production options.

## Related Topics

- [Kconfig And Defconfig](../../build-systems/advanced/linux-kernel/kconfig-and-defconfig.md)
- [Configuration Fragments And Auditing](../../build-systems/advanced/linux-kernel/configuration-fragments-and-auditing.md)
- [Kernel Configuration And Platform Policy](../configuration-and-platform-policy/index.md)
