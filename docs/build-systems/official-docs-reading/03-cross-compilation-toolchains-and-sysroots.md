---
status: active
reviewed: false
domain: build-systems
difficulty: intermediate
last_reviewed: null
---

# Cross-Compilation, Toolchains, And Sysroots

Official reading for separating the build host from the target system and for
making every compiler, linker, header, and library selection explicit.

## Official Reading

- [ ] **P0** [GCC documentation](https://gcc.gnu.org/onlinedocs/gcc/): target options, machine options, environment variables, and the meaning of `-B`, `-isystem`, `--sysroot`, and related search controls.
- [ ] **P0** [GNU Autoconf manual: system types](https://www.gnu.org/software/autoconf/manual/autoconf.html): build, host, target, canonical triples, and cross-compilation tests.
- [ ] **P0** [CMake toolchain-file documentation](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html): target system, compilers, sysroot, root paths, and try-compile behavior.
- [ ] **P0** [pkg-config guide](https://people.freedesktop.org/~dbn/pkg-config-guide.html): `PKG_CONFIG_SYSROOT_DIR`, `PKG_CONFIG_LIBDIR`, `.pc` files, and cross-build isolation.
- [ ] **P1** [GNU Binutils documentation](https://sourceware.org/binutils/docs/): target-prefixed tools, `ld` search paths, and target artifact inspection.
- [ ] **P1** [Yocto Project SDK documentation](https://docs.yoctoproject.org/sdk-manual/): installed SDK environments and extensible SDK behavior.

## Practice And Evidence

- [ ] Write down the build, host, and target triples for one native and one
  embedded build.
- [ ] Build a hello-world program against a controlled sysroot and prove that
  no host headers or libraries were selected.
- [ ] Use `pkg-config` in both native and target modes and inspect its search
  path and returned flags.
- [ ] Show which compiler, linker, assembler, C library, headers, and startup
  objects produced the target binary.

## Exit Criteria

- [ ] I can diagnose host contamination, wrong-architecture objects, and a
  sysroot that is incomplete or from the wrong release.
- [ ] I can reproduce a cross build from a documented toolchain and environment.
