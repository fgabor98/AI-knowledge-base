---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Product-Scale Maintenance And Engineering

This page covers the engineering practices needed to maintain Device Tree support across product families, organizations, release lines, and upstream development.

## Topics Covered

- Devicetree specification version differences and compatibility implications
- schema and DTS review methodology
- separating correctness, ABI, style, and maintainability concerns during review
- maintaining downstream vendor trees vs upstream DTS
- managing patch stacks and minimizing long-lived DTS divergence
- cross-version kernel, bootloader, firmware, DTB, and overlay compatibility testing
- large-product DT organization and ownership conventions
- defining ownership boundaries across silicon, module, carrier-board, and product teams
- CI design for multiple boards, product variants, and overlays
- validation matrices and representative hardware coverage
- deprecation and migration strategies for bindings, properties, and compatible strings
- realistic board bring-up failure postmortems
- converting failures and escapes into reusable review checks and CI coverage

## Related Topics

- [Binding Design And Stable ABI](binding-design-and-stable-abi.md)
- [Security And Production Lifecycle](security-and-production-lifecycle.md)
- [Board Porting Workflow](board-porting-workflow.md)
