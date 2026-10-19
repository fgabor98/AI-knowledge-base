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

## Reading Trackers

- [Official Build Systems Documentation Reading Checklist](official-docs-reading/index.md)
- [Knowledge Guide Companion Checklist](official-docs-reading/knowledge-guide-companion.md)

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

### Intermediate Track: Compile, Link, Make, Yocto, TI SDK

For an engineer who can already write C and wants to work confidently with embedded Linux builds, this is the shortest path to intermediate level. Each step has real, re-runnable command output.

1. **Compilation stages and the compiler driver**: [Direct Compiler Invocation](direct-compiler-invocation.md), then [Translation Pipeline](../c/compilation-linking-and-abi/translation-pipeline.md) in the C section.
2. **Objects, symbols, and how binaries are composed**: [Object Files and Linking](object-files-and-linking.md), [Object Files, Symbols, and Relocations](../c/compilation-linking-and-abi/object-files-symbols-and-relocations.md), [Static and Dynamic Linking](../c/compilation-linking-and-abi/static-and-dynamic-linking.md), [Linker Scripts and Memory Layout](../c/compilation-linking-and-abi/linker-scripts-and-memory-layout.md).
3. **Runtime linking on the target**: [Shared Libraries, ABI, and Runtime Linking](shared-libraries-abi-and-runtime-linking.md).
4. **Make**: [Make Basics](make-basics.md), [Make Variables and Pattern Rules](make-variables-and-pattern-rules.md), [Intermediate Make](make-intermediate.md).
5. **Cross-compilation**: [Cross-Compilation](cross-compilation.md), [Target Triples and Sysroots](target-triples-and-sysroots.md).
6. **Yocto**: the [Yocto and OpenEmbedded](advanced/yocto-openembedded/index.md) overview (release baseline and hands-on environment), then Mental Model, Build Directory and Configuration, Recipes, Tasks and Workdirs, Packaging and QA, Debugging BitBake Builds.
7. **TI Processor SDK**: [TI Processor SDK Linux](advanced/ti-processor-sdk/index.md), then TI Yocto and Arago Build Flow, Boot Artifact Pipeline, Machines, Distros, and Image Targets, Custom Sitara Board Bring-Up.

You have reached intermediate level when you can, without looking anything up:

- name the program behind each stage of a `gcc` command and the error each stage produces;
- explain why a link succeeds and the program still does not start on the target, and prove which library the loader picked;
- write a Makefile that builds correctly incrementally, in parallel, and when called by Yocto or Buildroot;
- create a product layer with a Makefile-based recipe, find any variable's origin with `bitbake-getvar`, and fix the QA errors from a wrongly written Makefile;
- map every file that boots an AM62x to the core that runs it and the recipe and configuration that built it.

### All Pages By Level

Beginner pages:

1. [Direct Compiler Invocation](direct-compiler-invocation.md)
2. [Object Files and Linking](object-files-and-linking.md)
3. [Make Basics](make-basics.md)
4. [Native Linux Userspace Builds](native-linux-userspace-builds.md)

Intermediate pages:

1. [Make Variables and Pattern Rules](make-variables-and-pattern-rules.md)
2. [Intermediate Make](make-intermediate.md)
3. [Cross-Compilation](cross-compilation.md)
4. [Target Triples and Sysroots](target-triples-and-sysroots.md)
5. [Target pkg-config](target-pkg-config.md)
6. [Shared Libraries, ABI, and Runtime Linking](shared-libraries-abi-and-runtime-linking.md)
7. [CMake Basics](cmake-basics.md)
8. [CMake Toolchain Files](cmake-toolchain-files.md)
9. [Ninja as a Generated Backend](ninja-generated-backend.md)
10. [Install Rules and Staging](install-rules-and-staging.md)
11. [CMake Package Discovery](cmake-package-discovery.md)
12. [Autotools and Meson for Embedded Cross-Builds](autotools-and-meson-for-embedded-cross-builds.md)
13. [Filesystem Image Basics](filesystem-image-basics.md)
14. [Build Artifact Debugging](build-artifact-debugging.md)
15. [Source Fetching and Patch Management](source-fetching-and-patch-management.md)
16. [Patch Management for Embedded Builds](patch-management-for-embedded-builds.md)
17. [Build Caching and Mirrors](build-caching-and-mirrors.md)
18. [Build Quality Gates](build-quality-gates.md)

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

- real command output, produced by a tool session under `examples/build-systems/` and checked by `scripts/validate-build-systems.sh`
- the tool and release versions the output came from

- a minimal command-line example
- a realistic embedded Linux constraint
- native and cross-build notes where relevant
- debugging checks for toolchains, sysroots, and dependency discovery
- references to primary manuals or project documentation, with links
