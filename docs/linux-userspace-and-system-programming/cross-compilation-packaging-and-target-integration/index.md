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
