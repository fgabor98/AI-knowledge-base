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

## Related Topics

- [Direct Compiler Invocation](direct-compiler-invocation.md)
- [Make Basics](make-basics.md)
- [Native Linux Userspace Builds](native-linux-userspace-builds.md)
- [C Programming](../c/index.md)

## References

- GCC manual
- GNU Binutils documentation
- `man ld`
- `man ar`
- `man nm`
- `man readelf`
