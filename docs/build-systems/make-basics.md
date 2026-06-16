---
status: draft
reviewed: false
domain: build-systems
difficulty: beginner
reviewer: null
last_reviewed: null
---

# Make Basics

## What Problem Does This Solve?

Make records how files depend on each other and which commands produce them. It prevents you from manually retyping compiler commands and rebuilds only the targets affected by changed inputs.

Make is especially important for embedded Linux developers because it appears in the Linux kernel, U-Boot, BusyBox, Buildroot, vendor SDKs, and many driver examples.

## Core Concepts

- Makefile
- target
- prerequisite
- recipe
- timestamp-based rebuilds
- default target
- phony target
- incremental build

## Mental Model

A Makefile is a dependency graph plus commands:

```text
target: prerequisites
	command to build target
```

Make asks:

```text
Does the target exist?
Are any prerequisites newer than the target?
If yes, run the recipe.
```

For C projects:

```text
main.c -> main.o
util.c -> util.o
main.o + util.o -> app
```

Make is not a C-specific tool. It does not understand headers, compilers, or linkers by itself. A Makefile teaches Make the relationship between files. Some built-in rules exist, but production Makefiles should be explicit enough that the build is readable and debuggable.

## Syntax / API / Mechanism

Basic rule:

```make
target: prerequisite
	command
```

Important detail: recipe lines must start with a tab character.

Common targets:

```make
app: main.o util.o
	gcc main.o util.o -o app

main.o: main.c util.h
	gcc -c main.c -o main.o

util.o: util.c util.h
	gcc -c util.c -o util.o

.PHONY: clean
clean:
	rm -f app *.o
```

Run:

```sh
make
make clean
```

Useful commands:

```sh
make app
make -n
make -B
make --debug=b
make -j4
```

- `make app` builds a specific target.
- `make -n` prints recipes without running them.
- `make -B` treats all targets as out of date.
- `make --debug=b` explains basic rebuild decisions.
- `make -j4` runs up to four jobs in parallel.

## Minimal Example

Given:

```text
main.c
util.c
util.h
```

Use this `Makefile`:

```make
app: main.o util.o
	gcc main.o util.o -o app

main.o: main.c util.h
	gcc -c main.c -o main.o

util.o: util.c util.h
	gcc -c util.c -o util.o

.PHONY: clean
clean:
	rm -f app main.o util.o
```

Build:

```sh
make
```

Clean:

```sh
make clean
```

## Real-World Example

A simple board utility can start with a Makefile like this:

```make
app: main.o gpio.o config.o
	gcc main.o gpio.o config.o -lgpiod -o board-tool

main.o: src/main.c include/gpio.h include/config.h
	gcc -Iinclude -Wall -Wextra -O2 -c src/main.c -o main.o

gpio.o: src/gpio.c include/gpio.h
	gcc -Iinclude -Wall -Wextra -O2 -c src/gpio.c -o gpio.o

config.o: src/config.c include/config.h
	gcc -Iinclude -Wall -Wextra -O2 -c src/config.c -o config.o

.PHONY: clean
clean:
	rm -f board-tool *.o
```

This is still repetitive. The next step is to introduce variables, pattern rules, and automatic variables.

## Common Scenarios

### Nothing Happens When Running `make`

Symptom:

```text
make: 'app' is up to date.
```

This usually means the target exists and its prerequisites are older. Check timestamps:

```sh
ls -l app main.o util.o main.c util.c util.h
```

If the Makefile forgot a header prerequisite, Make may incorrectly think an object is up to date.

### `missing separator`

Symptom:

```text
Makefile:2: *** missing separator.  Stop.
```

The usual cause is spaces instead of a tab before a recipe command:

```make
app: main.o
	gcc main.o -o app
```

The line before `gcc` must start with a tab.

### Header Changes Are Ignored

If `util.h` changes, both `main.o` and `util.o` may need to rebuild. Basic Makefiles must list those header dependencies manually:

```make
main.o: main.c util.h
util.o: util.c util.h
```

Larger Makefiles usually generate dependency files automatically with compiler options such as `-MMD -MP`.

### Parallel Build Fails But Serial Build Works

If `make` works but `make -j4` fails, the Makefile may be missing dependencies. Parallel builds expose incomplete dependency graphs because Make runs independent-looking targets at the same time.

Example mistake:

```make
app:
	gcc main.o util.o -o app
```

This target does not tell Make that `app` depends on `main.o` and `util.o`.

### A Phony Target Conflicts With A File

If a file named `clean` exists and the Makefile does not mark `clean` as phony, `make clean` may do nothing.

Use:

```make
.PHONY: clean
clean:
	rm -f app *.o
```

### Recursive Make In A Small Project

Beginners often create one Makefile per directory too early. For small projects, one clear Makefile is easier to reason about. Recursive Make becomes relevant later for large systems, but it introduces dependency visibility problems if used casually.

## Common Mistakes

- Using spaces instead of a tab at the start of a recipe line.
- Forgetting to list a header as a prerequisite.
- Forgetting `.PHONY` for targets like `clean`.
- Naming the output incorrectly in the recipe.
- Assuming Make understands C dependencies automatically without being told.
- Running `make clean` and deleting files that are not build outputs.
- Forgetting to put real prerequisites on the final executable target.
- Writing recipes that depend on the current shell directory in surprising ways.
- Hiding errors with a leading `-` before commands without understanding the consequence.
- Using recursive Make before the dependency graph requires it.

## Debugging Checklist

- Run `make --dry-run` to see what would execute.
- Run `make --always-make` to force recipes.
- Run `make --debug=b` to inspect basic rebuild decisions.
- Check whether recipe lines begin with tabs.
- Check whether the target file exists.
- Check timestamps on targets and prerequisites.
- Check whether headers are listed as prerequisites.
- Run with `make -j1` and then `make -j4` if a race is suspected.
- Check whether the first target in the file is the intended default target.
- Check whether a target name conflicts with a real file.

## Related Topics

- [Direct Compiler Invocation](direct-compiler-invocation.md)
- [Object Files and Linking](object-files-and-linking.md)
- [Make Variables and Pattern Rules](make-variables-and-pattern-rules.md)
- [Linux Kernel Build System Roadmap](linux-kernel-build-roadmap.md)

## References

- GNU Make manual
- `man make`
