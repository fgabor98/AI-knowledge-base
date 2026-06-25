---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# CMake Package Discovery

## What Problem Does This Solve?

CMake package discovery finds headers, libraries, compile definitions, and transitive dependencies for external packages. In embedded Linux, this must happen against the target sysroot, not the host workstation.

This topic explains `find_package`, imported targets, `Find*.cmake` modules, package config files, and the common ways dependency discovery goes wrong in cross-builds.

## Core Concepts

- `find_package`
- module mode
- config mode
- `FindFoo.cmake`
- `FooConfig.cmake`
- imported target
- `CMAKE_PREFIX_PATH`
- `CMAKE_MODULE_PATH`
- `CMAKE_FIND_ROOT_PATH`
- package version
- target sysroot discovery

## Mental Model

CMake package discovery should produce usable targets:

```cmake
find_package(Foo REQUIRED)
target_link_libraries(app PRIVATE Foo::Foo)
```

The imported target should carry:

- include directories
- library paths
- compile definitions
- transitive link dependencies
- sometimes compile features or options

Raw variables such as `FOO_INCLUDE_DIRS` and `FOO_LIBRARIES` still exist in older packages, but imported targets are usually safer.

## Syntax / API / Mechanism

Basic package lookup:

```cmake
find_package(ZLIB REQUIRED)
target_link_libraries(app PRIVATE ZLIB::ZLIB)
```

Config mode:

```cmake
find_package(Foo CONFIG REQUIRED)
target_link_libraries(app PRIVATE Foo::Foo)
```

Module path extension:

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Foo REQUIRED)
```

Search prefix:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/sdk/sysroot/usr
```

Cross-builds should usually rely on the toolchain file and sysroot find modes instead of ad hoc host paths.

## Minimal Example

```cmake
cmake_minimum_required(VERSION 3.20)
project(zlib_example C)

find_package(ZLIB REQUIRED)

add_executable(app main.c)
target_link_libraries(app PRIVATE ZLIB::ZLIB)
```

Configure:

```sh
cmake -S . -B build
cmake --build build --verbose
```

Inspect the verbose command for include and library paths.

## Real-World Example

Using `pkg-config` through CMake:

```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod)

add_executable(gpio-tool src/main.c)
target_link_libraries(gpio-tool PRIVATE PkgConfig::GPIOD)
```

For a cross-build, the shell environment must still point `pkg-config` at target `.pc` files:

```sh
PKG_CONFIG_SYSROOT_DIR=/opt/sitara-sysroot \
PKG_CONFIG_LIBDIR=/opt/sitara-sysroot/usr/lib/pkgconfig \
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake
```

## Common Scenarios

### Module Mode vs Config Mode

Module mode uses CMake's `FindFoo.cmake` files:

```cmake
find_package(Foo REQUIRED)
```

Config mode uses package-provided files:

```text
FooConfig.cmake
FooTargets.cmake
```

Config mode often provides better imported targets because the package knows how it was built.

### Host Package Accidentally Found

Symptoms:

- `CMakeCache.txt` contains `/usr/lib` or `/usr/include`
- verbose build output uses host paths
- cross-link fails with architecture mismatch

Check:

```sh
grep -R '/usr/lib\\|/usr/include' build/CMakeCache.txt
```

### `CMAKE_PREFIX_PATH`

`CMAKE_PREFIX_PATH` tells CMake where to look for package prefixes:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/foo
```

CMake may search:

```text
/opt/foo/lib/cmake/Foo/
/opt/foo/share/Foo/
/opt/foo/include/
/opt/foo/lib/
```

For embedded target packages, this prefix should point inside the target sysroot or SDK, not a host install.

### Imported Target Not Available

Older find modules may only provide variables:

```cmake
target_include_directories(app PRIVATE ${FOO_INCLUDE_DIRS})
target_link_libraries(app PRIVATE ${FOO_LIBRARIES})
```

This works, but it is easier to lose transitive dependencies. Prefer imported targets when available.

### Version Constraints

```cmake
find_package(Foo 2.1 REQUIRED)
```

Version checks are useful, but only if the package metadata is correct and comes from the target SDK being used.

## Common Mistakes

- Adding host paths to `CMAKE_PREFIX_PATH` during a target build.
- Assuming `find_package` always uses `pkg-config`.
- Assuming `pkg-config` environment is irrelevant to CMake.
- Using raw include and library variables when a package provides imported targets.
- Shipping a library without installing its `*Config.cmake` or `.pc` metadata.
- Reusing a build directory after changing dependency paths.
- Hiding dependency discovery failures by manually adding broad `-I` and `-L` paths.

## Debugging Checklist

- Inspect `CMakeCache.txt`.
- Build with `cmake --build build --verbose`.
- Search the cache for host paths.
- Check whether the package was found in module mode or config mode.
- Check whether an imported target exists.
- Check `CMAKE_PREFIX_PATH`, `CMAKE_MODULE_PATH`, and `CMAKE_FIND_ROOT_PATH`.
- For `PkgConfig`, print `PKG_CONFIG_LIBDIR` and `PKG_CONFIG_SYSROOT_DIR`.
- Delete the build directory after changing package paths.

## Related Topics

- [CMake Basics](cmake-basics.md)
- [CMake Toolchain Files](cmake-toolchain-files.md)
- [Target pkg-config](target-pkg-config.md)
- [Target Triples and Sysroots](target-triples-and-sysroots.md)

## References

- CMake `find_package` documentation
- CMake `cmake-packages(7)`
- CMake `cmake-buildsystem(7)`
- pkg-config manual
