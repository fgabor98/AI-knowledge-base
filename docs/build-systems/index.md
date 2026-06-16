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
- CMake package discovery and exported targets
- dependency vendoring and source fetch policy
- cross-compilation, toolchains, and sysroots
- toolchain version pinning and build environment isolation
- kernel module build integration
- Buildroot packages, Yocto/OpenEmbedded recipes, and TI Processor SDK builds
- build caching, mirrors, and shared state
- patch management for vendor and upstream sources
- binary package feeds and SDK generation
- ABI compatibility, symbol visibility, and linker behavior
- static analysis, test, coverage, and sanitizer integration
- reproducible builds and CI checks
- release artifacts and build provenance

## Learning Path

Beginner pages:

1. [Direct Compiler Invocation](direct-compiler-invocation.md)
2. [Object Files and Linking](object-files-and-linking.md)
3. [Make Basics](make-basics.md)
4. [Make Variables and Pattern Rules](make-variables-and-pattern-rules.md)
5. [Native Linux Userspace Builds](native-linux-userspace-builds.md)

Roadmaps:

1. [Build Systems for Embedded Linux](embedded-linux-roadmap.md)
2. [Linux Kernel Build System Roadmap](linux-kernel-build-roadmap.md)
3. [U-Boot Build System Roadmap](u-boot-build-roadmap.md)
4. [Yocto and OpenEmbedded Roadmap](yocto-openembedded-roadmap.md)
5. [TI Processor SDK Roadmap](ti-processor-sdk-roadmap.md)

Then expand individual topic pages from the [Topic Map](../topic-map.md).

For product release pipelines, hardware CI, update bundles, factory flashing, and release traceability, see [Embedded Productization](../embedded-productization/index.md).

## Page Rules

Every Build Systems topic page should use the [Topic Page Template](../topic-page-template.md) and stay in draft status until reviewed.

Good Build Systems pages should include:

- a minimal command-line example
- a realistic embedded Linux constraint
- native and cross-build notes where relevant
- debugging checks for toolchains, sysroots, and dependency discovery
- references to primary manuals or project documentation
