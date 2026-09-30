---
status: active
reviewed: false
domain: build-systems
difficulty: intermediate
last_reviewed: null
---

# Knowledge Guide Companion Checklist

This checklist tracks the Build Systems pages that accompany the [Official
Build Systems Documentation Reading Checklist](index.md). It uses the same
eight stages so official manuals, local explanations, source inspection, and
build labs can be completed together.

All 110 Build Systems pages that existed when this checklist was created are
assigned exactly once. A page may relate to several stages, but it has only one
checkbox here so its completion state remains unambiguous.

## Synchronization Rule

For each stage:

```text
read one coherent group of official documents
-> read the matching knowledge-guide pages below
-> reproduce or inspect the associated build
-> preserve command lines, manifests, and artifacts
-> complete the associated exercise or lab
-> check off both trackers
```

Mark a knowledge-guide page complete when you can explain its mental model,
identify its inputs and outputs, and apply its debugging checklist. Reading the
words once is not enough.

## Stage 1: Compiler, Linker, And Build Graph Foundations

Official tracker: [Compiler, Linker, And Build Graph Foundations](01-compiler-linker-and-build-graph.md)

- [ ] [Build Systems](../index.md)
- [ ] [Build Systems for Embedded Linux](../embedded-linux-roadmap.md)
- [ ] [Advanced Build Systems](../advanced/index.md)
- [ ] [Direct Compiler Invocation](../direct-compiler-invocation.md)
- [ ] [Object Files and Linking](../object-files-and-linking.md)
- [ ] [Make Basics](../make-basics.md)
- [ ] [Make Variables and Pattern Rules](../make-variables-and-pattern-rules.md)
- [ ] [Native Linux Userspace Builds](../native-linux-userspace-builds.md)

Stage completion:

- [ ] I can follow source, preprocessing, compilation, assembly, linking, and loading.
- [ ] I can write and inspect a correct dependency graph for a small project.

## Stage 2: Build Orchestrators And Project Integration

Official tracker: [Build Orchestrators And Project Integration](02-build-orchestrators-and-project-integration.md)

- [ ] [CMake Basics](../cmake-basics.md)
- [ ] [Ninja as a Generated Backend](../ninja-generated-backend.md)
- [ ] [Autotools And Meson For Embedded Cross-Builds](../autotools-and-meson-for-embedded-cross-builds.md)

Stage completion:

- [ ] I can explain the difference between a project description, generated build graph, and executor.
- [ ] I can configure, build, test, and install a project while preserving the relevant logs.

## Stage 3: Cross-Compilation, Toolchains, And Sysroots

Official tracker: [Cross-Compilation, Toolchains, And Sysroots](03-cross-compilation-toolchains-and-sysroots.md)

- [ ] [Cross-Compilation](../cross-compilation.md)
- [ ] [Target Triples and Sysroots](../target-triples-and-sysroots.md)
- [ ] [CMake Toolchain Files](../cmake-toolchain-files.md)
- [ ] [Target pkg-config](../target-pkg-config.md)
- [ ] [Cross-Compilation Toolchains In Depth](../advanced/cross-compilation-toolchains-in-depth.md)

Stage completion:

- [ ] I can document the build, host, and target systems and the exact toolchain selected.
- [ ] I can prove that headers, libraries, and `pkg-config` metadata came from the intended sysroot.

## Stage 4: Dependencies, Installation, Packaging, And ABI

Official tracker: [Dependencies, Installation, Packaging, And ABI](04-dependencies-installation-packaging-and-abi.md)

- [ ] [Shared Libraries, ABI, And Runtime Linking](../shared-libraries-abi-and-runtime-linking.md)
- [ ] [CMake Package Discovery](../cmake-package-discovery.md)
- [ ] [Install Rules and Staging](../install-rules-and-staging.md)
- [ ] [Source Fetching and Patch Management](../source-fetching-and-patch-management.md)
- [ ] [Patch Management For Embedded Builds](../patch-management-for-embedded-builds.md)

Stage completion:

- [ ] I can trace a dependency from discovery through staging, packaging, and runtime loading.
- [ ] I can state the ABI and provenance consequences of a dependency or patch change.

## Stage 5: Embedded Distribution Build Systems

Official tracker: [Embedded Distribution Build Systems](05-embedded-distribution-build-systems.md)

### Image And Root Filesystem Composition

- [ ] [Filesystem Image Basics](../filesystem-image-basics.md)
- [ ] [Package Management And Rootfs Composition](../advanced/package-management-and-rootfs-composition.md)
- [ ] [Initramfs, Recovery, And Manufacturing Images](../advanced/initramfs-recovery-and-manufacturing-images.md)
- [ ] [Buildroot](../advanced/buildroot.md)

### Yocto And OpenEmbedded

- [ ] [Yocto and OpenEmbedded](../advanced/yocto-openembedded/index.md)
- [ ] [Yocto, OpenEmbedded, and BitBake Mental Model](../advanced/yocto-openembedded/mental-model.md)
- [ ] [Build Directory and Configuration](../advanced/yocto-openembedded/build-directory-and-configuration.md)
- [ ] [Layers](../advanced/yocto-openembedded/layers.md)
- [ ] [Machine and Distro Configuration](../advanced/yocto-openembedded/machine-and-distro-configuration.md)
- [ ] [Recipes](../advanced/yocto-openembedded/recipes.md)
- [ ] [BitBake Metadata, Overrides, and Python](../advanced/yocto-openembedded/bitbake-metadata-overrides-and-python.md)
- [ ] [Tasks and Workdirs](../advanced/yocto-openembedded/tasks-and-workdirs.md)
- [ ] [Images and Package Groups](../advanced/yocto-openembedded/images-and-packagegroups.md)
- [ ] [Kernel Recipe Internals](../advanced/yocto-openembedded/kernel-recipe-internals.md)
- [ ] [Kernel and Bootloader Integration](../advanced/yocto-openembedded/kernel-and-bootloader-integration.md)
- [ ] [Packaging, QA, and Package Feeds](../advanced/yocto-openembedded/packaging-qa-and-feeds.md)
- [ ] [WIC and Partition Layouts](../advanced/yocto-openembedded/wic-and-partition-layouts.md)
- [ ] [SDK Generation](../advanced/yocto-openembedded/sdk-generation.md)
- [ ] [Devtool and Recipe Development](../advanced/yocto-openembedded/devtool-and-recipe-development.md)
- [ ] [Multiconfig and Firmware Builds](../advanced/yocto-openembedded/multiconfig-and-firmware-builds.md)
- [ ] [Licensing, CVE, and SBOM Workflows](../advanced/yocto-openembedded/licensing-cve-and-sbom.md)
- [ ] [Reproducibility, Caches, and Mirrors](../advanced/yocto-openembedded/reproducibility-caches-and-mirrors.md)
- [ ] [CI, Hash Equivalence, and Shared State](../advanced/yocto-openembedded/ci-hash-equivalence-and-sstate.md)
- [ ] [Debugging BitBake Builds](../advanced/yocto-openembedded/debugging-bitbake-builds.md)
- [ ] [End-to-End Product Layer Lab](../advanced/yocto-openembedded/end-to-end-product-layer-lab.md)

Stage completion:

- [ ] I can trace a package from source fetch to rootfs, package feed, and image output.
- [ ] I can explain configuration ownership, layer/recipe ownership, and when a rebuild is required.

## Stage 6: Kernel, Bootloader, Device Tree, And Images

Official tracker: [Kernel, Bootloader, Device Tree, And Images](06-kernel-bootloader-device-tree-and-images.md)

### Linux Kernel Build System

- [ ] [Linux Kernel Build System](../advanced/linux-kernel/index.md)
- [ ] [Kernel Source Tree and Outputs](../advanced/linux-kernel/source-tree-and-outputs.md)
- [ ] [Kbuild Objects and Directories](../advanced/linux-kernel/kbuild-objects-and-directories.md)
- [ ] [Kconfig and Defconfig](../advanced/linux-kernel/kconfig-and-defconfig.md)
- [ ] [Configuration Fragments and Auditing](../advanced/linux-kernel/configuration-fragments-and-auditing.md)
- [ ] [Cross-Building and Installing the Kernel](../advanced/linux-kernel/cross-building-and-installing.md)
- [ ] [Kernel Release Artifacts](../advanced/linux-kernel/kernel-release-artifacts.md)
- [ ] [Modules and External Modules](../advanced/linux-kernel/modules-and-external-modules.md)
- [ ] [Device Tree Builds](../advanced/linux-kernel/device-tree-builds.md)
- [ ] [Device Tree Binding Validation](../advanced/linux-kernel/device-tree-binding-validation.md)
- [ ] [Initramfs and Built-In Root Filesystem](../advanced/linux-kernel/initramfs-and-built-in-rootfs.md)
- [ ] [Vendor Kernel Patch Management](../advanced/linux-kernel/vendor-kernel-patch-management.md)
- [ ] [Kernel Documentation Reading Guide](../advanced/linux-kernel/documentation-reading-guide.md)

### U-Boot And Boot Artifacts

- [ ] [U-Boot Build System](../advanced/u-boot/index.md)
- [ ] [Source Tree and Outputs](../advanced/u-boot/source-tree-and-outputs.md)
- [ ] [Kconfig and Generated Config](../advanced/u-boot/kconfig-and-generated-config.md)
- [ ] [SPL, TPL, and U-Boot Proper](../advanced/u-boot/spl-tpl-and-u-boot-proper.md)
- [ ] [Cross-Building and Flashing](../advanced/u-boot/cross-building-and-flashing.md)
- [ ] [Board Defconfigs](../advanced/u-boot/board-defconfigs.md)
- [ ] [Device Tree in U-Boot](../advanced/u-boot/device-tree-in-u-boot.md)
- [ ] [Environment and Boot Flow](../advanced/u-boot/environment-and-boot-flow.md)
- [ ] [FIT Images and Boot Artifacts](../advanced/u-boot/fit-images-and-boot-artifacts.md)
- [ ] [Secure Boot and Signing](../advanced/u-boot/secure-boot-and-signing.md)
- [ ] [Driver Model and Pre-Relocation](../advanced/u-boot/driver-model-and-pre-relocation.md)
- [ ] [Board Porting and Bring-Up](../advanced/u-boot/board-porting-and-bring-up.md)
- [ ] [Release Artifacts and Provenance](../advanced/u-boot/release-artifacts-and-provenance.md)
- [ ] [Vendor U-Boot Patch Management](../advanced/u-boot/vendor-u-boot-patch-management.md)
- [ ] [U-Boot Documentation Reading Guide](../advanced/u-boot/documentation-reading-guide.md)

### Board And Image Composition

- [ ] [Device Tree Build And Validation](../advanced/device-tree-build-and-validation.md)
- [ ] [Boot Image Composition, FIT, And Signing](../advanced/boot-image-composition-fit-and-signing.md)

Stage completion:

- [ ] I can identify the owner, source, configuration, and consumer of every boot artifact.
- [ ] I can reproduce and inspect a kernel, DTB, bootloader, initramfs, FIT, and signed image set.

## Stage 7: Debugging, Testing, Reproducibility, And CI

Official tracker: [Debugging, Testing, Reproducibility, And CI](07-debugging-testing-reproducibility-and-ci.md)

- [ ] [Build Artifact Debugging](../build-artifact-debugging.md)
- [ ] [Build Caching and Mirrors](../build-caching-and-mirrors.md)
- [ ] [Build Quality Gates](../build-quality-gates.md)
- [ ] [Build CI For Embedded Linux](../advanced/build-ci-for-embedded-linux.md)
- [ ] [Reproducible Embedded Linux Releases](../advanced/reproducible-embedded-linux-releases.md)
- [ ] [Debugging Kernel Builds](../advanced/linux-kernel/debugging-kernel-builds.md)
- [ ] [Reproducible Kernel Builds](../advanced/linux-kernel/reproducible-kernel-builds.md)
- [ ] [Debugging U-Boot Builds](../advanced/u-boot/debugging-u-boot-builds.md)
- [ ] [Reproducible U-Boot Builds](../advanced/u-boot/reproducible-u-boot-builds.md)

Stage completion:

- [ ] I can preserve enough evidence to reproduce a failure without the original workstation.
- [ ] I can compare clean and cached builds and explain intentional output differences.
- [ ] I can enforce quality, security, and artifact checks in CI at the right boundary.

## Stage 8: Product Builds, Supply Chain, And Release Delivery

Official tracker: [Product Builds, Supply Chain, And Release Delivery](08-product-builds-supply-chain-and-release-delivery.md)

### BSP Integration And Productization

- [ ] [BSP Build Integration](../advanced/bsp-build-integration.md)
- [ ] [BSP Artifact Flow and Provenance](../advanced/bsp-integration/artifact-flow-and-provenance.md)
- [ ] [Configuration and Patch Ownership](../advanced/bsp-integration/configuration-and-patch-ownership.md)
- [ ] [Image Layout and Deployment](../advanced/bsp-integration/image-layout-and-deployment.md)
- [ ] [Boot Debugging and Runtime Validation](../advanced/bsp-integration/boot-debugging-and-runtime-validation.md)
- [ ] [BSP Release Reproducibility](../advanced/bsp-integration/release-reproducibility.md)
- [ ] [Board Porting Build Workflow](../advanced/board-porting-build-workflow.md)
- [ ] [OTA And Update System Build Integration](../advanced/ota-update-system-build-integration.md)

### TI Processor SDK Linux

- [ ] [TI Processor SDK Linux](../advanced/ti-processor-sdk/index.md)
- [ ] [SDK Overview and Release Model](../advanced/ti-processor-sdk/sdk-overview-and-release-model.md)
- [ ] [Build Environment Setup](../advanced/ti-processor-sdk/build-environment-setup.md)
- [ ] [Installed SDK and Source Layout](../advanced/ti-processor-sdk/installed-sdk-and-source-layout.md)
- [ ] [Machines, Distros, and Image Targets](../advanced/ti-processor-sdk/machines-distros-and-image-targets.md)
- [ ] [TI Yocto and Arago Build Flow](../advanced/ti-processor-sdk/ti-yocto-arago-build-flow.md)
- [ ] [SDK Build System vs TI Yocto Layers](../advanced/ti-processor-sdk/sdk-build-system-vs-ti-yocto-layers.md)
- [ ] [SDK Customization for Products](../advanced/ti-processor-sdk/sdk-customization-for-products.md)
- [ ] [Kernel Integration](../advanced/ti-processor-sdk/kernel-integration.md)
- [ ] [U-Boot Integration](../advanced/ti-processor-sdk/u-boot-integration.md)
- [ ] [Firmware and Heterogeneous Cores](../advanced/ti-processor-sdk/firmware-and-heterogeneous-cores.md)
- [ ] [Boot Artifact Pipeline](../advanced/ti-processor-sdk/boot-artifact-pipeline.md)
- [ ] [Deployment and Flashing](../advanced/ti-processor-sdk/deployment-and-flashing.md)
- [ ] [Debugging TI SDK Builds and Boots](../advanced/ti-processor-sdk/debugging-ti-sdk-builds-and-boots.md)
- [ ] [Custom Sitara Board Bring-Up](../advanced/ti-processor-sdk/custom-sitara-board-bring-up.md)
- [ ] [Release Engineering and SDK Upgrades](../advanced/ti-processor-sdk/release-engineering-and-sdk-upgrades.md)
- [ ] [End-to-End Product Layer Lab](../advanced/ti-processor-sdk/end-to-end-product-layer-lab.md)

Stage completion:

- [ ] I can trace product source, BSP layers, configuration, patches, artifacts, signing, and deployment.
- [ ] I can produce a release manifest and SDK that another engineer can audit and use.
- [ ] I have completed one end-to-end board or product-layer build lab.

## Overall Completion

- [ ] All 110 knowledge-guide page checkboxes are complete.
- [ ] All eight official-documentation stage checkboxes are complete.
- [ ] I have recorded version-specific differences between upstream tools and the active vendor SDK/BSP.
- [ ] I have reproduced at least one clean build and one warm-cache build.
- [ ] I have a release manifest containing source, toolchain, configuration, patches, packages, images, and checksums.
- [ ] I have a list of topics that require another pass or a project-specific deep dive.
