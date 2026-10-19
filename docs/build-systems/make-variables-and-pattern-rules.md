---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Make Variables and Pattern Rules

## What Problem Does This Solve?

Basic Makefiles become repetitive quickly. Variables and pattern rules let you express the build once and reuse it across many files.

For embedded Linux work, this is also where Makefiles become configurable: the compiler, flags, install paths, and cross-compiler prefix are supplied by someone else — a developer's shell, a CI job, Buildroot, a Yocto recipe, or a vendor SDK environment script. A Makefile that ignores or silently replaces those values builds for the wrong machine, with the wrong sysroot, or without the flags the distribution requires.

## Core Concepts

- recursive (`=`) and simple (`:=`) expansion
- conditional (`?=`) and appending (`+=`) assignment
- variable origin: default, environment, file, command line, override
- built-in variables (`CC`, `CXX`, `AR`) and built-in rules
- pattern rules and automatic variables
- `CPPFLAGS`, `CFLAGS`, `LDFLAGS`, `LDLIBS` roles
- cross-compiler prefix
- generated header dependencies

## Mental Model

Instead of writing one compile rule for every `.c` file, write the pattern:

```text
Any .o file can be built from the matching .c file.
```

In Make:

```make
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
```

Automatic variables make the rule generic:

- `$@` is the target.
- `$<` is the first prerequisite.
- `$^` is all prerequisites, without duplicates.
- `$(@D)` is the directory part of the target.
- `$*` is the stem matched by `%`.

Variables make the Makefile configurable. But a variable value can come from **five places**, and they do not all have the same strength. That ranking is the part most Makefiles get wrong:

```text
weakest   default        built into make (CC = cc, CXX = g++, AR = ar)
          environment    exported by the shell, CI job, SDK setup script, or Yocto
          file           assigned in the Makefile (=, :=, +=, ?=)
          command line   make CC=clang CFLAGS=-O0
strongest override       "override VAR = ..." in the Makefile
```

A Makefile assignment beats the environment. A command-line assignment beats the Makefile. `make -e` lifts the environment above the Makefile. `$(origin VAR)` tells you which source won.

## Syntax / API / Mechanism

Assignment operators:

| Operator | Meaning | Use it for |
| --- | --- | --- |
| `=` | recursive: expanded every time the variable is used | values that refer to variables defined later |
| `:=` | simple: expanded once, when the line is read | internal derived values, `$(shell ...)` results |
| `?=` | assign only if the variable is **undefined** | defaults for variables make does not predefine |
| `+=` | append, keeping the existing flavor | adding flags to a value someone else set |
| `override VAR ...` | assign even if set on the command line | project-required flags (use sparingly) |

Conventional variable roles, honoured by make's built-in rules and by most packaging systems:

- `CPPFLAGS` are preprocessor flags, such as `-Iinclude` and `-DNAME=value`.
- `CFLAGS` are C compiler flags, such as `-Wall -O2 -g`.
- `LDFLAGS` are linker options, such as `-Lpath` or `-Wl,--as-needed`.
- `LDLIBS` are libraries, such as `-lgpiod -lm`. They go **after** the objects.

Keep them separate. A distribution build passes hardening and sysroot options in `CFLAGS`/`LDFLAGS`; if your link rule drops `LDFLAGS`, those options silently disappear (Yocto reports this as the `ldflags` QA error).

## The `CC ?= gcc` Trap

Many Makefiles, tutorials, and vendor examples start with `CC ?= gcc`. It does nothing. `CC` is one of make's built-in variables, so it is always defined, and `?=` only assigns undefined variables.

The transcripts below come from `bash examples/build-systems/tool-sessions/make-variables/run.sh` (GNU Make 4.3):

```text
$ make --no-print-directory -f conditional.mk
CC=cc  origin=default
$ make --no-print-directory -p -f /dev/null 2>/dev/null | grep -E '^(CC|CXX|AR|CFLAGS|LDFLAGS|LDLIBS) ?='
AR = ar
CXX = g++
CC = cc
```

`make -p` prints make's internal database: `CC`, `CXX`, and `AR` exist before your Makefile is read, while `CFLAGS`, `LDFLAGS`, and `LDLIBS` do not. So `CFLAGS ?= -O2` works and `CC ?= gcc` does not. On most Linux hosts `cc` is a link to `gcc`, which hides the mistake until someone builds on a host where it is not.

### A Hard-Coded Compiler Replaces The Environment

Command-line values win against the Makefile, so `make CC=clang` works with almost any Makefile:

```text
$ make --no-print-directory -f cross.mk CC=clang
CC=clang  origin=command line
```

The environment is different. Yocto recipes, the Yocto SDK `environment-setup-*` script, and many CI jobs **export** `CC` — including `--sysroot` and the target tuning flags. An ordinary `=` or `:=` in the Makefile replaces it without warning:

```make
--8<-- "examples/build-systems/tool-sessions/make-variables/cross.mk"
```

```text
$ CC='aarch64-poky-linux-gcc --sysroot=/sdk/sysroot' make --no-print-directory -f cross.mk
CC=gcc  origin=file
$ CC='aarch64-poky-linux-gcc --sysroot=/sdk/sysroot' make --no-print-directory -e -f cross.mk
CC=aarch64-poky-linux-gcc --sysroot=/sdk/sysroot  origin=environment override
```

The first build compiles target code with the host `gcc`. It may even succeed and produce an x86-64 binary that only fails on the board. `make -e` is the caller-side workaround; the Makefile-side fix is below.

### The Fix: Choose A Default Only When Nobody Else Did

```make
--8<-- "examples/build-systems/tool-sessions/make-variables/respect.mk"
```

```text
$ make --no-print-directory -f respect.mk
CC=gcc  origin=file
$ make --no-print-directory -f respect.mk CROSS_COMPILE=aarch64-linux-gnu-
CC=aarch64-linux-gnu-gcc  origin=file
$ CC='aarch64-poky-linux-gcc --sysroot=/sdk/sysroot' make --no-print-directory -f respect.mk
CC=aarch64-poky-linux-gcc --sysroot=/sdk/sysroot  origin=environment
$ make --no-print-directory -f respect.mk CC=clang
CC=clang  origin=command line
```

`$(origin CC)` is `default` only when the value still comes from make's built-in database. Every real caller — environment, command line, an included file — is respected, and `CROSS_COMPILE` still works for developers. Apply the same guard to `CXX`, `AR`, and other built-in tool variables. For tools make does not predefine (`OBJCOPY`, `STRIP`, `PKG_CONFIG`), `?=` is correct.

The simplest correct alternative is to not assign `CC` at all and let callers pass it.

## Flags: Append, But Know Who Can Undo It

The usual advice is "append project flags with `+=`":

```make
--8<-- "examples/build-systems/tool-sessions/make-variables/flags.mk"
```

That works for the environment, and does **not** work for the command line, because a command-line value ignores every ordinary assignment in the Makefile — including `+=`:

```text
$ make --no-print-directory -f flags.mk
CFLAGS=-O2 -Wall -Wextra  origin=file
$ CFLAGS='-O0 -g' make --no-print-directory -f flags.mk
CFLAGS=-O0 -g -Wall -Wextra  origin=file
$ make --no-print-directory -f flags.mk CFLAGS='-O0 -g'
CFLAGS=-O0 -g  origin=command line
$ make --no-print-directory -f flags-override.mk CFLAGS='-O0 -g'
CFLAGS=-O0 -g -Wall -Wextra  origin=override
```

The third run silently lost `-Wall -Wextra`. Buildroot passes `CC`, `CFLAGS`, and `LDFLAGS` on the command line (`$(TARGET_CONFIGURE_OPTS)`), so this is not a corner case.

Two robust patterns:

1. `override CFLAGS += -Wall -Wextra` — always appends, as the last run shows. Use it for the few flags the project cannot build without.
2. Keep project flags in a private variable and put both in the recipe, as Automake does with `AM_CFLAGS`:

```make
KB_CFLAGS := -std=c17 -Wall -Wextra
CFLAGS    ?= -O2 -g

%.o: %.c
	$(CC) $(KB_CPPFLAGS) $(CPPFLAGS) $(KB_CFLAGS) $(CFLAGS) -c $< -o $@
```

The caller's `CFLAGS` comes last, so a caller can still override an optimization level, but cannot accidentally remove required options. The [Intermediate Make](make-intermediate.md) project uses this pattern.

## Minimal Example

```make
APP  := app
OBJS := main.o util.o

CFLAGS ?= -Wall -Wextra -O2

$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(APP) $(OBJS)
```

`CC` is not assigned: make's default `cc` is used unless a caller provides one.

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

`-MMD` writes a `.d` file listing the non-system headers each object included. `-MP` adds an empty rule for each header so that deleting a header later does not break the build. The leading `-` on `include` ignores missing `.d` files on the first build.

## Real-World Example

An override-friendly Makefile for a small Linux userspace tool:

```make
APP  := board-tool
OBJS := main.o gpio.o config.o

CROSS_COMPILE ?=
ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

CPPFLAGS ?=
CFLAGS   ?= -O2
override CPPFLAGS += -Iinclude
override CFLAGS   += -Wall -Wextra
LDLIBS   += -lgpiod
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

The same file now builds correctly when called:

| Caller | How it passes the toolchain | Result |
| --- | --- | --- |
| developer | nothing | host `gcc` |
| developer, cross | `make CROSS_COMPILE=aarch64-linux-gnu-` | prefixed cross compiler |
| Yocto SDK shell | exported `CC`, `CFLAGS`, `LDFLAGS` | SDK compiler with `--sysroot` |
| Yocto recipe (`oe_runmake`) | exported environment, plus `EXTRA_OEMAKE` | recipe compiler and flags |
| Buildroot | `CC=... CFLAGS=... LDFLAGS=...` on the command line | Buildroot toolchain, project warnings kept by `override` |

Install through a staging directory:

```make
PREFIX ?= /usr
DESTDIR ?=

.PHONY: install
install: $(APP)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(APP) $(DESTDIR)$(PREFIX)/bin/$(APP)
```

Packaging systems install into a staging directory (`DESTDIR`), never into the build host's `/usr`. See [Install Rules and Staging](install-rules-and-staging.md).

## Common Scenarios

### Build Source Files From A `src/` Directory

Pattern rules must match the real layout:

```make
OBJS := main.o gpio.o

%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
```

This builds `main.o` from `src/main.c`.

### Build Into A Separate Output Directory

```make
OBJS := build/main.o build/gpio.o

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build:
	mkdir -p $@
```

The `|` makes `build` an order-only prerequisite: it must exist, but its timestamp never forces a rebuild. [Intermediate Make](make-intermediate.md) shows why that matters.

### Use `pkg-config` From Make

```make
PKG_CONFIG ?= pkg-config
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags libgpiod)
LDLIBS   += $(shell $(PKG_CONFIG) --libs libgpiod)
```

`PKG_CONFIG` is not a built-in variable, so `?=` is right here. For cross-builds, the caller must point `pkg-config` at the target sysroot; see [Target pkg-config](target-pkg-config.md).

### Find Out Where A Value Came From

Add a generic helper target:

```make
print-%:
	@echo '$*=$($*)  [origin: $(origin $*)]'
```

```sh
make print-CC print-CFLAGS
```

`$(info ...)` and `$(warning ...)` print while the Makefile is parsed. `make -p` dumps every variable and rule, including built-in ones.

## Common Mistakes

- Writing `CC ?= gcc` and believing it sets a default; it never does.
- Hard-coding `CC = gcc` or `CC := $(CROSS_COMPILE)gcc`, which discards an exported SDK or Yocto compiler.
- Using `=` and `:=` without understanding when expansion happens; `VAR = $(shell ...)` runs the command on every use.
- Expecting `CFLAGS += ...` to survive `make CFLAGS=...`.
- Overusing `override`, so callers can no longer change anything.
- Putting libraries in `LDFLAGS` instead of `LDLIBS`, which puts `-lfoo` before the objects.
- Dropping `$(LDFLAGS)` from the link rule, losing sysroot and hardening flags.
- Forgetting that `$<` means only the first prerequisite.
- Writing a pattern rule that does not match the project layout.
- Using `$(shell pkg-config ...)` with the host `pkg-config` during a cross-build.
- Forgetting to clean generated `.d` dependency files.

## Debugging Checklist

- Run `make -n` to see the fully expanded commands without running them.
- Run `make print-CC` (with the helper above) or add `$(info CC=$(CC) [$(origin CC)])`.
- Run `make -p -f /dev/null` to list built-in variables and rules.
- If the origin is `file` but you exported a value, the Makefile replaced it; try `make -e` to confirm.
- If the origin is `command line` and project flags are missing, the Makefile needs `override` or private flag variables.
- Check whether `CROSS_COMPILE` includes the trailing hyphen.
- Check `file` on the output: an x86-64 result from a "cross" build means the host compiler was used.
- Check generated `.d` files when header rebuild behavior is wrong.

## Exercises

1. Run `examples/build-systems/tool-sessions/make-variables/run.sh` and predict each `origin` before reading it.
2. Take a vendor Makefile that writes `CC = gcc`, source a Yocto SDK environment script, and show which compiler is used with and without `make -e`.
3. Convert the real-world example to private `KB_CFLAGS` variables instead of `override` and confirm both callers in the table still work.

## Related Topics

- [Make Basics](make-basics.md)
- [Intermediate Make](make-intermediate.md)
- [Native Linux Userspace Builds](native-linux-userspace-builds.md)
- [Cross-Compilation](cross-compilation.md)
- [Linux Kernel Build System](advanced/linux-kernel/index.md)

## References

- [GNU Make: How variables get their values](https://www.gnu.org/software/make/manual/html_node/Using-Variables.html)
- [GNU Make: The `override` directive](https://www.gnu.org/software/make/manual/html_node/Override-Directive.html)
- [GNU Make: The `origin` function](https://www.gnu.org/software/make/manual/html_node/Origin-Function.html)
- [GNU Make: Variables used by implicit rules](https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html)
- [GNU Make: Pattern rules and automatic variables](https://www.gnu.org/software/make/manual/html_node/Pattern-Rules.html)
- [GNU Coding Standards: Variables for commands](https://www.gnu.org/prep/standards/html_node/Command-Variables.html)
