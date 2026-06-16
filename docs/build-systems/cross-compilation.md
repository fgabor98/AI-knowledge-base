---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Cross-Compilation

## What Problem Does This Solve?

Cross-compilation builds software on one machine so it can run on another. In embedded Linux, this usually means building on an x86_64 workstation or CI runner and running the result on an ARM, AArch64, or RISC-V board.

This is the point where simple compiler commands become embedded build engineering. You must keep the build machine, target machine, compiler, headers, libraries, and runtime root filesystem aligned.

## Core Concepts

- build machine
- host machine
- target machine
- cross-compiler
- target triple
- ABI
- sysroot
- staging directory
- root filesystem
- host tools vs target artifacts

## Mental Model

For ordinary native builds:

```text
build machine == run machine
```

For cross-builds:

```text
build machine != run machine
```

In GNU build-system vocabulary, the words can be subtle:

- build: the machine doing the compilation
- host: the machine where the built program will run
- target: the machine for which a compiler itself generates code

For normal embedded application cross-compilation, people often casually say "target" for the board where the program runs. Be aware that Autotools and compiler documentation may use these terms more precisely.

For practical embedded Linux application work, keep two columns in your head:

```text
host/build side              target/runtime side
--------------------------   ---------------------------
x86_64 workstation or CI     ARM/AArch64/RISC-V board
host tools                   target applications
host Python, shell, C tools  target shared libraries
host /usr/include            target sysroot headers
host /usr/lib                target sysroot libraries
```

Most cross-compilation bugs come from accidentally mixing those columns.

## Syntax / API / Mechanism

Native build:

```sh
gcc main.c -o app
```

Cross-build:

```sh
arm-linux-gnueabihf-gcc main.c -o app
aarch64-linux-gnu-gcc main.c -o app
```

Inspect the output:

```sh
file app
readelf -h app
```

Use a sysroot:

```sh
arm-linux-gnueabihf-gcc \
  --sysroot=/opt/arm-sysroot \
  -Wall -Wextra \
  main.c \
  -o app
```

With Make:

```sh
make CC=arm-linux-gnueabihf-gcc
```

or:

```sh
make CROSS_COMPILE=arm-linux-gnueabihf-
```

Common inspection commands:

```sh
arm-linux-gnueabihf-gcc -dumpmachine
arm-linux-gnueabihf-gcc -v
file app
readelf -h app
readelf -l app
readelf -d app
```

Deployment commands for quick manual testing:

```sh
scp app root@target:/tmp/
ssh root@target /tmp/app
```

## Minimal Example

`hello.c`:

```c
#include <stdio.h>

int main(void)
{
    puts("hello from target");
    return 0;
}
```

Build for AArch64:

```sh
aarch64-linux-gnu-gcc -Wall -Wextra -O2 hello.c -o hello
file hello
```

Copy to the target and run:

```sh
scp hello root@target:/tmp/
ssh root@target /tmp/hello
```

## Real-World Example

A small board utility might need a target sysroot and target library:

```sh
arm-linux-gnueabihf-gcc \
  --sysroot=/opt/sitara-sysroot \
  -I/opt/sitara-sysroot/usr/include \
  -Wall -Wextra -O2 \
  src/gpio-tool.c \
  -L/opt/sitara-sysroot/usr/lib \
  -lgpiod \
  -o gpio-tool
```

The binary, headers, and libraries must all agree on:

- CPU architecture
- floating-point ABI where relevant
- C library family and version
- kernel/userspace ABI expectations
- runtime library availability on the root filesystem
- dynamic linker path
- vendor SDK or distribution release
- debug symbol and stripped binary handling

In a production workflow, the compiler, sysroot, deployed root filesystem, and debug symbols should be traceable to the same build or SDK release.

## Common Scenarios

### Bare-Metal Toolchain vs Linux Toolchain

These are not interchangeable:

```text
arm-none-eabi       -> bare-metal firmware
arm-linux-gnueabihf -> Linux userspace
```

For embedded Linux applications, use a `*-linux-*` toolchain unless you are deliberately building firmware, boot ROM payloads, or bare-metal code.

Typical split:

```text
Linux userspace app       -> aarch64-linux-gnu-gcc
kernel or U-Boot          -> often a *-linux-* cross compiler
bare-metal MCU firmware   -> arm-none-eabi-gcc
PRU firmware              -> TI PRU toolchain or LLVM flow, platform-dependent
```

Always check the vendor BSP documentation for bootloader, firmware, and heterogeneous-core components.

### Host Tool Needed During A Target Build

A project may build a generator and then use it to produce target source:

```text
gen-table runs on host
app runs on target
```

The generator must be built with the host compiler. The final app must be built with the target compiler. This distinction becomes important in CMake, Yocto, Buildroot, and kernel builds.

A common failure is building the generator with the target compiler and then trying to execute it on the host:

```text
cannot execute binary file: Exec format error
```

That usually means a target binary was executed on the build machine.

### Build Succeeds But Target Cannot Run Binary

Check:

```sh
file app
readelf -h app
readelf -d app
```

Common causes:

- wrong architecture
- wrong ABI
- missing dynamic linker
- missing shared library
- target root filesystem is older than the sysroot used for the build

Check the dynamic linker:

```sh
readelf -l app | grep interpreter
```

Example output:

```text
[Requesting program interpreter: /lib/ld-linux-armhf.so.3]
```

That path must exist on the target root filesystem.

### Build Accidentally Uses Host Dependency

If a cross-build uses `/usr/include` or `/usr/lib` from the workstation, stop and fix the dependency search path. Target headers and target libraries should come from the target sysroot, SDK, or build framework staging area.

### Build Works Locally But Fails In CI

Likely causes:

- local environment has extra host packages installed
- CI does not have the same SDK setup
- toolchain path is not pinned
- sysroot is generated but not restored in CI
- `pkg-config` uses different search paths

Good CI builds should print toolchain versions and key paths early in the log.

### Build Works In CI But Fails On Hardware

Likely causes:

- artifact deployed to the board is not the artifact from CI
- root filesystem on hardware does not match the build sysroot
- shared library missing on the board
- CPU variant or ABI mismatch
- service environment differs from manual shell testing

Always record the binary checksum and image version when testing on hardware.

### Cross-Compile With A Build Framework

Buildroot, Yocto, and TI Processor SDK usually set `CC`, `CFLAGS`, `LDFLAGS`, sysroot, and `pkg-config` environment for you. Inside those systems, avoid overriding toolchain variables unless the package documentation explicitly requires it.

### Use QEMU For Smoke Testing

For some userspace programs, QEMU user-mode can catch basic architecture and dynamic-linker problems before hardware deployment. It is not a replacement for board testing, especially for GPIO, I2C, SPI, timing, device tree, kernel driver, or hardware-dependent behavior.

## Common Mistakes

- Using `gcc` instead of the cross-compiler.
- Using `arm-none-eabi-gcc` for Linux userspace.
- Mixing headers from one SDK with libraries from another.
- Building against a sysroot that does not match the deployed root filesystem.
- Running target binaries directly on the host.
- Using host `pkg-config` metadata during a target build.
- Forgetting that generated host tools must run on the build machine.
- Copying only the executable to the target when it needs new shared libraries too.
- Trusting a manually installed SDK without recording its version.
- Using a sysroot from one board image and deploying to a different board image.
- Assuming QEMU success proves hardware success.

## Debugging Checklist

- Confirm the compiler executable name.
- Run `file app`.
- Run `readelf -h app` and check machine and ABI.
- Run `readelf -d app` and check needed shared libraries and dynamic linker.
- Check every `-I` and `-L` path.
- Check whether `pkg-config` is target-aware.
- Compare the build sysroot with the deployed root filesystem.
- Rebuild from clean output after changing toolchains or sysroots.
- Print the compiler path with `which <compiler>`.
- Record `<compiler> --version` in CI logs.
- Check the program interpreter with `readelf -l app`.
- Check whether the target has every library shown by `readelf -d app`.
- Verify the deployed artifact checksum on the board.
- Check service logs if manual execution works but system startup fails.

## Related Topics

- [Target Triples and Sysroots](target-triples-and-sysroots.md)
- [Target pkg-config](target-pkg-config.md)
- [CMake Toolchain Files](cmake-toolchain-files.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)

## References

- GCC manual
- GNU Autoconf manual
- Yocto Project documentation
- Buildroot manual
