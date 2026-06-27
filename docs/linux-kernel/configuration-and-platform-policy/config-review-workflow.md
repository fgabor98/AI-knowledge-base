---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Config Review Workflow

## What Problem Does This Solve?

Kernel config reviews prevent accidental feature loss, debug exposure, security regressions, and unsupported product behavior.

## Core Concepts

- defconfig
- fragments
- final `.config`
- dependency resolution
- config ownership
- diff review
- audit scripts
- release gates

## Mental Model

Review the final resolved config, not only requested fragments. Kconfig dependencies can silently change the result.

## Practice Skeleton

- Generate a final `.config`.
- Compare it against expected fragments.
- Classify differences as board, product, debug, or security choices.
- Add a CI check for critical symbols.

## Debugging Checklist

- Check unmet dependencies.
- Check fragment ordering.
- Check vendor defconfig changes.
- Keep critical options under automated review.

## Related Topics

- [Configuration Fragments And Auditing](../../build-systems/advanced/linux-kernel/configuration-fragments-and-auditing.md)
- [Debug Vs Production Configs](debug-vs-production-configs.md)
- [Module Signing And Hardening](module-signing-and-hardening.md)
