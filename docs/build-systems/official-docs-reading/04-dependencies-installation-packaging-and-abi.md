---
status: active
reviewed: false
domain: build-systems
difficulty: intermediate
last_reviewed: null
---

# Dependencies, Installation, Packaging, And ABI

Official reading for how a project discovers libraries, produces an installable
layout, and preserves or intentionally changes its binary interface.

## Official Reading

- [ ] **P0** [CMake packages and dependencies](https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html) and [Using Dependencies](https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html): config packages, find modules, imported targets, components, and version constraints.
- [ ] **P0** [pkg-config guide](https://people.freedesktop.org/~dbn/pkg-config-guide.html): metadata contracts, private dependencies, variables, and transitive link flags.
- [ ] **P0** [GNU `ld` documentation](https://sourceware.org/binutils/docs/ld/): library search order, symbol resolution, version scripts, rpaths, and dynamic linking.
- [ ] **P1** [GCC visibility documentation](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html): visibility, position-independent code, and ABI-relevant code-generation choices.
- [ ] **P1** [CMake install and packaging documentation](https://cmake.org/cmake/help/latest/guide/Installing-and-Testing-with-CMake/index.html): install trees, exports, CPack, and relocatable packages.
- [ ] **P1** [System V ABI documentation](https://refspecs.linuxfoundation.org/elf/): ELF and dynamic-loader contracts.

## Practice And Evidence

- [ ] Consume one dependency through an imported CMake target and one through
  `pkg-config`; compare the resulting include and link interfaces.
- [ ] Inspect `DT_NEEDED`, RPATH/RUNPATH, symbol versions, and exported symbols
  in a shared-library artifact.
- [ ] Install a library into a staging prefix, generate its package metadata,
  and consume it from a clean build.
- [ ] Record the ABI, API, SONAME, package, and runtime-loader consequences of
  one library change.

## Exit Criteria

- [ ] I can explain where a dependency was found and why that copy won.
- [ ] I can distinguish a compile-time interface problem from a link-time,
  loader-time, or ABI-compatibility problem.
