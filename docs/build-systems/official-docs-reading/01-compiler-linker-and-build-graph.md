---
status: active
reviewed: false
domain: build-systems
difficulty: beginner
last_reviewed: null
---

# Compiler, Linker, And Build Graph Foundations

Official reading for the transformation from source files to object files,
executables, shared libraries, and repeatable build actions.

## Official Reading

- [ ] **P0** [GCC documentation](https://gcc.gnu.org/onlinedocs/gcc/): invocation, preprocessing, compilation, optimization, warnings, debugging, and target options.
- [ ] **P0** [GNU Binutils documentation](https://sourceware.org/binutils/docs/): `as`, `ld`, `ar`, `nm`, `objcopy`, `objdump`, `readelf`, and `strip`.
- [ ] **P0** [GNU `ld` linker documentation](https://sourceware.org/binutils/docs/ld/): inputs, search paths, scripts, symbols, relocations, and shared objects.
- [ ] **P0** [GNU Make manual](https://www.gnu.org/software/make/manual/make.html): rules, prerequisites, recipes, variables, and the basic update algorithm.
- [ ] **P1** [ELF specification](https://refspecs.linuxfoundation.org/elf/): sections, segments, symbols, relocations, and dynamic linking.

## Practice And Evidence

- [ ] Compile, assemble, link, inspect, and strip a small C program by hand.
- [ ] Explain the difference between a source, preprocessed source, object,
  executable, shared library, and debug-information file.
- [ ] Use `readelf`, `objdump`, `nm`, and `ldd` or an equivalent target-side
  inspection workflow to answer a concrete artifact question.
- [ ] Write a Makefile whose prerequisites express the real dependency graph,
  including generated headers and order-only prerequisites.

## Exit Criteria

- [ ] I can explain every major compiler and linker command in a verbose build.
- [ ] I can distinguish a missing source dependency, compile failure, link
  failure, and runtime loader failure from the evidence they leave.
