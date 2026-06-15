---
status: draft
reviewed: false
domain: build-systems
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Build Systems for Embedded Linux

## What Problem Does This Solve?

Embedded Linux developers need to build software for a machine that is not usually the development workstation. That means build knowledge must cover more than "compile this file": it must include toolchains, cross-compilation, sysroots, dependency discovery, kernel module builds, root filesystem integration, and reproducible system images.

This roadmap explains how Makefiles, CMake, and related build tools fit together from an embedded Linux point of view.

## Core Concepts

- Direct compiler invocation
- Object files, libraries, and linking
- Make and Makefiles
- CMake as a build-system generator
- Ninja as a low-level build executor
- Cross-compilation
- Toolchains and target triples
- Sysroots
- `pkg-config`
- CMake package discovery
- Install rules and staging
- Patch management
- Build caching
- Build environment isolation
- Kernel module builds
- System build frameworks
- Buildroot
- Yocto and OpenEmbedded
- TI Processor SDK
- Root filesystem image tools
- Update artifact tooling

## Mental Model

Think in layers:

```text
source files
-> compiler and linker commands
-> project-level build tool
-> target sysroot and dependencies
-> package or root filesystem integration
-> final image deployed to hardware
```

A Makefile usually describes concrete build commands directly. CMake describes the project and generates files for another build executor, such as Make or Ninja. Buildroot, Yocto/OpenEmbedded, and vendor SDKs sit above project-level builds and assemble complete embedded Linux systems.

For embedded Linux, always ask:

- What runs on the host?
- What runs on the target?
- Which compiler is being used?
- Which headers and libraries are being found?
- Where will the resulting files be installed?

## Syntax / API / Mechanism

### Direct Compiler Commands

Start by understanding the commands that build systems eventually run:

```sh
gcc -c main.c -o main.o
gcc -c util.c -o util.o
gcc main.o util.o -o app
```

Common options:

- `-I` adds header search paths.
- `-L` adds library search paths.
- `-l` links a library.
- `-D` defines a preprocessor symbol.
- `-Wall -Wextra` enable useful warnings.
- `-g` adds debug information.
- `-O0`, `-O2`, and `-Os` control optimization.

For embedded Linux, the compiler usually changes:

```sh
arm-linux-gnueabihf-gcc main.c -o app
aarch64-linux-gnu-gcc main.c -o app
```

### Make

Make is common in embedded Linux because it is used by the Linux kernel, U-Boot, BusyBox, Buildroot, and many vendor SDKs.

```make
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

app: main.o util.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f app *.o
```

For cross-compilation:

```sh
make CC=arm-linux-gnueabihf-gcc
```

or:

```make
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
```

### CMake

CMake is a build-system generator. It can generate Makefiles, Ninja files, Visual Studio projects, or other backend build files.

```cmake
cmake_minimum_required(VERSION 3.20)
project(app C)

add_executable(app main.c util.c)
```

Build natively:

```sh
cmake -S . -B build
cmake --build build
```

Build with Ninja:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

For embedded Linux, CMake cross-builds normally use a toolchain file:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_SYSROOT /opt/arm-sysroot)
set(CMAKE_FIND_ROOT_PATH /opt/arm-sysroot)
```

Use it with:

```sh
cmake -S . -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake
```

### `pkg-config`

Linux userspace projects often discover compiler and linker flags with `pkg-config`:

```sh
pkg-config --cflags --libs libgpiod
```

In embedded builds, make sure `pkg-config` reads target `.pc` files from the target sysroot, not host library metadata from the workstation.

### Dependency Discovery And Install Rules

Embedded Linux builds need predictable dependency discovery and installation behavior.

Learn:

- `pkg-config` search paths and sysroot behavior
- CMake `find_package`
- CMake imported and exported targets
- install prefixes
- `DESTDIR`
- staging directories
- runtime library paths and dynamic linker paths
- generated SDK sysroots

The goal is to avoid building against host headers or libraries while still producing installable target artifacts.

### Source, Patch, And Cache Management

Complete systems rarely build only first-party code. Learn how sources are fetched, patched, cached, mirrored, and rebuilt.

Important topics:

- source mirrors
- vendored tarballs
- Git submodules and their risks
- patch series
- quilt-style patch flows
- Yocto `.bbappend` patching
- Buildroot package patches
- `ccache`
- Yocto shared state cache
- download caches
- offline and repeatable builds

### Build Quality Gates

Build systems often own quality checks, not just compilation.

Useful integrations:

- compiler warnings as policy
- static analysis
- unit tests
- integration tests
- code coverage
- sanitizers for host-side or emulated tests
- image size checks
- ABI compatibility checks
- license and SBOM checks

### Kernel Module Builds

Out-of-tree kernel modules use the kernel build system:

```make
obj-m += my_driver.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Cross-building a module usually includes `ARCH`, `CROSS_COMPILE`, and the target kernel build directory:

```sh
make -C /path/to/linux \
  ARCH=arm \
  CROSS_COMPILE=arm-linux-gnueabihf- \
  M=$PWD modules
```

### System Build Frameworks

Complete embedded Linux products need tools above Make and CMake. These tools build or assemble the toolchain, bootloader, kernel, root filesystem, packages, images, and sometimes update bundles.

Base summary:

- Buildroot: simpler full-system builder; good first system build framework.
- Yocto/OpenEmbedded: metadata-driven distribution build ecosystem; learn in detail for production BSPs and long-lived products.
- PTXdist: industrial embedded Linux build framework used in some professional environments.
- OpenWrt build system: important for routers, gateways, Wi-Fi devices, and network appliances.
- Vendor BSP build systems: board-vendor workflows that often wrap Yocto/OE, Buildroot, Make, shell scripts, and image tools.
- Root filesystem image tools: `cpio`, `mkfs.ext4`, `mksquashfs`, `mkfs.ubifs`, `ubinize`, and WIC-style partition image generation.
- Update systems: RAUC, SWUpdate, Mender, and OSTree-style update flows.
- Containerized build environments: Docker or Podman used to stabilize host build environments and CI builds.
- Bazel-like hermetic build concepts: useful to understand even when the production system uses Yocto or Buildroot.

Treat Yocto/OpenEmbedded and TI Processor SDK as detailed follow-on topics:

- [Yocto and OpenEmbedded Roadmap](yocto-openembedded-roadmap.md)
- [TI Processor SDK Roadmap](ti-processor-sdk-roadmap.md)

## Minimal Example

Build a small C program manually, then move it into Make:

```sh
gcc -c main.c -o main.o
gcc -c util.c -o util.o
gcc main.o util.o -o app
```

Equivalent Makefile:

```make
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

app: main.o util.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f app *.o
```

Cross-build the same project:

```sh
make clean
make CC=arm-linux-gnueabihf-gcc
```

## Real-World Example

A practical embedded Linux learning sequence:

1. Build a single C file with `gcc`.
2. Split the program into multiple `.c` files and link object files manually.
3. Write a Makefile with `CC`, `CFLAGS`, `clean`, and pattern rules.
4. Cross-compile the Makefile project with a target Linux toolchain.
5. Copy the binary to a board and run it from a serial console or SSH session.
6. Add one real target dependency, such as `libgpiod`, using `pkg-config`.
7. Rewrite the same project in CMake.
8. Add a CMake toolchain file and sysroot configuration.
9. Add install rules and verify staged installation.
10. Build the CMake project with Ninja.
11. Add static analysis or unit tests to the build.
12. Build a simple out-of-tree kernel module.
13. Add the userspace app to Buildroot as a package.
14. Write a Yocto recipe for the same app.
15. Build the app into a Yocto image from a custom layer.
16. Reproduce a TI Processor SDK image for one supported EVM.
17. Add the same app to the TI SDK build through a custom layer.

This progression keeps the project small while adding the constraints that embedded Linux work actually has.

## Common Mistakes

- Treating CMake as a replacement compiler instead of a generator for backend build files.
- Cross-compiling with the correct compiler but accidentally using host headers or host libraries.
- Confusing `arm-none-eabi` bare-metal toolchains with `arm-linux-gnueabi` or `arm-linux-gnueabihf` Linux userspace toolchains.
- Hard-coding compiler names instead of allowing `CC`, `CXX`, `CFLAGS`, and `LDFLAGS` to be overridden.
- Ignoring sysroot configuration until dependency discovery breaks.
- Writing install rules that install into the host filesystem instead of a staging directory or root filesystem.
- Building a kernel module against headers that do not match the target kernel.
- Letting CMake or `pkg-config` find host dependencies during a target build.
- Treating Git submodules or floating source URLs as reproducible dependency management.
- Ignoring generated files, code generators, and host tools in the dependency graph.
- Rebuilding everything because caches, mirrors, or task dependencies are not understood.
- Shipping binaries without matching debug symbols or build provenance.
- Mixing vendor BSP releases, layers, toolchains, and documentation from different versions.
- Treating system image generation as only a filesystem copy operation.

## Debugging Checklist

- Print the compiler command and confirm it is the target compiler.
- Check the target triple, for example `arm-linux-gnueabihf` or `aarch64-linux-gnu`.
- Confirm include paths point into the target sysroot where appropriate.
- Confirm library paths and `pkg-config` metadata come from the target sysroot.
- Use `file app` to confirm the binary architecture.
- Use `readelf -d app` to inspect dynamic library requirements.
- Confirm the target root filesystem contains the required shared libraries.
- For kernel modules, confirm `ARCH`, `CROSS_COMPILE`, and the kernel build directory match the target kernel.
- Rebuild from a clean tree when changing toolchains or sysroots.
- Inspect CMake cache entries when dependency discovery looks wrong.
- Check `DESTDIR`, install prefix, and staging paths before packaging.
- Check whether generated code or host tools need to be rebuilt.
- Confirm cache hits and misses when diagnosing slow builds.
- Preserve debug symbols and build IDs for released binaries.

## Learning Path

### Beginner

1. Direct compiler commands
2. Object files and linking
3. Make basics
4. Make variables and pattern rules
5. Native Linux userspace builds

### Intermediate

1. Cross-compilation
2. Target triples and sysroots
3. `pkg-config`
4. CMake basics
5. CMake toolchain files
6. Ninja as a generated backend
7. Install rules and staging directories
8. CMake package discovery
9. Source patching and dependency vendoring
10. Build caching with `ccache` and framework-specific caches

### Advanced

1. Kernel module builds
2. Buildroot packages
3. Yocto/OpenEmbedded recipes, layers, images, and BitBake tasks
4. TI Processor SDK source builds and image customization
5. Root filesystem and partition image generation
6. Update artifact generation
7. Reproducible and hermetic build concepts
8. CI checks for cross-builds
9. Static analysis, coverage, and sanitizer integration
10. ABI compatibility and symbol management
11. Binary package feeds and SDK generation
12. Build performance, caching, and mirrors

### System Build Framework Survey

Learn the purpose of these even if they are not the main focus:

1. Buildroot
2. PTXdist
3. OpenWrt build system
4. Vendor BSP build flows
5. Root filesystem image tools
6. RAUC, SWUpdate, Mender, and OSTree-style update systems
7. Containerized build environments
8. Bazel-like hermetic build systems and remote cache concepts

## Related Topics

- [Build Systems](index.md)
- [Yocto and OpenEmbedded Roadmap](yocto-openembedded-roadmap.md)
- [TI Processor SDK Roadmap](ti-processor-sdk-roadmap.md)
- [Embedded Linux](../embedded-linux/index.md)
- [Linux Kernel Programming](../linux-kernel/index.md)
- [C Programming](../c/index.md)
- [Topic Map](../topic-map.md)

## References

- GNU Make manual
- CMake documentation
- Ninja manual
- Linux kernel documentation for external modules
- Buildroot manual
- Yocto Project documentation
- TI Processor SDK Linux documentation
