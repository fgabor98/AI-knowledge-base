---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Microcontroller Platforms

Outline page for the C Programming roadmap.

## Scope

- Platform-Specific C
- Embedded-oriented C programming
- Standard, implementation, platform, and toolchain boundaries

## Learning Objectives

- Define the problem addressed by this topic.
- Identify the relevant language rules and implementation assumptions.
- Apply the topic safely in host, freestanding, RTOS, or embedded Linux code.
- Recognize common failure modes and debugging evidence.
- Connect the topic to standards, tests, and production practices.

## Topic Outline

- ARM Cortex-M
- RISC-V microcontrollers
- Exception models
- Interrupt controllers
- MPU usage
- Flash wait states
- Tightly coupled memory
- FPU and DSP extensions
- Vendor startup code
- HAL tradeoffs

## Exercises And Examples

- Add a minimal hosted example.
- Add an embedded-oriented example where applicable.
- Add a failure-focused example.
- Add tests, diagnostics, or measurement guidance.

## Common Mistakes

- Treating implementation behavior as portable ISO C behavior.
- Omitting lifetime, ownership, bounds, or concurrency assumptions.
- Ignoring the target platform and toolchain contract.
- Writing examples without failure handling or verification.

## Related Topics

- [Platform-Specific C overview](./index.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- Add primary standards, compiler documentation, ABI references, manuals, and platform documentation during the content pass.

