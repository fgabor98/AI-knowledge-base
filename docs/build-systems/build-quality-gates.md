---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Build Quality Gates

## What Problem Does This Solve?

Build systems should do more than produce binaries. They should enforce enough quality checks that broken code, unsafe warnings, missing tests, ABI changes, license issues, or image regressions are caught before they reach hardware or releases.

Quality gates are the checks that a build must pass before artifacts are accepted.

## Core Concepts

- warnings policy
- static analysis
- unit tests
- integration tests
- cross-test strategy
- emulator tests
- sanitizers
- coverage
- ABI checks
- image size checks
- license checks
- CI gating

## Mental Model

Use a layered approach:

```text
fast local checks
-> native unit tests
-> static analysis
-> cross-build checks
-> emulator or hardware smoke tests
-> release artifact checks
```

Not every check belongs in every local build. Fast checks should run often. Expensive checks can run in CI or before release.

## Syntax / API / Mechanism

Compiler warnings:

```sh
gcc -Wall -Wextra -Werror -c main.c -o main.o
```

Native tests with CMake:

```cmake
enable_testing()
add_executable(test-parser tests/test_parser.c src/parser.c)
add_test(NAME parser COMMAND test-parser)
```

Run:

```sh
ctest --test-dir build --output-on-failure
```

Sanitizer example:

```sh
gcc -fsanitize=address,undefined -g test_parser.c parser.c -o test-parser
./test-parser
```

Coverage example:

```sh
gcc --coverage test_parser.c parser.c -o test-parser
./test-parser
gcov parser.c
```

## Minimal Example

Makefile targets:

```make
.PHONY: all check clean

all: app

check: test-parser
	./test-parser

test-parser: tests/test_parser.c src/parser.c
	$(CC) $(CFLAGS) -O0 -g $^ -o $@
```

Run:

```sh
make
make check
```

## Real-World Example

An embedded Linux userspace project might use:

```text
local developer:
  make
  make check

CI build:
  native unit tests
  clang-tidy or cppcheck
  cross-compile target binary
  package install test into staging directory
  image size check

hardware CI:
  flash image
  boot smoke test
  service startup check
  peripheral smoke test
```

The build system owns the repeatable commands. The CI system owns when and where they run.

## Common Scenarios

### Warnings As Errors

`-Werror` is useful in CI for code you control. It can be painful when porting third-party code or moving compiler versions.

Use deliberately:

```text
first-party code: strict warnings
third-party imports: patch or suppress carefully
vendor BSP code: avoid broad churn unless required
```

### Native Unit Tests For Target Code

Separate portable logic from hardware access so it can be tested natively:

```text
parser.c       -> native tests
state_machine.c -> native tests
gpio_linux.c   -> target or mocked tests
```

This gives fast feedback without a board.

### Cross-Compiled Tests

Target tests may need:

- QEMU user-mode
- full system emulation
- a real board
- a hardware-in-the-loop setup

Do not run target test binaries on the host by accident.

### Static Analysis

Static analysis can run on source before target deployment:

```text
cppcheck
clang-tidy
compiler warnings
include-what-you-use
```

For cross-builds, tools need the same include paths and compile definitions as the real build. `compile_commands.json` is often the bridge.

### Image Size Checks

Embedded images have storage limits. CI can check:

- rootfs size
- boot partition size
- kernel image size
- initramfs size
- package growth

### ABI Checks

If you ship shared libraries or SDKs, check whether public ABI changes are intentional. This matters when applications are built separately from the root filesystem.

### License And SBOM Checks

Build frameworks such as Yocto can generate license manifests and source archives. Treat these as release gates, not afterthoughts.

## Common Mistakes

- Only checking that compilation succeeds.
- Running no native tests because the product is embedded.
- Running target binaries on the host in CI.
- Enabling `-Werror` globally for third-party code without a porting plan.
- Letting static analysis use different flags than the real build.
- Ignoring image size growth until release week.
- Treating license and SBOM output as paperwork instead of build artifacts.
- Running expensive checks on every local edit and making developers bypass them.

## Debugging Checklist

- Separate fast local checks from slower CI gates.
- Confirm tests are built for the machine where they run.
- Use verbose build output to verify analysis flags.
- Check whether `compile_commands.json` matches the active build.
- Track image size over time.
- Archive test logs with build artifacts.
- Make quality gate failures specific and actionable.
- Allow deliberate, reviewed exceptions for third-party or vendor code.

## Related Topics

- [Ninja as a Generated Backend](ninja-generated-backend.md)
- [CMake Basics](cmake-basics.md)
- [Native Linux Userspace Builds](native-linux-userspace-builds.md)
- [Embedded Productization](../embedded-productization/index.md)

## References

- CTest documentation
- GCC instrumentation options
- Clang sanitizers documentation
- Yocto Project test and license documentation
