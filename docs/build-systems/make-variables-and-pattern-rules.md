---
status: draft
reviewed: false
domain: build-systems
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Make Variables and Pattern Rules

## What Problem Does This Solve?

Basic Makefiles become repetitive quickly. Variables and pattern rules let you express the build once and reuse it across many files.

For embedded Linux work, this is also where Makefiles become configurable: the compiler, flags, install paths, and cross-compiler prefix can be overridden by the environment, a CI job, Buildroot, Yocto, or a vendor SDK.

## Core Concepts

- variables
- default assignment
- command-line override
- pattern rule
- automatic variables
- object list
- phony targets
- cross-compiler prefix

## Mental Model

Instead of writing one compile rule for every `.c` file, write the pattern:

```text
Any .o file can be built from the matching .c file.
```

In Make:

```make
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
```

Automatic variables make the rule generic:

- `$@` is the target.
- `$<` is the first prerequisite.
- `$^` is all prerequisites.

Variables make the Makefile configurable. Pattern rules make it scalable. Automatic variables make pattern rules readable.

For embedded work, this is not just convenience. It is how the same project can be built by:

- a developer using the host compiler
- a CI job using stricter flags
- Buildroot passing its own `CC`, `CFLAGS`, and `LDFLAGS`
- Yocto running the build inside a recipe environment
- a vendor SDK using a cross-compiler prefix

## Syntax / API / Mechanism

Common variable assignments:

```make
CC = gcc
CFLAGS = -Wall -Wextra -O2
```

Immediate expansion:

```make
CC := gcc
```

Default assignment, useful for override-friendly Makefiles:

```make
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
```

The practical distinction:

- `=` expands when the variable is used.
- `:=` expands immediately when the line is read.
- `?=` sets a default only if the variable is not already defined.
- `+=` appends to an existing variable.

Beginner rule of thumb:

- use `?=` for variables users or packaging systems should override
- use `:=` for internal variables derived from other values
- use `+=` to add local flags without discarding existing ones

Append:

```make
CFLAGS += -Iinclude
LDLIBS += -lgpiod
```

Pattern rule:

```make
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
```

Object list:

```make
OBJS := main.o util.o
```

Link rule:

```make
app: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
```

Conventional variable roles:

- `CPPFLAGS` are preprocessor flags, such as `-Iinclude` and `-DNAME=value`.
- `CFLAGS` are C compiler flags, such as `-Wall -O2 -g`.
- `LDFLAGS` are linker options, such as `-Lpath` or `-Wl,...`.
- `LDLIBS` are libraries, such as `-lgpiod -lm`.

Keeping these separate makes the Makefile easier for external build systems to override.

Cross-compiler prefix:

```make
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
```

Build for a target:

```sh
make CROSS_COMPILE=arm-linux-gnueabihf-
```

Generate header dependency files:

```make
DEPFLAGS := -MMD -MP

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)
```

This tells the compiler to write `.d` files describing included headers, then asks Make to include them if present.

## Minimal Example

```make
APP := app
OBJS := main.o util.o

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?=
LDLIBS ?=

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(APP) $(OBJS)
```

Run:

```sh
make
make clean
make CC=clang
make CFLAGS="-Wall -Wextra -O0 -g"
```

Add generated header dependencies:

```make
DEPFLAGS := -MMD -MP

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)
```

## Real-World Example

An override-friendly Makefile for a small Linux userspace tool:

```make
APP := board-tool
OBJS := main.o gpio.o config.o

CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

CPPFLAGS ?= -Iinclude
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?=
LDLIBS ?= -lgpiod
DEPFLAGS := -MMD -MP

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(OBJS:.o=.d)

.PHONY: clean
clean:
	rm -f $(APP) $(OBJS) $(OBJS:.o=.d)
```

Native build:

```sh
make
```

Cross-build:

```sh
make CROSS_COMPILE=arm-linux-gnueabihf-
```

Debug build:

```sh
make CFLAGS="-Wall -Wextra -O0 -g"
```

Install through a staging directory:

```make
PREFIX ?= /usr
DESTDIR ?=

.PHONY: install
install: $(APP)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(APP) $(DESTDIR)$(PREFIX)/bin/$(APP)
```

This matters because packaging systems should install into a staging directory, not directly into the developer's host `/usr`.

## Common Scenarios

### Let A Caller Override The Compiler

Good:

```make
CC ?= gcc
```

Build with:

```sh
make CC=clang
```

Less flexible:

```make
CC = gcc
```

This can override values passed by a packaging system or SDK environment.

### Add Project Flags Without Destroying Caller Flags

If a caller provides `CFLAGS`, this line can accidentally replace them:

```make
CFLAGS = -Wall -Wextra -O2
```

Prefer a pattern like:

```make
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra
```

Be deliberate here. Some projects want caller flags to fully replace defaults; others want to append project-required warnings.

### Build Source Files From A `src/` Directory

Pattern rules must match the real layout:

```make
OBJS := main.o gpio.o

%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
```

This builds `main.o` from `src/main.c`.

### Build Into A Separate Output Directory

For larger projects, object files may go under `build/`:

```make
OBJS := build/main.o build/gpio.o

build/%.o: src/%.c
	install -d $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
```

`$(@D)` expands to the directory part of the target.

### Use `pkg-config` From Make

Native example:

```make
CPPFLAGS += $(shell pkg-config --cflags libgpiod)
LDLIBS += $(shell pkg-config --libs libgpiod)
```

For embedded cross-builds, this is only correct if `pkg-config` is configured for the target sysroot.

### Prepare For Buildroot Or Yocto

External build systems often call Make like this:

```sh
make CC=arm-linux-gnueabihf-gcc CFLAGS="..." LDFLAGS="..."
```

Your Makefile should respect those values instead of hiding hard-coded compilers and paths inside recipes.

## Common Mistakes

- Using `=` and `:=` without understanding when expansion happens.
- Hard-coding `gcc` instead of using `$(CC)`.
- Hard-coding flags so CI or packaging systems cannot override them.
- Putting libraries in `LDFLAGS` instead of `LDLIBS`.
- Forgetting that `$<` means only the first prerequisite.
- Writing a pattern rule that does not match the project layout.
- Making `clean` remove source files or generated files that are not owned by the build.
- Using `$(shell pkg-config ...)` without considering cross-compilation.
- Forgetting to clean generated `.d` dependency files.
- Including `.d` files before `OBJS` is defined.
- Placing `-lfoo` before object files in the final link rule.
- Overriding caller-provided flags unintentionally.

## Debugging Checklist

- Run `make --dry-run` to inspect expanded commands.
- Run `make print-VAR` only if the Makefile defines such helper targets.
- Temporarily add `$(info CC=$(CC))` to inspect variable values.
- Check command-line overrides such as `make CC=clang`.
- Check whether `CROSS_COMPILE` includes the trailing hyphen.
- Check whether the pattern rule matches the source path.
- Check whether objects in `OBJS` correspond to real source files.
- Run `make -n` to inspect the final expanded compile and link commands.
- Check generated `.d` files when header rebuild behavior is wrong.
- Check whether `CPPFLAGS`, `CFLAGS`, `LDFLAGS`, and `LDLIBS` are being used in the right recipes.
- Build once with `make clean` and once incrementally to verify both paths.
- Try `make CC=clang` or `make CROSS_COMPILE=...` to test override behavior.

## Related Topics

- [Make Basics](make-basics.md)
- [Native Linux Userspace Builds](native-linux-userspace-builds.md)
- [Linux Kernel Build System Roadmap](linux-kernel-build-roadmap.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)

## References

- GNU Make manual
- `man make`
