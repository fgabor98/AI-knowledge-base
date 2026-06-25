---
status: draft
reviewed: false
domain: build-systems
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Direct Compiler Invocation

## What Problem Does This Solve?

Before using Make, CMake, Yocto, or any other build system, you need to understand the compiler and linker commands those tools eventually run.

Direct compiler invocation teaches the smallest useful build loop:

```text
source file -> compiler -> executable
```

For embedded Linux developers, this is the foundation for understanding cross-compilers, sysroots, library paths, build logs, and why a build may accidentally use host headers or libraries.

## Core Concepts

- compiler driver
- source files
- executable output
- compiler warnings
- optimization level
- debug information
- include paths
- library paths
- preprocessor definitions
- host build vs target build

## Mental Model

The command you type is usually a compiler driver, such as `gcc` or `clang`. It coordinates several lower-level steps:

```text
preprocess -> compile -> assemble -> link
```

For a single-file program, the compiler driver hides most of that complexity:

```sh
gcc main.c -o app
```

For an embedded target, the command often looks similar, but the compiler is different:

```sh
arm-linux-gnueabihf-gcc main.c -o app
```

The build command is not only "compile this C file." It also decides the target architecture, ABI, include search paths, library search paths, optimization level, debug format, and final executable name.

When reading a build log, separate the command into questions:

- Which compiler driver is being executed?
- Is this a compile-only step or a compile-and-link step?
- Which source files are inputs?
- Which headers can be found through `-I` paths?
- Which libraries can be found through `-L` and `-l`?
- Which architecture and ABI will the output use?
- Is the output meant to run on the host or on a target board?

That habit scales from one `gcc` command to Make, CMake, kernel builds, Yocto recipes, and TI Processor SDK logs.

## Syntax / API / Mechanism

Common compiler-driver options:

- `-c` compiles to an object file and does not link.
- `-o app` writes the output to `app`.
- `-Wall -Wextra` enable useful diagnostics.
- `-Werror` turns warnings into errors; useful in CI, sometimes too strict while porting.
- `-g` includes debug information.
- `-O0` disables optimization for easier debugging.
- `-O2` enables common production optimization.
- `-Os` optimizes for size.
- `-std=c11` or similar selects a language standard.
- `-DNAME=value` defines a preprocessor symbol.
- `-Ipath` adds a header search path.
- `-Lpath` adds a library search path.
- `-lname` links a library named `libname.so` or `libname.a`.
- `--sysroot=path` makes the compiler search for target headers and libraries under a sysroot.
- `-v` prints detailed compiler-driver behavior and search paths.

Examples:

```sh
gcc -Wall -Wextra -g main.c -o app
gcc -O2 -DNDEBUG main.c -o app
gcc -Iinclude main.c -o app
gcc main.c -L/usr/local/lib -lgpiod -o app
```

Compile without linking:

```sh
gcc -Wall -Wextra -c main.c -o main.o
```

Show preprocessing output:

```sh
gcc -E main.c
```

Show assembly output:

```sh
gcc -S main.c -o main.s
```

Cross-compiler examples:

```sh
arm-linux-gnueabihf-gcc main.c -o app
aarch64-linux-gnu-gcc main.c -o app
```

Common build modes:

```sh
gcc -Wall -Wextra -O0 -g main.c -o app-debug
gcc -Wall -Wextra -O2 -DNDEBUG main.c -o app-release
gcc -Wall -Wextra -Os main.c -o app-small
```

For embedded Linux, `-Os` is common for small root filesystems, while `-O2 -g` is common for release builds that still preserve useful debug information.

## Minimal Example

Create `hello.c`:

```c
#include <stdio.h>

int main(void)
{
    puts("hello");
    return 0;
}
```

Build it:

```sh
gcc -Wall -Wextra -g hello.c -o hello
```

Run it:

```sh
./hello
```

Inspect the result:

```sh
file hello
```

Optional inspections:

```sh
readelf -h hello
readelf -d hello
```

## Real-World Example

A small embedded Linux userspace utility often starts like this:

```sh
gcc -Wall -Wextra -O2 -D_GNU_SOURCE \
  -Iinclude \
  src/gpio-toggle.c \
  -lgpiod \
  -o gpio-toggle
```

When cross-compiling, the compiler and dependency paths must match the target:

```sh
arm-linux-gnueabihf-gcc -Wall -Wextra -O2 \
  --sysroot=/opt/target-sysroot \
  -I/opt/target-sysroot/usr/include \
  src/gpio-toggle.c \
  -L/opt/target-sysroot/usr/lib \
  -lgpiod \
  -o gpio-toggle
```

This kind of command is verbose. Build systems exist partly to generate and repeat these commands correctly.

## Common Scenarios

### Build A Debug Binary

Use this when you will run the program under `gdb` or inspect a core dump:

```sh
gcc -Wall -Wextra -O0 -g main.c -o app
```

`-O0` keeps the generated machine code closer to the source. `-g` adds debug information. For larger projects, you may still debug optimized code, but beginner debugging is easier with `-O0 -g`.

### Build A Release Binary

Use optimization and keep warnings enabled:

```sh
gcc -Wall -Wextra -O2 -DNDEBUG main.c -o app
```

`-DNDEBUG` disables standard `assert` checks. Do not add it casually if assertions catch real field failures.

### Build Against A Header In The Project

If the source includes `"driver.h"` from an `include/` directory:

```sh
gcc -Iinclude -Wall -Wextra src/main.c -o app
```

Header search order matters. A local project header may accidentally shadow a system header if names are too generic.

### Build Against A Library

If the program uses `libgpiod`, the compile and link command may need:

```sh
gcc -Wall -Wextra main.c -lgpiod -o gpio-tool
```

If the headers or libraries are in non-standard locations:

```sh
gcc -I/opt/gpiod/include main.c -L/opt/gpiod/lib -lgpiod -o gpio-tool
```

For target builds, those paths must point to target artifacts, not host artifacts.

### Cross-Compile A Simple Program

```sh
arm-linux-gnueabihf-gcc -Wall -Wextra main.c -o app
file app
```

The output should report an ARM binary. You usually cannot run this binary directly on an x86 development machine.

### Use A Sysroot

A sysroot is a directory that looks like the target root filesystem from the compiler's point of view:

```sh
arm-linux-gnueabihf-gcc --sysroot=/opt/arm-sysroot main.c -o app
```

This helps the compiler find target headers and libraries consistently. It does not automatically guarantee that every manually added `-I` or `-L` path is correct.

### Inspect A Failing Build Command

If a larger build system fails, copy the failing compiler command and simplify it. Remove unrelated flags only after preserving the original command somewhere. The goal is to find the smallest command that still reproduces the error.

## Common Mistakes

- Running `gcc` when the binary must run on an ARM or AArch64 target.
- Forgetting `-o` and wondering why the executable is named `a.out`.
- Ignoring warnings instead of compiling with `-Wall -Wextra`.
- Treating warnings as harmless when they indicate ABI, format-string, or missing-prototype bugs.
- Using host include paths during a target build.
- Using host libraries during a target build.
- Adding `-I` and `-L` paths without understanding whether they belong to the host or target.
- Assuming `-O2` and `-g` cannot be used together.
- Using `ld` directly as a beginner instead of the compiler driver; the compiler driver passes needed runtime startup files and default libraries.
- Linking a library before the object that needs it, which can matter for static libraries.
- Cross-compiling successfully but then testing with `./app` on the host.

## Debugging Checklist

- Run `file app` and confirm the architecture.
- Add `-Wall -Wextra` and fix the first diagnostic before chasing later ones.
- Add `-v` to inspect compiler search paths.
- Use `gcc -E` when debugging preprocessor include or macro behavior.
- Check whether the command uses the intended compiler.
- Check each `-I` path for host-vs-target confusion.
- Check each `-L` path for host-vs-target confusion.
- Use `readelf -d app` to inspect dynamic library dependencies.
- Use `readelf -h app` to inspect architecture and ABI details.
- If a target binary will not run on the board, compare `file app` with known-good target binaries.

## Related Topics

- [Object Files and Linking](object-files-and-linking.md)
- [Native Linux Userspace Builds](native-linux-userspace-builds.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)
- [C Programming](../c/index.md)

## References

- GCC manual
- Clang command guide
- `man gcc`
- `man ld`
