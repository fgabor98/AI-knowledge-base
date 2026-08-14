---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Common Embedded Libraries

Outline page for the C Programming roadmap.

## Scope

- Standard Library And Ecosystem
- Embedded-oriented C programming
- Standard, implementation, platform, and toolchain boundaries

## Learning Objectives

- Define the problem addressed by this topic.
- Identify the relevant language rules and implementation assumptions.
- Apply the topic safely in host, freestanding, RTOS, or embedded Linux code.
- Recognize common failure modes and debugging evidence.
- Connect the topic to standards, tests, and production practices.

## Topic Outline

- Vendor HALs
- CMSIS-style interfaces
- RTOS APIs
- FreeRTOS
- Zephyr
- lwIP
- TLS libraries
- USB stacks
- CAN stacks
- Filesystem libraries
- Serialization libraries
- Test and mocking frameworks

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

- [Standard Library And Ecosystem overview](./index.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- Add primary standards, compiler documentation, ABI references, manuals, and platform documentation during the content pass.

