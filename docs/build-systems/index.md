---
status: draft
reviewed: false
domain: build-systems
difficulty: beginner
last_reviewed: null
---

# Build Systems

Build system topics focused on C projects, cross-compilation, reproducibility, Yocto, Buildroot, and CI checks.

This section covers the tools and workflows that turn source code into native binaries, cross-compiled target binaries, kernel modules, packages, and embedded Linux images.

## Scope

- direct compiler and linker invocation
- Make and Makefiles
- CMake and generated build backends
- Ninja and Meson
- `pkg-config` and target dependency discovery
- cross-compilation, toolchains, and sysroots
- kernel module build integration
- Buildroot packages and Yocto recipes
- reproducible builds and CI checks

## Learning Path

Start with the embedded Linux roadmap:

1. [Build Systems for Embedded Linux](embedded-linux-roadmap.md)
2. [Linux Kernel Build System Roadmap](linux-kernel-build-roadmap.md)
3. [U-Boot Build System Roadmap](u-boot-build-roadmap.md)

Then expand individual topic pages from the [Topic Map](../topic-map.md).

## Page Rules

Every Build Systems topic page should use the [Topic Page Template](../topic-page-template.md) and stay in draft status until reviewed.

Good Build Systems pages should include:

- a minimal command-line example
- a realistic embedded Linux constraint
- native and cross-build notes where relevant
- debugging checks for toolchains, sysroots, and dependency discovery
- references to primary manuals or project documentation
