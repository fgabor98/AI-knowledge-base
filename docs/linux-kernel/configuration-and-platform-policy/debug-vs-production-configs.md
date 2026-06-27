---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Debug Vs Production Configs

## What Problem Does This Solve?

Debug kernels expose failures earlier and with more evidence, while production kernels prioritize boot time, footprint, attack surface, and deterministic behavior.

## Core Concepts

- debug symbols
- lockdep
- KASAN
- UBSAN
- dynamic debug
- ftrace
- debugfs
- production hardening
- footprint tradeoffs

## Mental Model

Use debug configs to find classes of bugs that production configs may hide. Promote only deliberate, documented options into production.

## Practice Skeleton

- Create a debug config fragment.
- Create a production config fragment.
- Compare boot time and image size.
- Run the same driver test on both configs.

## Debugging Checklist

- Confirm requested debug options survive dependency resolution.
- Check runtime overhead.
- Keep debugfs policy explicit.
- Do not ship accidental diagnostic exposure.

## Related Topics

- [Config Review Workflow](config-review-workflow.md)
- [Configuration Fragments And Auditing](../../build-systems/advanced/linux-kernel/configuration-fragments-and-auditing.md)
- [Kernel Debugging Basics](../debugging/index.md)
