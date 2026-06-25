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

Intermediate pages:

1. [Cross-Compilation](cross-compilation.md)
2. [Target Triples and Sysroots](target-triples-and-sysroots.md)
3. [Target pkg-config](target-pkg-config.md)
4. [Shared Libraries, ABI, and Runtime Linking](shared-libraries-abi-and-runtime-linking.md)
5. [CMake Basics](cmake-basics.md)
6. [CMake Toolchain Files](cmake-toolchain-files.md)
7. [Ninja as a Generated Backend](ninja-generated-backend.md)
8. [Install Rules and Staging](install-rules-and-staging.md)
9. [CMake Package Discovery](cmake-package-discovery.md)
10. [Autotools and Meson for Embedded Cross-Builds](autotools-and-meson-for-embedded-cross-builds.md)
11. [Filesystem Image Basics](filesystem-image-basics.md)
12. [Build Artifact Debugging](build-artifact-debugging.md)
13. [Source Fetching and Patch Management](source-fetching-and-patch-management.md)
14. [Patch Management for Embedded Builds](patch-management-for-embedded-builds.md)
15. [Build Caching and Mirrors](build-caching-and-mirrors.md)
16. [Build Quality Gates](build-quality-gates.md)

Advanced roadmaps:

1. [Advanced Build Systems](advanced/index.md)
2. [BSP Build Integration](advanced/bsp-build-integration.md)
3. [Linux Kernel Build System](advanced/linux-kernel/index.md)
4. [U-Boot Build System](advanced/u-boot/index.md)
5. [Yocto and OpenEmbedded](advanced/yocto-openembedded/index.md)
6. [TI Processor SDK Linux](advanced/ti-processor-sdk/index.md)
7. [Buildroot](advanced/buildroot.md)
8. [Device Tree Build and Validation](advanced/device-tree-build-and-validation.md)
9. [Board Porting Build Workflow](advanced/board-porting-build-workflow.md)
10. [Boot Image Composition, FIT, and Signing](advanced/boot-image-composition-fit-and-signing.md)
11. [Cross-Compilation Toolchains in Depth](advanced/cross-compilation-toolchains-in-depth.md)
12. [Reproducible Embedded Linux Releases](advanced/reproducible-embedded-linux-releases.md)
13. [OTA and Update System Build Integration](advanced/ota-update-system-build-integration.md)
14. [Initramfs, Recovery, and Manufacturing Images](advanced/initramfs-recovery-and-manufacturing-images.md)
15. [Package Management and Rootfs Composition](advanced/package-management-and-rootfs-composition.md)
16. [Build CI for Embedded Linux](advanced/build-ci-for-embedded-linux.md)

Overall roadmap:

1. [Build Systems for Embedded Linux](embedded-linux-roadmap.md)

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
