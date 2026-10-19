---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Intermediate Make

## What Problem Does This Solve?

A beginner Makefile works on a clean checkout with `make`. An intermediate Makefile must also be **correct incrementally** (rebuild exactly what changed), **correct in parallel** (`make -j$(nproc)`), **safe on failure** (no half-written outputs), and **predictable when called by something else** (Yocto, Buildroot, a vendor SDK, a top-level Makefile). The Linux kernel, U-Boot, BusyBox, and most vendor SDKs depend on these properties, and most "works on my machine" build failures come from breaking one of them.

This page builds one small project and shows each property with real `make` output.

## Core Concepts

- read phase and run phase
- timestamp-based out-of-date checks
- generated header dependencies (`-MMD -MP`)
- order-only prerequisites
- target-specific variables
- stamp files for non-file inputs
- `.DELETE_ON_ERROR`
- one shell per recipe line
- parallel builds and missing edges
- recursive make, `$(MAKE)`, and `MAKEFLAGS`
- functions: `wildcard`, `patsubst`, `filter`, `foreach`, `call`, `eval`
- `--trace`, `-n`, `-p`, `--debug`

## Mental Model

GNU Make works in two phases:

```text
read phase                                  run phase
----------                                  ---------
read Makefile and every included file       start from the goal (first target or command line)
expand := assignments and $(shell ...)      walk prerequisites depth-first
expand target and prerequisite lists        target older than any prerequisite? -> run recipe
build the complete dependency graph         each recipe line runs in its own /bin/sh
```

Two consequences follow:

1. Make only knows the edges you give it. A missing edge does not usually fail a serial build because the serial order happens to be right. It fails in parallel, or after an incremental change, which is much harder to debug.
2. Make compares **file timestamps**, not command lines or variable values. Changing `CFLAGS` or a version number does not rebuild anything unless the Makefile turns that change into a file change.

## The Example Project

All transcripts on this page come from `bash examples/build-systems/tool-sessions/make-project/run.sh` (GNU Make 4.3, GCC 13.3). The project is three C files under `src/` and headers under `include/`:

```make
--8<-- "examples/build-systems/tool-sessions/make-project/Makefile"
```

What each part is for:

| Construct | Why |
| --- | --- |
| `$(wildcard src/*.c)` and `patsubst` | object list follows the source tree; no hand-maintained lists |
| `ifeq ($(origin CC),default)` | respect a compiler exported by Yocto/SDK or passed by Buildroot; see [Make Variables](make-variables-and-pattern-rules.md#the-cc-gcc-trap) |
| `KB_CPPFLAGS`, `KB_CFLAGS` | required project flags survive `make CFLAGS=...` |
| `-MMD -MP` and `-include $(DEPS)` | header changes rebuild the right objects |
| `\| $(BUILD)` | output directory must exist, but its timestamp is irrelevant |
| `.DELETE_ON_ERROR:` | a failed recipe never leaves a plausible-looking target |
| target-specific `KB_CPPFLAGS +=` | only `version.o` gets the version define |
| `version.stamp` | a changed `VERSION` becomes a changed file |
| `print-%` | ask make what a variable is and where it came from |

## Incremental Correctness

### First Build And A No-Op Build

```text
$ make --no-print-directory
gcc -Iinclude  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/gpio.c -o build/gpio.o
gcc -Iinclude  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/main.c -o build/main.o
gcc -Iinclude -DTOOL_VERSION='"1.0"'  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/version.c -o build/version.o
gcc  build/gpio.o build/main.o build/version.o  -o build/board-tool
$ ./build/board-tool
board-tool 1.0: line 3 = 1
$ make --no-print-directory
make: Nothing to be done for 'all'.
```

A second `make` that does nothing is the first test of any Makefile. If it rebuilds something, there is a target that is never created (a misspelled output, a phony target used as a prerequisite) or a prerequisite that is always newer.

### Generated Header Dependencies

The compiler writes the header list while compiling:

```text
$ cat build/main.d
build/main.o: src/main.c include/gpio.h include/legacy.h
include/gpio.h:
include/legacy.h:
```

The first line is the real dependency rule. The empty rules come from `-MP`. Touching a header now rebuilds exactly its users, and `--trace` explains every decision:

```text
$ touch include/gpio.h && make --no-print-directory --trace
Makefile:35: update target 'build/gpio.o' due to: include/gpio.h
gcc -Iinclude  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/gpio.c -o build/gpio.o
Makefile:35: update target 'build/main.o' due to: include/gpio.h
gcc -Iinclude  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/main.c -o build/main.o
Makefile:32: update target 'build/board-tool' due to: build/gpio.o build/main.o
gcc  build/gpio.o build/main.o build/version.o  -o build/board-tool
```

`version.o` does not include `gpio.h` and was not rebuilt.

Why `-MP` matters: delete `legacy.h` and remove its `#include`. The old `build/main.d` still names `include/legacy.h`. Without `-MP`, make stops with `No rule to make target 'include/legacy.h', needed by 'build/main.o'`, and developers "fix" it with `make clean`. With `-MP`, the empty rule makes the missing header harmless, and the next compile writes a new `.d` file:

```text
$ rm include/legacy.h && sed -i -e '/legacy.h/d' -e 's/LEGACY_BANNER/"board-tool"/' src/main.c && make --no-print-directory
gcc -Iinclude  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/main.c -o build/main.o
gcc  build/gpio.o build/main.o build/version.o  -o build/board-tool
```

Use `-MMD` (user headers only), not `-MD` (also system headers), unless you need to rebuild when the toolchain or sysroot headers change. In that case a full rebuild after a toolchain change is usually simpler and safer.

### Order-Only Prerequisites

Objects need `build/` to exist. If `build` were an ordinary prerequisite, its timestamp would matter, and a directory's timestamp changes every time a file is added to it:

```text
$ touch build/notes.txt && make --no-print-directory --trace
make: Nothing to be done for 'all'.
$ sed 's/ | \$(BUILD)$/ \$(BUILD)/' Makefile > no-order-only.mk && touch build/notes.txt && make --no-print-directory -n -f no-order-only.mk --trace | grep 'due to'
no-order-only.mk:35: update target 'build/gpio.o' due to: build
no-order-only.mk:35: update target 'build/main.o' due to: build
no-order-only.mk:35: update target 'build/version.o' due to: build
no-order-only.mk:32: update target 'build/board-tool' due to: build/gpio.o build/main.o build/version.o
```

Everything after `|` in a prerequisite list is order-only: it must exist, and it is built first, but it never makes the target out of date. Use it for output directories and for tools that must exist but whose rebuild should not cascade.

### Changing A Variable Is Not Changing A File

Running `make VERSION=2.1` on an up-to-date tree normally does nothing, because no file changed. The project's stamp file fixes that:

```make
$(shell mkdir -p $(BUILD) && echo '$(VERSION)' | cmp -s - $(BUILD)/version.stamp || echo '$(VERSION)' > $(BUILD)/version.stamp)
$(BUILD)/version.o: $(BUILD)/version.stamp
```

The `$(shell ...)` runs during the read phase and rewrites the stamp only when its content differs:

```text
$ make --no-print-directory VERSION=2.1 --trace
Makefile:35: update target 'build/version.o' due to: build/version.stamp
gcc -Iinclude -DTOOL_VERSION='"2.1"'  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/version.c -o build/version.o
Makefile:32: update target 'build/board-tool' due to: build/version.o
gcc  build/gpio.o build/main.o build/version.o  -o build/board-tool
$ ./build/board-tool && make --no-print-directory VERSION=2.1
board-tool 2.1: line 3 = 1
make: Nothing to be done for 'all'.
```

Kbuild generalizes this idea: every object has a `.cmd` file holding the exact command that produced it, and `if_changed` rebuilds when the command line differs. That is why changing a kernel config option rebuilds exactly the affected objects. For small projects, a stamp per important variable is enough. Otherwise, document that changing flags requires `make clean`.

## Asking Make What It Thinks

```text
$ make --no-print-directory print-CC print-CFLAGS print-KB_CFLAGS CFLAGS=-Os
CC=gcc  [origin: file]
CFLAGS=-Os  [origin: command line]
KB_CFLAGS=-std=c17 -Wall -Wextra  [origin: file]
$ CC=clang make --no-print-directory print-CC
CC=clang  [origin: environment]
```

A dry run shows fully expanded commands without needing the tools to exist. It is useful for checking a cross configuration on a machine without the cross compiler:

```text
$ make --no-print-directory -n --always-make CROSS_COMPILE=aarch64-linux-gnu- | head -n 3
mkdir -p build
aarch64-linux-gnu-gcc -Iinclude  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/gpio.c -o build/gpio.o
aarch64-linux-gnu-gcc -Iinclude  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/main.c -o build/main.o
```

`--always-make` treats every target as out of date, including the order-only `build` directory, which is why `mkdir` appears even though the directory exists.

Staged installation, as a packaging system calls it:

```text
$ make --no-print-directory install DESTDIR=$PWD/stage && find stage -type f
gcc -Iinclude -DTOOL_VERSION='"1.0"'  -std=c17 -Wall -Wextra -O2 -g -MMD -MP -c src/version.c -o build/version.o
gcc  build/gpio.o build/main.o build/version.o  -o build/board-tool
install -D -m 0755 build/board-tool ./stage/usr/bin/board-tool
stage/usr/bin/board-tool
```

The previous build used `VERSION=2.1`. This one runs with the default `1.0`, so the stamp changed again and `version.o` was rebuilt before installing. Without the stamp, the installed binary would silently report the wrong version.

| Tool | Answers |
| --- | --- |
| `make -n` | which commands would run, fully expanded |
| `make --trace` | why each target is rebuilt (GNU Make 4.0+) |
| `make --debug=b` or `-d` | the full decision process, very verbose |
| `make -p` | every variable and rule after reading, including built-ins |
| `make -r -R` | disable built-in rules/variables to prove you do not depend on them |
| `make --warn-undefined-variables` | catches typos such as `$(CLFAGS)` |
| `$(info ...)`, `$(warning ...)` | print during the read phase |
| `print-%` helper | one variable's value and origin |

## Failure Safety: `.DELETE_ON_ERROR`

Recipes write their targets with shell redirection. If the recipe fails after the target was created, the half-written file has a fresh timestamp, and the next `make` treats it as up to date:

```make
--8<-- "examples/build-systems/tool-sessions/make-project/delete-on-error.mk"
```

```text
$ make --no-print-directory -f delete-on-error.mk
printf '#define TABLE_SIZE 16\n' > table.h
false
make: *** [delete-on-error.mk:4: table.h] Error 1
[exit status 2]
$ ls table.h && make --no-print-directory -f delete-on-error.mk
table.h
make: 'table.h' is up to date.
$ rm -f table.h && make --no-print-directory -f delete-on-error.mk --eval .DELETE_ON_ERROR:
printf '#define TABLE_SIZE 16\n' > table.h
false
make: *** [delete-on-error.mk:4: table.h] Error 1
make: *** Deleting file 'table.h'
[exit status 2]
```

The second run "succeeded" with a truncated header. That kind of error shows up much later as a confusing compile or runtime failure. Put `.DELETE_ON_ERROR:` in every Makefile. For generators that should never expose partial output even when interrupted, write to `$@.tmp` and `mv` it into place as the last step.

## Recipes Are Shell, One Line At A Time

Every recipe line runs in a **separate** shell:

```make
bad:
	cd build
	ls          # runs in the original directory, not build/

good:
	cd build && ls
```

Other recipe rules worth knowing:

- Make stops when a line exits non-zero. Inside one line, use `&&` or `set -e` so a failing first command is not hidden by a successful last one.
- `@` hides the command echo, `-` ignores the exit status (rarely correct), `+` runs the line even under `make -n`.
- `$$` passes a literal `$` to the shell: `for f in *.c; do echo $$f; done`.
- `.ONESHELL:` runs a whole recipe in one shell. It changes the behavior of every recipe in the file, so do not add it casually to an existing Makefile.
- Make uses `/bin/sh`, which may be `dash`. Bash-only syntax needs `SHELL := /bin/bash`.

## Parallel Builds

`make -jN` runs independent targets at the same time. "Independent" means "has no edge between them in the graph you wrote", which is not the same as "really independent".

```make
--8<-- "examples/build-systems/tool-sessions/make-project/race.mk"
```

```text
$ make --no-print-directory -f race.mk
sleep 1
printf '#define BOARD_ID 7\n' > config.h
cc -c app.c -o app.o
cc app.o -o app
$ rm -f app app.o config.h && make --no-print-directory -f race.mk -j4
sleep 1
cc -c app.c -o app.o
app.c:1:10: fatal error: config.h: No such file or directory
    1 | #include "config.h"
      |          ^~~~~~~~~~
compilation terminated.
make: *** [race.mk:11: app.o] Error 1
make: *** Waiting for unfinished jobs....
printf '#define BOARD_ID 7\n' > config.h
[exit status 2]
```

The serial build worked only because make processes prerequisites left to right and `config.h` happened to come first. The fix is the real edge: `app.o: app.c config.h`. Then `-j4` works:

```text
$ rm -f app app.o config.h && make --no-print-directory -f race-fixed.mk -j4
sleep 1
printf '#define BOARD_ID 7\n' > config.h
cc -c app.c -o app.o
cc app.o -o app
```

Generated headers are the most common source of missing edges, because `-MMD` only records a header after the first successful compile. List generated headers explicitly as prerequisites of the objects that include them, or as order-only prerequisites of all objects.

Practical rules:

- Test every Makefile with `make clean && make -j$(nproc)` in CI. Yocto and Buildroot build with high parallelism by default.
- Use `make -O` (`--output-sync`) to keep each job's output together in parallel logs.
- `.NOTPARALLEL:` serializes the whole Makefile. It is a workaround that hides a missing edge, not a fix.
- Two recipes that write the same file are a race even if the graph looks right.

## Recursive Make

Large trees (the kernel, U-Boot, vendor SDK top-level Makefiles) delegate to sub-directories with `make -C dir`. Always invoke the sub-make as `$(MAKE)`:

```text
$ make --no-print-directory -f recursive.mk -j4 CC=clang
make -C lib
lib: CC=clang [command line]  MAKEFLAGS= -j4 --jobserver-auth=3,4 --no-print-directory -- CC=clang
make -C lib
make[1]: warning: jobserver unavailable: using -j1.  Add '+' to parent make rule.
lib: CC=clang [command line]  MAKEFLAGS= -j1 --no-print-directory -- CC=clang
```

The jobserver descriptor numbers (`3,4`) vary between runs and Make versions; Make 4.4 uses a named FIFO instead. What the transcript shows:

- Command-line variables (`CC=clang`) and flags reach every sub-make through the exported `MAKEFLAGS`, whether it is started as `$(MAKE)` or `make`.
- Only `$(MAKE)` is recognized as a recursive call. A literal `make` is not given the parallel **jobserver**, so that whole subtree builds with `-j1`.
- `make -n` really executes `$(MAKE)` lines (passing `-n` down), so a dry run shows the sub-make's commands. A literal `make` line is only printed.

```text
$ make --no-print-directory -f recursive.mk -n | head -n 4
make -C lib
echo 'lib: CC=cc [default]  MAKEFLAGS=n --no-print-directory'
make -C lib
```

Ordinary Makefile variables are **not** passed down unless exported (`export CROSS_COMPILE`) or put on the sub-make command line.

The deeper problem is the one in Peter Miller's paper *Recursive Make Considered Harmful*: each sub-make sees only its own part of the graph, so dependencies between directories are invisible and parallelism is limited. Options:

- **Non-recursive make**: the top Makefile `include`s a fragment per directory (`src/module.mk`) and builds one global graph.
- **Disciplined recursion**, as Kbuild does: directories are visited in a defined order, each directory's `Makefile` only lists objects (`obj-y += gpio.o`), and the top level owns the link.
- For new userspace projects with many directories, a generator such as [CMake](cmake-basics.md) with [Ninja](ninja-generated-backend.md) is usually the better tool.

## Make Functions

Functions make the Makefile follow the source tree instead of hand-maintained lists:

```make
SRCS := $(wildcard src/*.c)                      # src/gpio.c src/main.c ...
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))    # build/gpio.o ...
DEPS := $(OBJS:.o=.d)                            # substitution reference
TESTS := $(filter test_%,$(notdir $(SRCS:.c=)))  # names starting with test_
GIT_REV := $(shell git describe --always --dirty 2>/dev/null)
```

Use `:=` with `$(shell ...)` so the command runs once, not every time the variable is expanded. Note that `$(wildcard)` is evaluated when the Makefile is read: a source file generated during the build is not in the list unless you add it explicitly.

For several programs with the same shape, a `define` template plus `call` and `eval` generates the rules:

```make
BUILD := build
PROGRAMS := sensord sensorctl

sensord_SRCS     := sensord_main.c sensord.c common.c
sensorctl_SRCS   := sensorctl_main.c sensorctl.c common.c
sensorctl_LDLIBS := -lm

# Declare the default goal before eval creates any rule.
all:

# $(1) is the program name. $$ survives the call and is expanded by eval.
define PROGRAM_template
$(1)_OBJS := $$(patsubst %.c,$(BUILD)/%.o,$$($(1)_SRCS))
$(BUILD)/$(1): $$($(1)_OBJS)
	$$(CC) $$(LDFLAGS) $$^ $$(LDLIBS) $$($(1)_LDLIBS) -o $$@
endef

$(foreach prog,$(PROGRAMS),$(eval $(call PROGRAM_template,$(prog))))

all: $(addprefix $(BUILD)/,$(PROGRAMS))

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $@

.PHONY: all
```

Without the early `all:` line, the first rule created by `eval` (`build/sensord`) becomes the default goal, and a plain `make` silently builds only one program. When a template misbehaves, print what it generates: `make --eval '$(info $(call PROGRAM_template,sensorctl))' -n`.

Templates are powerful and hard to read. Use them when there are several real instances, and keep them small.

## How This Maps To Kbuild And Vendor SDKs

The kernel and U-Boot build systems are Make, using the same mechanisms at a larger scale:

| Mechanism on this page | Kbuild equivalent |
| --- | --- |
| object list per directory | `obj-y += foo.o`, `obj-$(CONFIG_FOO) += foo.o` |
| generated dependencies | the compiler's dependency output, post-processed by `fixdep` to add per-`CONFIG_` dependencies |
| stamp file for variables | `.foo.o.cmd` files plus `if_changed` |
| out-of-tree build directory | `make O=../build` |
| recursive `$(MAKE) -C` | `make -C $(KDIR) M=$(PWD) modules` for external modules |
| `CROSS_COMPILE` prefix | `make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-` |
| verbose commands | `make V=1` |

See [Kbuild Objects and Directories](advanced/linux-kernel/kbuild-objects-and-directories.md) and [Modules and External Modules](advanced/linux-kernel/modules-and-external-modules.md).

## Common Mistakes

- Never testing a no-op rebuild, an incremental rebuild after touching a header, or `make clean && make -j$(nproc)`.
- Listing directories as normal prerequisites instead of order-only ones.
- Forgetting `-MP`, then needing `make clean` whenever a header is deleted or renamed.
- Expecting `make CFLAGS=...` or `make VERSION=...` to rebuild an up-to-date tree.
- Omitting `.DELETE_ON_ERROR:` and shipping a truncated generated file.
- Using `cd dir` on its own recipe line.
- Writing `make -C sub` instead of `$(MAKE) -C sub`.
- Adding `.NOTPARALLEL:` or `-j1` to hide a missing dependency.
- Using `$(wildcard)` for files that are generated during the build.
- Letting a template or included file silently change the default goal.

## Debugging Checklist

- Does a second `make` print `Nothing to be done`? If not, use `--trace` to see what it rebuilds and why.
- After `touch` on a header, does `--trace` rebuild exactly that header's users?
- Does `make clean && make -j$(nproc)` pass repeatedly? Run it several times; races are intermittent.
- Do the `.d` files list the headers you expect?
- What does `make print-VAR` say about the value and origin?
- Does `make -n` show the compiler, flags, and paths you expect?
- Is the default goal what you think? Check the first rule after all includes and templates, or set `.DEFAULT_GOAL`.
- In a recursive build, is each sub-make started with `$(MAKE)`, and are needed variables exported?
- Did a failed generator leave a file behind? Check for `.DELETE_ON_ERROR`.

## Exercises

1. Run `examples/build-systems/tool-sessions/make-project/run.sh` and read the full transcript.
2. Remove `-MP` from the project, repeat the "delete a header" step, and record the error.
3. Add a generated header `build/board.h` produced by a script. Make the build pass `make clean && make -j16` ten times in a row.
4. Add a `CFLAGS` stamp so that changing `CFLAGS` rebuilds every object but nothing else.
5. Split the project into `src/` and `lib/` with a recursive `$(MAKE)`, then convert it to a non-recursive `include`-based layout and compare `make -j` behavior.
6. Read `scripts/Kbuild.include` in a kernel tree and find `if_changed`; explain how it relates to the stamp file.

## Related Topics

- [Make Basics](make-basics.md)
- [Make Variables and Pattern Rules](make-variables-and-pattern-rules.md)
- [Install Rules and Staging](install-rules-and-staging.md)
- [Ninja as a Generated Backend](ninja-generated-backend.md)
- [Kbuild Objects and Directories](advanced/linux-kernel/kbuild-objects-and-directories.md)
- [Yocto Recipes: Makefile-based software](advanced/yocto-openembedded/recipes.md#makefile-based-software)

## References

- [GNU Make manual](https://www.gnu.org/software/make/manual/make.html)
- [GNU Make: Types of prerequisites (order-only)](https://www.gnu.org/software/make/manual/html_node/Prerequisite-Types.html)
- [GNU Make: Errors in recipes and `.DELETE_ON_ERROR`](https://www.gnu.org/software/make/manual/html_node/Errors.html)
- [GNU Make: Recursive use of make](https://www.gnu.org/software/make/manual/html_node/Recursion.html)
- [GNU Make: Eval function](https://www.gnu.org/software/make/manual/html_node/Eval-Function.html)
- [GCC preprocessor options (`-MMD`, `-MP`)](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html)
- [Linux kernel: Kbuild makefiles](https://docs.kernel.org/kbuild/makefiles.html)
- Peter Miller, *Recursive Make Considered Harmful* (1997)
