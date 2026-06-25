---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Ninja as a Generated Backend

## What Problem Does This Solve?

Ninja is a small, fast build executor. In embedded Linux application development, you usually do not write `build.ninja` manually. Instead, CMake or Meson generates Ninja files, and Ninja executes the actual compile and link commands.

Ninja matters because it makes rebuilds fast, exposes dependency graph mistakes quickly, and is common in CI and modern CMake workflows.

## Core Concepts

- generated backend
- `build.ninja`
- fast incremental builds
- explicit dependency graph
- parallel execution
- CMake generator
- Meson backend
- compile commands database

## Mental Model

Use CMake or Meson to describe the project:

```text
CMakeLists.txt or meson.build
```

Generate Ninja files:

```text
build.ninja
```

Let Ninja execute:

```text
compiler, linker, generator, test commands
```

Ninja is intentionally low level. It is excellent at executing a known graph, but it is not where most project-level logic should live.

The usual responsibility split:

```text
CMake/Meson: decide what should exist and how targets relate
Ninja:       execute the graph quickly and correctly
compiler:    compile and link the actual code
```

When something is conceptually wrong, fix CMake or Meson. When you need to inspect what is being executed, Ninja is often the fastest view.

## Syntax / API / Mechanism

Generate Ninja with CMake:

```sh
cmake -S . -B build -G Ninja
```

Build:

```sh
cmake --build build
```

or directly:

```sh
ninja -C build
```

Verbose build:

```sh
ninja -C build -v
```

Clean:

```sh
ninja -C build clean
```

List targets:

```sh
ninja -C build -t targets
```

Explain rebuild decisions:

```sh
ninja -C build -d explain
```

Other useful tools:

```sh
ninja -C build -t clean
ninja -C build -t graph
ninja -C build -t commands
ninja -C build -t deps
```

Use these for debugging. Do not build workflows that depend heavily on parsing unstable diagnostic output.

## Minimal Example

```sh
cmake -S . -B build -G Ninja
ninja -C build
./build/app
```

Equivalent through CMake:

```sh
cmake --build build
```

Prefer `cmake --build` in documentation and scripts when you want generator independence. Use `ninja` directly when you are intentionally relying on Ninja-specific tooling.

## Real-World Example

A CMake-based userspace tool can be built with Ninja for faster local iteration:

```sh
cmake -S . -B build-arm \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake

ninja -C build-arm -v
```

The `-v` output shows the exact cross-compiler commands. This is useful when checking sysroots, include paths, library paths, and generated source dependencies.

For CI logs, this is often more useful than a quiet high-level failure:

```sh
ninja -C build-arm -v
```

## Common Scenarios

### Fast Rebuilds

Ninja is usually faster than Make for large generated dependency graphs. This is useful for CMake projects with many source files or generated code.

Speed matters in embedded development because full system builds are already expensive. Fast local app rebuilds reduce feedback time before packaging into Yocto, Buildroot, or an SDK.

### Debug A Rebuild That Should Not Happen

Use:

```sh
ninja -C build -d explain
```

This shows why Ninja thinks a target is dirty.

Typical reasons:

- command line changed
- input file timestamp changed
- output file missing
- dependency file lists a changed header
- generated file rule is incomplete

### Export Compile Commands

With CMake:

```sh
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

This generates:

```text
build/compile_commands.json
```

Many language servers, static analyzers, and editor tools use this file.

Typical uses:

- clangd
- clang-tidy
- include path debugging
- editor diagnostics
- custom static analysis scripts

### Generated Files

Ninja expects dependencies to be accurate. If a generated header is missing from the build graph, parallel builds may fail. This is not Ninja being unreliable; it is exposing an incomplete dependency model.

In CMake, generated files usually need explicit custom commands and target dependencies:

```cmake
add_custom_command(
    OUTPUT generated.h
    COMMAND generator input.txt generated.h
    DEPENDS generator input.txt
)
```

Then ensure a target consumes `generated.h` through sources or dependencies.

### CMake Regeneration

When using CMake with Ninja, CMake usually adds regeneration rules so build files update when `CMakeLists.txt` changes. If behavior looks stale, reconfigure manually:

```sh
cmake -S . -B build -G Ninja
```

### `cmake --build` vs `ninja`

Use:

```sh
cmake --build build
```

when writing generator-independent scripts.

Use:

```sh
ninja -C build -v
```

when you intentionally need Ninja-specific behavior or diagnostics.

### Cross-Build Inspection

For cross-builds, inspect:

```sh
ninja -C build -v
```

Look for:

- target compiler name
- `--sysroot`
- target include paths
- target library paths
- accidental host `/usr/include` or `/usr/lib`
- generated source/header ordering

### Failing Parallel Build

Ninja runs parallel builds aggressively. If a build fails intermittently, suspect missing dependencies around generated files or custom commands.

## Common Mistakes

- Editing `build.ninja` manually instead of fixing `CMakeLists.txt` or `meson.build`.
- Assuming Ninja replaces CMake or Meson.
- Debugging only through the high-level tool and never looking at `ninja -v`.
- Forgetting to regenerate when project files change outside the normal flow.
- Ignoring dependency problems exposed by parallel builds.
- Treating a Ninja failure as a Ninja bug before checking generated dependency rules.
- Depending on generated files without declaring them.
- Assuming `compile_commands.json` exists without enabling it in CMake.
- Calling `ninja` directly in scripts that are meant to work with non-Ninja generators.

## Debugging Checklist

- Run `ninja -C build -v` to see commands.
- Run `ninja -C build -t targets` to inspect available targets.
- Run `ninja -C build -d explain` for rebuild reasoning.
- Check whether `compile_commands.json` exists when tooling needs it.
- Fix dependency graph problems in CMake or Meson, not in `build.ninja`.
- Reconfigure with CMake when generator inputs change.
- Use `ninja -C build -t commands` to inspect generated commands.
- Use `ninja -C build -t deps` to inspect recorded dependencies.
- Use `ninja -C build -d explain` to understand rebuilds.
- Check generated file rules when failures are intermittent.
- Use `cmake --build` for portable automation and direct `ninja` for Ninja-specific diagnostics.

## Related Topics

- [CMake Basics](cmake-basics.md)
- [CMake Toolchain Files](cmake-toolchain-files.md)
- [Native Linux Userspace Builds](native-linux-userspace-builds.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)

## References

- Ninja manual
- CMake generators documentation
- Meson documentation
