---
status: draft
reviewed: false
domain: build-systems
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Native Linux Userspace Builds

## What Problem Does This Solve?

A native Linux userspace build produces a program for the same machine that runs the build. This is the simplest realistic environment for learning build systems before adding cross-compilation, sysroots, target package managers, kernel modules, or full image builds.

For embedded Linux developers, native builds are useful for learning, host-side tests, command-line tools, generators, and prototypes.

## Core Concepts

- native build
- host machine
- userspace program
- compiler flags
- linker flags
- installed files
- runtime dependencies
- debug build
- release build
- host-side test

## Mental Model

Native build:

```text
build machine == run machine
```

Cross build:

```text
build machine != run machine
```

Start native because it removes target-specific variables. If a project cannot build cleanly natively, cross-compilation will usually be harder to debug.

Native builds are also useful even in target-focused projects:

- host-side unit tests
- command-line helper tools
- code generators
- configuration validators
- protocol parsers
- fast static analysis builds
- reproducing compiler and linker fundamentals before involving a BSP

Do not confuse "native" with "production." Native builds are often learning, test, or tooling builds. The final product build may still come from Yocto, Buildroot, TI Processor SDK, or another system framework.

## Syntax / API / Mechanism

Typical native build commands:

```sh
gcc -Wall -Wextra -g main.c -o app
./app
```

With multiple files:

```sh
gcc -Wall -Wextra -O2 -c main.c -o main.o
gcc -Wall -Wextra -O2 -c util.c -o util.o
gcc main.o util.o -o app
```

With Make:

```sh
make
make clean
```

With debug flags:

```sh
make CFLAGS="-Wall -Wextra -O0 -g"
```

With release-like flags:

```sh
make CFLAGS="-Wall -Wextra -O2"
```

Install to a staging directory:

```sh
make DESTDIR="$PWD/stage" install
```

Common inspection commands:

```sh
file app
ldd app
readelf -d app
nm app
```

Common test commands:

```sh
./app --help
./app < test-input.txt
```

## Minimal Example

Makefile:

```make
APP := hello
OBJS := hello.o

PREFIX ?= /usr/local
DESTDIR ?=

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?=
LDLIBS ?=

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: install clean
install: $(APP)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(APP) $(DESTDIR)$(PREFIX)/bin/$(APP)

clean:
	rm -f $(APP) $(OBJS)
```

Build:

```sh
make
```

Install into a staging directory instead of the real system:

```sh
make DESTDIR="$PWD/stage" install
```

Inspect:

```sh
find stage -type f -print
```

## Real-World Example

Suppose an embedded project has a host-side configuration generator:

```text
tools/gen-config.c
```

This tool runs on the development machine during the build, even if the final firmware or Linux image targets ARM:

```sh
gcc -Wall -Wextra -O2 tools/gen-config.c -o gen-config
./gen-config board.yaml generated-config.h
```

This distinction matters. In cross-builds, some binaries are target artifacts and some binaries are host tools. Build systems must keep those two categories separate.

## Common Scenarios

### Build A Host-Side Test Binary

For logic that does not require target hardware, build and run tests natively:

```sh
gcc -Wall -Wextra -O0 -g test_parser.c parser.c -o test-parser
./test-parser
```

This is faster than deploying to a board for every parser or state-machine bug.

### Build A Host Tool Used By A Target Build

Some projects generate headers or binary blobs:

```sh
gcc -Wall -Wextra -O2 tools/mkimage-table.c -o mkimage-table
./mkimage-table board.yaml generated/table.h
```

If the final target is ARM, this tool still must be built for the host because it runs during the build.

### Install Into A Staging Directory

Never test install rules first with `sudo make install`. Use:

```sh
make DESTDIR="$PWD/stage" PREFIX=/usr install
find stage -type f -print
```

Expected layout:

```text
stage/usr/bin/hello
```

Packaging systems use the same idea: install into a temporary directory, then package that directory.

### Check Runtime Libraries

For a native binary:

```sh
ldd ./app
```

For a target binary, avoid trusting host `ldd`. Use:

```sh
readelf -d ./app
```

Native `ldd` executes or traces through the host dynamic loader behavior. That is not the target runtime environment.

### Separate Debug And Release Outputs

For small projects:

```sh
make clean
make CFLAGS="-Wall -Wextra -O0 -g"
```

Then:

```sh
make clean
make CFLAGS="-Wall -Wextra -O2 -g"
```

Keeping `-g` in release-like builds is useful because debug symbols can be split and archived later.

### Prototype Before Cross-Compiling

If the code uses ordinary files, parsing, logging, or protocol handling, prove it natively first. Keep board-specific access behind small modules so most logic can be tested on the host.

Example boundary:

```text
portable logic: parser.c, state_machine.c, config.c
target-specific logic: gpio_linux.c, i2c_linux.c
```

### Avoid Host-Specific Assumptions

Native builds can hide target problems:

- `sizeof(long)` may differ across architectures.
- endianness may differ.
- filesystem paths may differ.
- available shared libraries may differ.
- timing and hardware behavior will differ.
- compiler defaults may differ.

Use native builds for fast feedback, not as a substitute for target validation.

## Common Mistakes

- Assuming native success proves the program will run on the target.
- Using `sudo make install` while learning instead of staging with `DESTDIR`.
- Confusing host tools with target programs.
- Testing only release builds and losing debug visibility.
- Testing only debug builds and missing optimization-related warnings.
- Ignoring runtime shared library dependencies.
- Hard-coding `/usr/local` paths into software intended for an embedded root filesystem.
- Assuming `ldd` results on the host describe the target runtime.
- Installing into the host system while experimenting.
- Building generated tools with the target compiler even though they must run on the host.
- Skipping native tests because the final product runs on hardware.
- Letting native-only assumptions leak into target code.

## Debugging Checklist

- Use `file app` to confirm it is a host binary.
- Run the program locally before adding a build system layer.
- Use `ldd app` to inspect host runtime dependencies.
- Use `make DESTDIR="$PWD/stage" install` to verify install layout.
- Check whether the binary is a host tool or target artifact.
- Use `gdb ./app` for host-side debugging.
- Keep native test builds separate from target release builds.
- Use `readelf -d app` when you need target-independent dynamic dependency inspection.
- Verify install layout with `find stage -type f`.
- Check whether generated files are rebuilt when generator inputs change.
- Confirm the build can run from a clean checkout without manual host-system installation.

## Related Topics

- [Direct Compiler Invocation](direct-compiler-invocation.md)
- [Object Files and Linking](object-files-and-linking.md)
- [Make Variables and Pattern Rules](make-variables-and-pattern-rules.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)

## References

- GCC manual
- GNU Make manual
- Filesystem Hierarchy Standard
- `man install`
