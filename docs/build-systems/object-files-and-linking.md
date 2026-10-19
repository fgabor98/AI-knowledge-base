---
status: draft
reviewed: false
domain: build-systems
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Object Files and Linking

## What Problem Does This Solve?

Real programs are usually split across multiple source files. Object files and linking explain how separately compiled files become one executable or library.

This is essential before learning Make, kernel builds, CMake targets, static libraries, shared libraries, and embedded root filesystem dependencies.

## Core Concepts

- translation unit
- object file
- symbol
- declaration
- definition
- linker
- executable
- static library
- shared library
- undefined reference

## Mental Model

A `.c` file is compiled into a `.o` object file:

```text
main.c -> main.o
util.c -> util.o
```

Each object file contains machine code plus symbols. Some symbols are defined inside the object. Others are references that must be resolved later.

The linker combines object files and libraries:

```text
main.o + util.o + libraries -> app
```

If the linker cannot find a needed definition, the build fails with an "undefined reference" error.

Think of compilation as local and linking as global:

- Compilation checks one translation unit at a time.
- Linking checks whether all required definitions exist together.
- Runtime loading checks whether needed shared libraries exist on the machine that runs the program.

These are different failure stages. A project can compile cleanly, link cleanly, and still fail on the target because a shared library is missing from the root filesystem.

## Syntax / API / Mechanism

Compile without linking:

```sh
gcc -c main.c -o main.o
gcc -c util.c -o util.o
```

Link object files:

```sh
gcc main.o util.o -o app
```

Inspect symbols:

```sh
nm main.o
nm util.o
```

Common `nm` symbol letters:

- `T` means a text/code symbol is defined in the file.
- `U` means the file refers to an undefined symbol.
- `D` or `B` means a global data symbol is defined.
- lowercase letters usually indicate local symbols.

Inspect the final binary:

```sh
file app
readelf -h app
readelf -d app
```

Create a static library:

```sh
ar rcs libutil.a util.o
gcc main.o -L. -lutil -o app
```

Create a shared library:

```sh
gcc -fPIC -c util.c -o util.o
gcc -shared util.o -o libutil.so
gcc main.o -L. -lutil -o app
```

When running a program linked against a shared library in the current directory, the runtime loader may not find it automatically. For quick local testing:

```sh
LD_LIBRARY_PATH=. ./app
```

For production embedded systems, prefer installing libraries into known target library paths or configuring the image properly. Do not rely on ad hoc `LD_LIBRARY_PATH` in service units unless there is a deliberate reason.

## Minimal Example

`util.h`:

```c
#ifndef UTIL_H
#define UTIL_H

int add(int a, int b);

#endif
```

`util.c`:

```c
#include "util.h"

int add(int a, int b)
{
    return a + b;
}
```

`main.c`:

```c
#include <stdio.h>
#include "util.h"

int main(void)
{
    printf("%d\n", add(2, 3));
    return 0;
}
```

Build:

```sh
gcc -Wall -Wextra -c main.c -o main.o
gcc -Wall -Wextra -c util.c -o util.o
gcc main.o util.o -o app
```

Run:

```sh
./app
```

Inspect symbols:

```sh
nm main.o
nm util.o
```

You should see that `main.o` refers to `add`, and `util.o` defines it.

## What Is Inside An Executable

The linker does more than resolve names. It also merges the *sections* of every input object (`.text`, `.rodata`, `.data`, `.bss`, ...) and groups them into *segments*, the pieces the loader maps into memory with one set of permissions. The transcripts below come from `bash examples/build-systems/tool-sessions/compiler-driver/run.sh`:

```text
$ gcc -O2 -Iinclude main.c -o app && size app
   text	   data	    bss	    dec	    hex	filename
   1417	    600	      8	   2025	    7e9	app
$ readelf -lW app | grep -E '^ +(Type|INTERP|LOAD|DYNAMIC|GNU_RELRO)'
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  INTERP         0x000318 0x0000000000000318 0x0000000000000318 0x00001c 0x00001c R   0x1
  LOAD           0x000000 0x0000000000000000 0x0000000000000000 0x000648 0x000648 R   0x1000
  LOAD           0x001000 0x0000000000001000 0x0000000000001000 0x000189 0x000189 R E 0x1000
  LOAD           0x002000 0x0000000000002000 0x0000000000002000 0x0000ec 0x0000ec R   0x1000
  LOAD           0x002db8 0x0000000000003db8 0x0000000000003db8 0x000258 0x000260 RW  0x1000
  DYNAMIC        0x002dc8 0x0000000000003dc8 0x0000000000003dc8 0x0001f0 0x0001f0 RW  0x8
  GNU_RELRO      0x002db8 0x0000000000003db8 0x0000000000003db8 0x000248 0x000248 R   0x1
$ readelf -lW app | sed -n '/Section to Segment mapping/,$p'
 Section to Segment mapping:
  Segment Sections...
   00
   01     .interp
   02     .interp .note.gnu.property .note.gnu.build-id .note.ABI-tag .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt
   03     .init .plt .plt.got .plt.sec .text .fini
   04     .rodata .eh_frame_hdr .eh_frame
   05     .init_array .fini_array .dynamic .got .data .bss
...
```

How to read it:

- The four `LOAD` segments are the program as the loader sees it: read-only metadata (02), executable code (03, flags `R E`), read-only constants (04), and writable data (05, `RW`).
- In the last `LOAD` segment `MemSiz` (0x260) is larger than `FileSiz` (0x258): the difference is `.bss`, which takes memory but no bytes in the file.
- `INTERP` names the dynamic loader. `DYNAMIC` holds the `NEEDED` library list and search paths that `readelf -d` prints.
- `GNU_RELRO` is made read-only after relocation, a hardening feature.
- Addresses start at 0 because this is a position-independent executable; the loader picks the real base address.

`size` sums the same data per kind: `text` includes code and read-only data, `data` is initialized writable data, `bss` is zero-initialized data. Use `size` to track growth in CI, and `readelf -lW` when the question is about permissions, the loader, or which section ended up where.

Firmware images use the same mechanism with a linker script that places segments at fixed flash and RAM addresses. See [Linker Scripts and Memory Layout](../c/compilation-linking-and-abi/linker-scripts-and-memory-layout.md).

## Real-World Example

An embedded Linux application may have separate modules for hardware access, configuration parsing, logging, and the main command-line interface:

```text
src/main.c
src/gpio.c
src/config.c
src/log.c
include/gpio.h
include/config.h
include/log.h
```

The manual build would look like:

```sh
gcc -Iinclude -Wall -Wextra -O2 -c src/main.c -o main.o
gcc -Iinclude -Wall -Wextra -O2 -c src/gpio.c -o gpio.o
gcc -Iinclude -Wall -Wextra -O2 -c src/config.c -o config.o
gcc -Iinclude -Wall -Wextra -O2 -c src/log.c -o log.o
gcc main.o gpio.o config.o log.o -lgpiod -o board-tool
```

This is the point where a Makefile becomes useful: it records the dependency graph and rebuilds only what changed.

## Common Scenarios

### Undefined Reference

Symptom:

```text
undefined reference to `add'
```

Likely causes:

- `util.o` was not included in the link command.
- The function name in the header does not match the function definition.
- The definition is excluded by `#ifdef`.
- A C++ function is being linked from C without `extern "C"`.
- The needed library appears too early in a static link command.

Start by checking the final link command and using `nm`:

```sh
nm util.o | grep ' add'
```

### Multiple Definition

Symptom:

```text
multiple definition of `counter'
```

Likely causes:

- A non-`static` variable definition was placed in a header.
- A function definition was placed in a header without being `static inline`.
- The same object file is linked twice.

Headers should usually contain declarations, not ordinary global definitions.

### Static Library Order Problem

With static libraries, order can matter:

```sh
gcc main.o -L. -lutil -o app
```

is usually correct if `main.o` needs symbols from `libutil.a`.

This may fail:

```sh
gcc -L. -lutil main.o -o app
```

because the linker may scan the static library before seeing the unresolved references from `main.o`.

### Shared Library Present At Build Time But Missing At Runtime

The link succeeds on the build machine:

```sh
gcc main.o -lgpiod -o gpio-tool
```

but the target fails:

```text
error while loading shared libraries: libgpiod.so.2: cannot open shared object file
```

The executable was linked against a shared library that is not present in the target root filesystem. Check:

```sh
readelf -d gpio-tool
```

Then make sure the root filesystem includes the required runtime package.

### Header Change Does Not Rebuild Object

If `util.h` changes but `main.o` is not rebuilt, the Makefile probably does not list the header dependency or does not generate dependency files. This creates stale object files, which can produce confusing behavior.

### Host Library Accidentally Linked Into Target Program

During cross-compilation, a link command can accidentally search `/usr/lib` from the host. The build may fail with an architecture mismatch, or worse, it may pick a wrong dependency in subtle ways. Use target sysroots and inspect link paths.

## Common Mistakes

- Compiling `main.c` but forgetting to link `util.o`.
- Putting function definitions in headers without understanding duplicate symbols.
- Declaring a function in a header but never defining it in a `.c` file.
- Linking libraries in the wrong order when using static libraries.
- Confusing compile errors with link errors.
- Building a shared library without `-fPIC`.
- Copying an executable to the target without the shared libraries it needs.
- Running `ldd` on a target binary and trusting host output.
- Forgetting that headers are compile-time inputs but shared libraries are runtime dependencies.
- Assuming a successful link means the target root filesystem has everything needed.

## Debugging Checklist

- If the error says "undefined reference", inspect the link command.
- Confirm the object file or library that defines the symbol is present.
- Use `nm file.o` or `nm libname.a` to find symbols.
- Check whether a missing dependency is a static or shared library.
- Check library order when linking static libraries.
- Use `readelf -d app` to inspect runtime shared library needs.
- Use `ldd app` only for host binaries, not target binaries that cannot run on the host.
- Use `file app libname.so` to confirm architectures match.
- Use `readelf -Ws file.o` or `readelf -Ws libname.so` when `nm` output is not enough.
- For runtime failures, check the target root filesystem, not only the build host.

## Go Deeper

The C section covers this material at intermediate and advanced depth, with real tool transcripts:

- [Object Files, Symbols, and Relocations](../c/compilation-linking-and-abi/object-files-symbols-and-relocations.md): sections, symbol binding, weak symbols, relocations
- [Static and Dynamic Linking](../c/compilation-linking-and-abi/static-and-dynamic-linking.md): archive member extraction, map files, `--gc-sections`, SONAME, `$ORIGIN`, interposition, `-fvisibility=hidden`
- [Linker Scripts and Memory Layout](../c/compilation-linking-and-abi/linker-scripts-and-memory-layout.md): LMA/VMA, memory regions, orphan sections
- [Debug Information and Binary Inspection](../c/compilation-linking-and-abi/debug-information-and-binary-inspection.md): from a crash address to a source line
- [Shared Libraries, ABI, and Runtime Linking](shared-libraries-abi-and-runtime-linking.md): library names, symbol versions, loader search order

## Related Topics

- [Direct Compiler Invocation](direct-compiler-invocation.md)
- [Make Basics](make-basics.md)
- [Native Linux Userspace Builds](native-linux-userspace-builds.md)
- [C Programming](../c/index.md)

## References

- [GNU ld manual](https://sourceware.org/binutils/docs/ld/)
- [GNU Binutils: `ar`, `nm`, `readelf`, `objdump`, `size`](https://sourceware.org/binutils/docs/binutils/)
- [GCC: Options for linking](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)
- [System V ABI and ELF specification](https://refspecs.linuxfoundation.org/elf/)
