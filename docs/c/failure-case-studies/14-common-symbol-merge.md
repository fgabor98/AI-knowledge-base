---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 14. Common Symbol Merge

Two files each define a global with the same name, without `static`, `extern`, or an
initializer. With `-fcommon`, the linker merges them into a single object, even though
one is an array and the other a counter. Writes to one now change the other.

## Symptom

A counter jumps to a value such as `1280066888` whenever a message arrives. In a hex
dump that is `0x4c4c4548`, which is ASCII `"HELL"`: the first bytes of the received
message. Two modules written by different people, which never reference each other,
share storage.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/14-common-symbol-merge/bug/uart.c"
```

```c
--8<-- "examples/c/failure-case-studies/14-common-symbol-merge/bug/stats.c"
```

```text
$ gcc -O2 bug/uart.c bug/stats.c bug/main.c
/usr/bin/ld: ...: multiple definition of `rx_buffer'; ...: first defined here
collect2: error: ld returned 1 exit status

$ gcc -O2 -Wall -Wextra -fcommon bug/uart.c bug/stats.c bug/main.c -o bug && ./bug
rx count = 1280066888 (0x4c4c4548)
$ nm -S bug | grep rx_buffer
0000000000004020 0000000000000010 B rx_buffer
```

There is a single `rx_buffer` symbol in the image, 16 bytes in size, used by both
modules.

## What The Linker Did

With `-fcommon`, a file-scope declaration without an initializer or storage-class
specifier is emitted as a *common* symbol. The linker merges all common symbols with
the same name into one allocation, sized for the largest one. This is the Fortran
`COMMON` block model that Unix C toolchains supported for decades. Since GCC 10 the
default is `-fno-common`, which emits a normal definition, and the duplicate becomes
the link error shown above.

## The Rule

In ISO C, `unsigned int rx_buffer;` at file scope is a *tentative definition*, which
becomes a definition at the end of the translation unit (C11 6.9.2p2). A program may
contain at most one external definition of each identifier (C11 6.9p5), so two of them
are undefined behavior. Annex J.5.11 lists "multiple external definitions" as a common
extension. That extension is what `-fcommon` implements, with no type check.

## Why Review And Testing Missed It

- Each file is correct on its own.
- Many embedded and legacy codebases build with `-fcommon` (or with GCC 9 or earlier,
  where it is the default), because old code depended on it and broke with GCC 10.
- `-Wall -Wextra` is a per-file analysis and cannot see the clash.

## Detection

| Tool | Result |
| --- | --- |
| GCC 10+ default (`-fno-common`) | link error: `multiple definition of 'rx_buffer'` |
| `-fcommon -Wall -Wextra` | silent |
| `-fcommon -Wl,--warn-common` | `warning: common of 'rx_buffer' overridden by larger common` |
| `-fcommon -flto` | `type of 'rx_buffer' does not match original declaration [-Wlto-type-mismatch]` |

Unlike [case 12](./12-enum-size-abi-drift.md), LTO catches this one: here the two
declarations have different types in the source, while in case 12 the source was
identical and only the compiler flags differed.

## Fix

Module state gets internal linkage, and anything shared has exactly one definition and
an `extern` declaration in a header:

```c
--8<-- "examples/c/failure-case-studies/14-common-symbol-merge/fixed/uart.c"
```

```text
$ gcc -O2 -Wall -Wextra -fcommon fixed/uart.c fixed/stats.c fixed/main.c -o fixed && ./fixed
rx count = 2 (0x00000002)
```

The fix is independent of the compiler flag: the fixed sources link and behave the
same with `-fcommon` and with `-fno-common`.

## Keep It Fixed

- Build with `-fno-common` (the default in current GCC and Clang). If a legacy
  component needs `-fcommon`, limit it to that component and add
  `-Wl,--warn-common` to the link.
- Code review rule: every file-scope object is either `static` or declared `extern` in
  exactly one header and defined in exactly one `.c` file. Clang's
  `-Wmissing-variable-declarations` enforces this.
- Check the map file or `nm` output for unexpectedly large or shared `.bss` symbols
  when integrating third-party code.

## Same Root Cause Elsewhere

- Two libraries each defining `debug_level`, `buffer`, or `state` with external
  linkage. With static archives the linker may silently pick one of them.
- Global symbol interposition in shared libraries: an executable's `log_init` replaces
  a library's internal `log_init`. Use `static` and symbol visibility to prevent it.
- Header files that *define* a variable (`int counter;` in a `.h`). With `-fcommon`
  this "works", and every includer shares one object.

## Review Question

> Does every global in this file have `static`, or exactly one `extern` declaration in
> a header that this file includes?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Storage Duration, Scope, And Linkage](../semantics-and-memory/storage-duration-scope-and-linkage.md)
- [Translation Units And Headers](../modular-design-and-apis/translation-units-and-headers.md)
- [Static And Dynamic Linking](../compilation-linking-and-abi/static-and-dynamic-linking.md)

## References

- C11 (N1570) 6.9p5, one external definition; 6.9.2, tentative definitions; J.5.11,
  multiple external definitions
- [GCC 10 porting notes: default to `-fno-common`](https://gcc.gnu.org/gcc-10/porting_to.html)
- [GNU ld `--warn-common`](https://sourceware.org/binutils/docs/ld/Options.html)
