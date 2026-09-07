---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Target Triples, Sysroots, And ABI

## Build roles

For GNU build configuration, **build** is the machine building the package,
**host** is where that package will run, and **target** is the platform for
which compiler tools in that package generate code. For an ordinary application,
the embedded board is its host and `--target` is usually irrelevant. Distinguish
these formal roles from informal “development host / target board” terminology.

A target triple captures architecture, vendor, operating system, and ABI conventions, but it is not the complete contract. Record CPU baseline and extensions, endianness, floating-point ABI, data model, threading model, libc, loader, kernel minimum, and C++ ABI where applicable.

## Sysroot discipline

A sysroot is the target's view of headers, startup objects, libraries, and metadata. Compile and link with the intended sysroot; ensure `pkg-config` uses target `.pc` files and does not return host include or library paths. Keep generated tools and target libraries in separate prefixes.

Do not “fix” a missing target dependency by adding `/usr/include` or `/usr/lib` from the build host. That may produce a binary that links but fails on the target or silently uses the wrong structure layout.

## ABI checks

Check `sizeof` assumptions, alignment, signedness, time/off_t widths, structure packing, symbol versions, exception/RTTI policy, and syscall calling conventions. Compile a small ABI probe as part of the toolchain validation and inspect the final ELF with `file` and `readelf`. The ABI is a compatibility promise across every library boundary, not merely a compiler flag.

## Apply the terminology to two builds

In Autoconf terminology:

| Package being built | Build | Host | Target |
| --- | --- | --- | --- |
| Sensor application built on x86-64 for ARM64 | x86-64 build machine | ARM64 runtime | Usually irrelevant |
| Cross compiler running on x86-64 and emitting ARM64 code | Machine building the compiler | x86-64 compiler runtime | ARM64 generated-code platform |

For the application, an illustrative configure invocation is:

```sh
./configure --build=x86_64-pc-linux-gnu --host=aarch64-linux-gnu
```

The actual canonical names and compiler prefix must match the SDK. Specifying
`--target` on an ordinary application does not substitute for `--host`.
Other build systems sometimes use “host” differently; translate roles before
copying flags. See
[Autoconf target triplets](https://www.gnu.org/software/autoconf/manual/autoconf-2.70/html_node/Specifying-Target-Triplets.html).

## Detect contamination

Inspect the compiler's `-dumpmachine`, `-print-sysroot`, verbose include
search and link command. `PKG_CONFIG_SYSROOT_DIR` adjusts sysrooted paths;
`PKG_CONFIG_LIBDIR` selects the target metadata search directories.
An inherited `PKG_CONFIG_PATH` can still introduce inappropriate metadata.
Prefer the SDK's environment wrapper and inspect the returned flags.

A build-time code generator must run on the build machine even when the
library it generates is for the target. CMake toolchain policy commonly
searches host programs but target headers/libraries/packages; an SDK may
supply the complete file. Never run target probes on the build machine and
silently reuse their failed results as target capabilities.
See [CMake program search roots](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH_MODE_PROGRAM.html).

A sysroot is compile/link input, not a proof of the final rootfs contents.
Verify the packaged runtime separately, especially optional plugins and
minimum libc symbol versions.

## Related topics

- [Stage 15 overview](index.md)
- [ELF and loader diagnosis](../diagnostics-debugging-and-performance/elf-abi-and-loader-diagnostics.md)
