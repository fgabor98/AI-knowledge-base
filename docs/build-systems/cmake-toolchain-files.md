---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# CMake Toolchain Files

## What Problem Does This Solve?

CMake needs to know about the target platform before it configures a cross-build. A toolchain file tells CMake which compiler to use, what system is being targeted, where the sysroot is, and how dependency discovery should behave.

For embedded Linux, a toolchain file is the difference between a controlled target build and a native build that accidentally uses host dependencies.

## Core Concepts

- `CMAKE_TOOLCHAIN_FILE`
- `CMAKE_SYSTEM_NAME`
- `CMAKE_SYSTEM_PROCESSOR`
- `CMAKE_C_COMPILER`
- `CMAKE_CXX_COMPILER`
- `CMAKE_SYSROOT`
- `CMAKE_FIND_ROOT_PATH`
- find root path modes
- target dependency discovery

## Mental Model

CMake detects compilers and platform behavior during the configure step. For cross-compilation, you must provide target facts before that detection happens:

```sh
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake
```

Do not switch a build directory from native to cross in place. Create a fresh build directory when changing the toolchain.

The toolchain file is read very early. It should describe the target platform and toolchain, not ordinary project options. Keep project feature toggles in normal CMake cache variables or presets, not in the toolchain file.

## Syntax / API / Mechanism

Example `toolchain-arm.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_SYSROOT /opt/arm-sysroot)
set(CMAKE_FIND_ROOT_PATH /opt/arm-sysroot)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

Find mode meaning:

- `PROGRAM`: where CMake searches for executables that run during the build.
- `LIBRARY`: where CMake searches for link libraries.
- `INCLUDE`: where CMake searches for headers.
- `PACKAGE`: where CMake searches for CMake package config files.

For cross-builds, programs often come from the host, while libraries, includes, and packages come from the target sysroot.

Configure:

```sh
cmake -S . -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake
```

Build:

```sh
cmake --build build-arm
```

Verbose build:

```sh
cmake --build build-arm --verbose
```

Inspect cache:

```sh
grep -E 'CMAKE_(C|CXX)_COMPILER|CMAKE_SYSROOT|CMAKE_FIND_ROOT_PATH' build-arm/CMakeCache.txt
```

## Minimal Example

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_SYSROOT /opt/aarch64-sysroot)
```

Use:

```sh
cmake -S . -B build-aarch64 -DCMAKE_TOOLCHAIN_FILE=aarch64.cmake
cmake --build build-aarch64
file build-aarch64/app
```

## Real-World Example

A target build using a generated SDK sysroot:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(SDK_ROOT /opt/ti-sdk)
set(TARGET_SYSROOT ${SDK_ROOT}/sysroots/armv7at2hf-neon-linux-gnueabi)

set(CMAKE_C_COMPILER ${SDK_ROOT}/sysroots/x86_64-linux/usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${SDK_ROOT}/sysroots/x86_64-linux/usr/bin/arm-linux-gnueabihf-g++)

set(CMAKE_SYSROOT ${TARGET_SYSROOT})
set(CMAKE_FIND_ROOT_PATH ${TARGET_SYSROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

Exact paths depend on the SDK. The important point is that compilers and target sysroot come from the same SDK release.

If the SDK provides an environment setup script, source it before configuring unless the SDK documentation says otherwise. Then record the environment in CI logs.

## Common Scenarios

### CMake Finds Host Libraries

If `find_package` finds host libraries, check:

```cmake
CMAKE_FIND_ROOT_PATH_MODE_LIBRARY
CMAKE_FIND_ROOT_PATH_MODE_INCLUDE
CMAKE_FIND_ROOT_PATH_MODE_PACKAGE
```

For target libraries, these are often set to `ONLY`.

Search the cache:

```sh
grep -R '/usr/lib\\|/usr/include' build-arm/CMakeCache.txt
```

Host paths in target dependency variables are warning signs.

### Host Programs Still Need To Be Found

Code generators that run during the build usually need host programs. That is why `CMAKE_FIND_ROOT_PATH_MODE_PROGRAM` is often `NEVER`: programs are searched on the host, libraries and headers in the target sysroot.

If a build needs both host tools and target packages with the same name, the project may need more explicit paths or a two-stage build.

### Toolchain File Changed But Build Still Uses Old Compiler

CMake caches compiler decisions. Delete the build directory and configure again:

```sh
rm -rf build-arm
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake
```

Changing `CMAKE_C_COMPILER` inside an existing build tree is not reliable. Treat the compiler as immutable for a build directory.

### pkg-config Inside CMake

If a CMake project uses `PkgConfig`, the environment still needs to point to target `.pc` files:

```sh
PKG_CONFIG_SYSROOT_DIR=/opt/arm-sysroot \
PKG_CONFIG_LIBDIR=/opt/arm-sysroot/usr/lib/pkgconfig \
cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake
```

### `try_compile` And `try_run`

CMake projects may test compiler or platform features during configuration.

- `try_compile` can usually work in cross-builds because it only builds.
- `try_run` is a problem because it tries to execute a target binary on the build machine.

Cross-aware projects avoid `try_run`, provide cached answers, or use an emulator.

### CMAKE_STAGING_PREFIX

Some cross-build workflows use a staging prefix for install results:

```cmake
set(CMAKE_STAGING_PREFIX /tmp/stage)
```

This is distinct from the target runtime install prefix. Use it carefully and follow the package framework's conventions.

### CMake Presets

Projects can wrap toolchain file usage in `CMakePresets.json`:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "arm-release",
      "generator": "Ninja",
      "binaryDir": "build-arm",
      "toolchainFile": "toolchain-arm.cmake",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    }
  ]
}
```

Presets can make repeated cross-build commands less error-prone.

## Common Mistakes

- Passing the toolchain file after configuring once natively in the same build directory.
- Setting only `CMAKE_C_COMPILER` and forgetting sysroot and find behavior.
- Letting `find_package` discover host packages.
- Using compilers from one SDK and sysroot from another.
- Hard-coding local absolute paths in a toolchain file that should be shared.
- Assuming every CMake project respects cross-compilation correctly.
- Putting project feature options into the toolchain file.
- Letting `try_run` silently produce bad cached assumptions.
- Forgetting to configure target-aware `pkg-config`.
- Using target sysroot paths for programs that must run on the host.
- Using a toolchain file with unportable absolute paths in shared repositories.

## Debugging Checklist

- Inspect `build/CMakeCache.txt`.
- Check `CMAKE_C_COMPILER`.
- Check `CMAKE_SYSROOT`.
- Check `CMAKE_FIND_ROOT_PATH`.
- Build with `cmake --build build --verbose`.
- Search the cache for host paths such as `/usr/lib` or `/usr/include`.
- Delete the build directory after changing the toolchain file.
- Confirm output with `file` and `readelf`.
- Check for `try_run` failures or warnings in configure output.
- Check `CMakeError.log` and `CMakeOutput.log`.
- Check whether `find_package` results point inside the sysroot.
- Confirm host tools found by CMake are executable on the build machine.
- Verify staged install paths before integrating with Yocto or Buildroot.

## Related Topics

- [CMake Basics](cmake-basics.md)
- [Cross-Compilation](cross-compilation.md)
- [Target Triples and Sysroots](target-triples-and-sysroots.md)
- [Target pkg-config](target-pkg-config.md)

## References

- CMake `cmake-toolchains(7)`
- CMake `find_package` documentation
- Yocto Project SDK documentation
- Buildroot manual
