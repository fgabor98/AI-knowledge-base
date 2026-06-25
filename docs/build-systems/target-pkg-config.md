---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Target pkg-config

## What Problem Does This Solve?

`pkg-config` prints compiler and linker flags for installed libraries. In native builds it is convenient. In cross-builds it is dangerous unless it is configured to read target metadata from the target sysroot.

This page explains how to think about `pkg-config` when building embedded Linux userspace software.

## Core Concepts

- `.pc` file
- compiler flags
- linker flags
- `PKG_CONFIG_PATH`
- `PKG_CONFIG_LIBDIR`
- `PKG_CONFIG_SYSROOT_DIR`
- sysroot-aware dependency discovery
- host dependency vs target dependency

## Mental Model

`pkg-config` does not discover libraries by magic. It reads `.pc` metadata files.

Example:

```sh
pkg-config --cflags --libs libgpiod
```

Output may look like:

```text
-I/usr/include -lgpiod
```

For a native build, that may be fine. For a target build, `/usr/include` means the host unless `pkg-config` is configured for the target sysroot.

Think of `pkg-config` as a metadata reader. If it reads host metadata, it gives host flags. If it reads target metadata, it gives target flags. The command name alone does not make it target-aware.

## Syntax / API / Mechanism

Common commands:

```sh
pkg-config --modversion libgpiod
pkg-config --cflags libgpiod
pkg-config --libs libgpiod
pkg-config --cflags --libs libgpiod
```

Common environment variables:

```sh
export PKG_CONFIG_SYSROOT_DIR=/opt/target-sysroot
export PKG_CONFIG_LIBDIR=/opt/target-sysroot/usr/lib/pkgconfig:/opt/target-sysroot/usr/share/pkgconfig
```

Variable roles:

- `PKG_CONFIG_PATH` adds extra search paths on top of defaults.
- `PKG_CONFIG_LIBDIR` replaces the default search path.
- `PKG_CONFIG_SYSROOT_DIR` prepends a sysroot to suitable absolute paths in `.pc` output.

For cross-builds, `PKG_CONFIG_LIBDIR` is often safer than `PKG_CONFIG_PATH` because it prevents fallback to host `.pc` directories.

Then:

```sh
pkg-config --cflags --libs libgpiod
```

In Make:

```make
CPPFLAGS += $(shell pkg-config --cflags libgpiod)
LDLIBS += $(shell pkg-config --libs libgpiod)
```

This is only correct for cross-builds if the environment points at target `.pc` files.

Anatomy of a simple `.pc` file:

```pkgconfig
prefix=/usr
includedir=${prefix}/include
libdir=${prefix}/lib

Name: libexample
Description: Example library
Version: 1.0
Cflags: -I${includedir}
Libs: -L${libdir} -lexample
Requires.private: zlib
```

Important fields:

- `Cflags` are compile flags.
- `Libs` are public link flags.
- `Requires` pulls public dependencies.
- `Requires.private` matters for static linking.
- `Libs.private` may add extra libraries for static linking.

## Minimal Example

Native:

```sh
pkg-config --cflags --libs zlib
gcc main.c $(pkg-config --cflags --libs zlib) -o app
```

Target-aware environment:

```sh
export SYSROOT=/opt/arm-sysroot
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
export PKG_CONFIG_LIBDIR="$SYSROOT/usr/lib/pkgconfig:$SYSROOT/usr/share/pkgconfig"

arm-linux-gnueabihf-gcc \
  --sysroot="$SYSROOT" \
  main.c \
  $(pkg-config --cflags --libs libgpiod) \
  -o gpio-tool
```

## Real-World Example

A board utility depends on `libgpiod`. The host workstation also has `libgpiod` installed. If `pkg-config` reads host metadata, the build may use host include paths or host library versions.

The correct setup should make `pkg-config` report paths inside the target sysroot or sysroot-adjusted paths:

```sh
PKG_CONFIG_SYSROOT_DIR=/opt/sitara-sysroot \
PKG_CONFIG_LIBDIR=/opt/sitara-sysroot/usr/lib/pkgconfig \
pkg-config --cflags --libs libgpiod
```

For static linking checks:

```sh
pkg-config --static --libs libgpiod
```

## Common Scenarios

### Host `.pc` File Accidentally Used

Symptom: the build command contains:

```text
-I/usr/include
-L/usr/lib
```

during a target build. This is a warning sign. Check `PKG_CONFIG_LIBDIR` and `PKG_CONFIG_PATH`.

Also inspect where the package came from:

```sh
pkg-config --debug --cflags --libs libgpiod
```

### `.pc` File Exists But Points Outside The Sysroot

Some `.pc` files contain absolute prefixes. `PKG_CONFIG_SYSROOT_DIR` can rewrite paths, but only if the metadata is compatible with sysroot use.

Example:

```text
prefix=/usr
Cflags: -I${prefix}/include
```

With `PKG_CONFIG_SYSROOT_DIR=/opt/sysroot`, output can become:

```text
-I/opt/sysroot/usr/include
```

If a `.pc` file hard-codes non-target paths such as `/home/user/build/tmp`, fix the package metadata or use the build framework's generated sysroot.

### Library Available But No `.pc` File

Not every library ships pkg-config metadata. In that case, you may need explicit `-I`, `-L`, and `-l` flags, a CMake package file, or build-framework-specific dependency handling.

Do not invent a `.pc` file for production without understanding the library's public and private dependencies.

### Yocto Or Buildroot Handles It For You

Yocto and Buildroot often set the correct `pkg-config` environment inside package builds. Do not override it casually. If a recipe or package build fails, inspect the environment first.

### Static vs Dynamic Linking

Dynamic linking may need fewer flags:

```sh
pkg-config --libs libfoo
```

Static linking may need private dependencies:

```sh
pkg-config --static --libs libfoo
```

If static linking fails with missing symbols from dependency libraries, compare these two outputs.

### Multiple Sysroots Installed

If you work with several SDKs, stale shell environment can point to the wrong sysroot. Print the environment before debugging the compiler:

```sh
env | grep '^PKG_CONFIG'
```

### CMake Uses pkg-config Internally

A CMake project can call `pkg-config` through `FindPkgConfig`. The same target environment rules apply. A correct CMake toolchain file does not automatically fix an incorrect `PKG_CONFIG_LIBDIR`.

## Common Mistakes

- Using host `pkg-config` output in a cross-build.
- Setting `PKG_CONFIG_PATH` when `PKG_CONFIG_LIBDIR` is the safer cross-build choice.
- Forgetting `PKG_CONFIG_SYSROOT_DIR`.
- Trusting `.pc` files copied from random target root filesystems.
- Mixing `.pc` files from one SDK with libraries from another.
- Hard-coding `pkg-config` results into a Makefile.
- Forgetting `--static` when diagnosing static link failures.
- Assuming CMake use means `pkg-config` is irrelevant.
- Leaving old `PKG_CONFIG_PATH` values in the shell.

## Debugging Checklist

- Run `pkg-config --variable=pc_path pkg-config`.
- Print `PKG_CONFIG_PATH`, `PKG_CONFIG_LIBDIR`, and `PKG_CONFIG_SYSROOT_DIR`.
- Use `pkg-config --debug --cflags --libs <name>` when needed.
- Inspect the actual `.pc` file.
- Check whether output paths belong to the host or target sysroot.
- Confirm the final compiler command uses the target compiler.
- Confirm linked libraries exist in the deployed root filesystem.
- Compare `pkg-config --libs <name>` and `pkg-config --static --libs <name>`.
- Search the output for host paths.
- Check whether `.pc` files refer to paths outside the active SDK or sysroot.
- In Yocto/Buildroot builds, inspect the package build log before overriding environment variables.

## Related Topics

- [Cross-Compilation](cross-compilation.md)
- [Target Triples and Sysroots](target-triples-and-sysroots.md)
- [Make Variables and Pattern Rules](make-variables-and-pattern-rules.md)
- [CMake Basics](cmake-basics.md)

## References

- pkg-config manual
- freedesktop.org pkg-config guide
- Yocto Project documentation
- Buildroot manual
