---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Stage 15: Cross-Compilation, Packaging, And Target Integration

An application is deployable only when its build identity, ABI, loader, libraries, files, service definition, permissions, and update artifact agree. Cross-compilation makes this contract explicit because the machine running the compiler is not the machine running the result.

## Build-to-target model

Keep **build**, **host**, and **target** roles distinct. Select a canonical target triple, compiler, sysroot, libc, kernel UAPI, and package metadata as one toolchain contract. Do not let host headers, libraries, or `pkg-config` results leak into the target build.

Package installation must be staged into a destination root, then validated as it will appear on the target. Treat the service unit, users/groups, directories, tmpfiles, capabilities, and upgrade behavior as part of the application package.

## Release gates

Before release, verify architecture and interpreter, dynamic dependencies, file ownership/modes, service startup, upgrade/rollback, debug artifact retention, SBOM/provenance, and reproducibility expectations. Test the packaged result, not only the build tree.

## Completion checklist

- [ ] Build and target identities are explicit and reproducible.
- [ ] No host library or header contaminates the target sysroot.
- [ ] The deployed loader and all required libraries are present and compatible.
- [ ] Package ownership, permissions, service integration, and state directories are defined.
- [ ] Every release can be traced to source, toolchain, inputs, and tests.

## Stage exercise

Cross-build a small utility using the product SDK, stage its install, package
it, and execute the packaged artifact in a clean target image. Preserve the
compiler/sysroot identity and inspect ELF interpreter/dependencies before
deployment.

Add the service unit and account through the package workflow, verify readiness
after boot, then upgrade from a previous package with modified configuration.
The evidence should connect source to package to image to running process,
including its matching debug symbols.

## Learning materials

1. [Target Triples, Sysroots, And ABI](target-triples-sysroots-and-abi.md)
2. [Dynamic Loader And Library Deployment](dynamic-loader-and-library-deployment.md)
3. [Installation Layout And Package Integration](installation-layout-and-package-integration.md)
4. [Yocto Application And Service Integration](yocto-application-and-service-integration.md)
5. [Artifacts, Provenance, And Release Identity](artifacts-provenance-and-release-identity.md)
