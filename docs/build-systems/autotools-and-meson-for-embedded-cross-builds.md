---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Autotools And Meson For Embedded Cross-Builds

## What Problem Does This Solve?

Many embedded Linux packages are not CMake projects. Older packages often use Autotools, while newer projects increasingly use Meson. A platform engineer must recognize how each system discovers dependencies, handles cross-compilation, and installs into a staging root.

## Core Concepts

- `configure`
- `config.site`
- `--host`
- `--build`
- `DESTDIR`
- Meson cross file
- native file
- `pkg-config`
- sysroot
- feature options

## Autotools Cross-Build Pattern

Typical pattern:

```bash
./configure \
  --build=x86_64-pc-linux-gnu \
  --host=aarch64-linux-gnu \
  --prefix=/usr \
  PKG_CONFIG_SYSROOT_DIR="$SYSROOT" \
  PKG_CONFIG_LIBDIR="$SYSROOT/usr/lib/pkgconfig:$SYSROOT/usr/share/pkgconfig"

make
make DESTDIR="$PWD/stage" install
```

`--host` is the target. `--build` is the machine doing the build. Many cross-build mistakes come from reversing or omitting these.

## Meson Cross-Build Pattern

Example cross file:

```ini
[binaries]
c = 'aarch64-linux-gnu-gcc'
cpp = 'aarch64-linux-gnu-g++'
ar = 'aarch64-linux-gnu-ar'
strip = 'aarch64-linux-gnu-strip'
pkgconfig = 'pkg-config'

[host_machine]
system = 'linux'
cpu_family = 'aarch64'
cpu = 'cortex-a53'
endian = 'little'

[properties]
sys_root = '/opt/sysroots/aarch64'
pkg_config_libdir = '/opt/sysroots/aarch64/usr/lib/pkgconfig'
```

Build:

```bash
meson setup builddir --cross-file cross-aarch64.ini --prefix=/usr
meson compile -C builddir
DESTDIR="$PWD/stage" meson install -C builddir
```

## What To Inspect

For Autotools:

- `config.log`
- `config.status`
- generated `Makefile`
- cached feature checks

For Meson:

- `meson-info/intro-buildoptions.json`
- `meson-logs/meson-log.txt`
- `build.ninja`
- dependency summary

## Embedded Linux Constraints

- tests that execute target binaries must be disabled or handled through emulation
- host `pkg-config` must not leak host libraries
- install paths must match the target filesystem
- optional features should be explicit
- generated tools may need a native compiler while target code needs a cross compiler

## Common Mistakes

- using `--target` when Autotools expects `--host`
- letting `pkg-config` find host `.pc` files
- running target test binaries on the host during configure
- installing directly into the sysroot instead of using `DESTDIR`
- assuming Meson and CMake use identical cross-file concepts

## Related Topics

- [Cross-Compilation](cross-compilation.md)
- [Target pkg-config](target-pkg-config.md)
- [Install Rules and Staging](install-rules-and-staging.md)

