---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# CMake Basics

## What Problem Does This Solve?

CMake describes a project and generates build files for another tool, such as Make or Ninja. It is useful when a C or C++ project needs portability, structured targets, install rules, generated configuration, tests, or integration with IDEs and packaging systems.

For embedded Linux developers, CMake is common in third-party userspace libraries and applications, and it is frequently wrapped by Yocto, Buildroot, and vendor SDKs.

## Core Concepts

- `CMakeLists.txt`
- configure step
- generate step
- build step
- build directory
- generator
- target
- executable target
- library target
- include directories
- link libraries
- install rules

## Mental Model

CMake is not the compiler and not usually the final build executor.

```text
CMakeLists.txt
-> cmake configure/generate
-> Makefiles or Ninja files
-> compiler and linker commands
```

Modern CMake is target-oriented. Instead of setting many global flags, prefer describing what each target needs:

```cmake
target_include_directories(app PRIVATE include)
target_link_libraries(app PRIVATE gpiod)
```

Think in three layers:

```text
configure: read CMakeLists.txt, detect tools, write cache
generate:  write backend files for Ninja, Make, or another generator
build:     run compiler/linker commands through the backend
```

Most CMake confusion comes from mixing those phases. If the compiler or dependency path is wrong, fix the configure inputs and recreate the build directory. If a source file fails to compile, inspect the build command.

## Syntax / API / Mechanism

Minimal project:

```cmake
cmake_minimum_required(VERSION 3.20)
project(app C)

add_executable(app main.c util.c)
```

Configure:

```sh
cmake -S . -B build
```

Build:

```sh
cmake --build build
```

Choose Ninja:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Common target commands:

```cmake
add_executable(app main.c)
add_library(util util.c)
target_include_directories(app PRIVATE include)
target_link_libraries(app PRIVATE util)
target_compile_options(app PRIVATE -Wall -Wextra)
target_compile_definitions(app PRIVATE _GNU_SOURCE)
```

Visibility keywords:

- `PRIVATE`: needed only to build this target.
- `PUBLIC`: needed by this target and by targets that link to it.
- `INTERFACE`: needed only by consumers, not by this target itself.

For example, a library's public headers should usually be exposed as `PUBLIC` or `INTERFACE`, while warning flags are often `PRIVATE`.

## Minimal Example

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(hello C)

add_executable(hello hello.c)

target_compile_options(hello PRIVATE -Wall -Wextra)
```

Build:

```sh
cmake -S . -B build
cmake --build build
./build/hello
```

Inspect the generated build:

```sh
cmake --build build --verbose
```

## Real-World Example

A small userspace tool split into modules:

```cmake
cmake_minimum_required(VERSION 3.20)
project(board_tool C)

add_executable(board-tool
    src/main.c
    src/gpio.c
    src/config.c
)

target_include_directories(board-tool PRIVATE include)
target_compile_options(board-tool PRIVATE -Wall -Wextra)
target_link_libraries(board-tool PRIVATE gpiod)

install(TARGETS board-tool RUNTIME DESTINATION bin)
```

Build and stage install:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /usr --component Runtime
```

For staging, use:

```sh
DESTDIR="$PWD/stage" cmake --install build --prefix /usr
```

Inspect staged output:

```sh
find stage -type f -print
```

## Common Scenarios

### Out-Of-Source Builds

Prefer:

```sh
cmake -S . -B build
```

Avoid generating build files directly into the source tree. Separate build directories allow Debug and Release builds side by side and make cleanup simpler.

Useful layout:

```text
project/
  CMakeLists.txt
  src/
  include/
  build-debug/
  build-release/
  build-arm/
```

### Debug vs Release

Single-config generators such as Make and Ninja use:

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
```

Do not assume `CMAKE_BUILD_TYPE` affects every generator the same way. Multi-config generators handle configurations differently.

Common values:

- `Debug`
- `Release`
- `RelWithDebInfo`
- `MinSizeRel`

For embedded Linux release builds, `RelWithDebInfo` can be useful because optimized binaries still carry debug information that can be split and archived.

### Link To A Library Target

Prefer linking to a CMake target when available:

```cmake
target_link_libraries(app PRIVATE SomePackage::SomeLibrary)
```

This carries include directories, compile definitions, and link requirements more reliably than raw `-I` and `-l` flags.

Raw library names still exist:

```cmake
target_link_libraries(app PRIVATE gpiod)
```

but imported targets are better when the package provides them:

```cmake
target_link_libraries(app PRIVATE Pkg::GPIOD)
```

### Install Rules Matter

Buildroot, Yocto, package managers, and SDK packaging rely on install rules. A project that builds but cannot install cleanly is not ready for system integration.

### Generated Configuration Headers

CMake can generate configuration headers:

```cmake
configure_file(config.h.in config.h)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
```

Remember that generated headers live in the build directory, not the source directory.

### Options

Expose build choices as options:

```cmake
option(ENABLE_TESTS "Build tests" ON)

if(ENABLE_TESTS)
    enable_testing()
endif()
```

For package builds, options are easier to control than editing source files.

### Tests

Basic test wiring:

```cmake
enable_testing()
add_executable(test-parser tests/test_parser.c src/parser.c)
add_test(NAME parser COMMAND test-parser)
```

Run:

```sh
ctest --test-dir build --output-on-failure
```

For cross-builds, tests may need an emulator, a target board, or may need to be disabled during package builds.

### Dependency Discovery

CMake can find dependencies through:

- built-in `Find*.cmake` modules
- package config files such as `FooConfig.cmake`
- `pkg-config` through `FindPkgConfig`
- manually supplied paths

Prefer package-provided imported targets where available. Be especially careful during cross-builds: dependency discovery must search the target sysroot, not the host.

### Cache Variables

CMake stores configure results in `CMakeCache.txt`. This is useful, but stale values can mislead you after changing compilers, sysroots, or dependency paths.

When in doubt:

```sh
rm -rf build
cmake -S . -B build
```

## Common Mistakes

- Thinking CMake builds directly instead of generating backend build files.
- Editing generated files inside the build directory.
- Using global include directories and flags when target-specific commands are better.
- Forgetting install rules.
- Mixing source and build directories.
- Reusing a build directory after changing compilers or toolchain files.
- Assuming a CMake project is cross-compile-ready just because it builds natively.
- Using `include_directories()` globally when `target_include_directories()` would be clearer.
- Using `link_libraries()` globally and accidentally affecting unrelated targets.
- Setting `CMAKE_C_FLAGS` for target-specific needs instead of using target commands.
- Forgetting generated header directories.
- Running tests during cross-builds when the test binaries cannot run on the host.
- Changing cache variables without reconfiguring from a clean directory.

## Debugging Checklist

- Inspect `CMakeCache.txt`.
- Check the selected generator.
- Check `CMAKE_C_COMPILER`.
- Check `CMAKE_BUILD_TYPE` for Make/Ninja builds.
- Build with verbose output: `cmake --build build --verbose`.
- Delete and recreate the build directory after changing compilers or toolchain files.
- Check whether dependencies are represented as targets or raw flags.
- Verify `cmake --install` into a staging directory.
- Search `CMakeCache.txt` for unexpected host paths.
- Use `cmake -LAH build` to inspect cache variables.
- Run `ctest --test-dir build --output-on-failure` for native test builds.
- Check whether generated files are in the build tree and included correctly.
- Use separate build directories for native, debug, release, and cross builds.

## Related Topics

- [CMake Toolchain Files](cmake-toolchain-files.md)
- [Ninja as a Generated Backend](ninja-generated-backend.md)
- [Target pkg-config](target-pkg-config.md)
- [Native Linux Userspace Builds](native-linux-userspace-builds.md)

## References

- CMake documentation
- CMake `cmake-buildsystem(7)`
- CMake `cmake-packages(7)`
- CMake `cmake-toolchains(7)`
