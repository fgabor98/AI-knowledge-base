---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 07. snprintf() Accumulation

`snprintf()` returns the length the output *would* have had, not the number of bytes
it wrote. Adding that return value to an offset works until the first truncation.
After that, the offset is past the end of the buffer, `size - used` wraps to a huge
value, and the next call writes past the buffer.

## Symptom

A status line, log record, or diagnostic string is formatted correctly in testing. On
a device with more sensors, longer names, or larger values, memory next to the buffer
is overwritten. With no stack protector, nothing reports the corruption.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/07-snprintf-accumulation/bug.c"
```

Without hardening, as with a typical bare-metal newlib build:

```text
$ gcc -O2 -Wall -Wextra -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
      -fno-stack-protector bug.c -o bug-plain && ./bug-plain
used = 67 (buffer is 32)
neighbour = "NEIGHBOUsensor3"
```

The neighbouring field has been overwritten, and the program exits with status 0.

## What Happened

| Call | `size - used` | Bytes written | Return | `used` after |
| --- | ---: | --- | ---: | ---: |
| 0 | 32 | `sensor0=1000;` | 13 | 13 |
| 1 | 19 | `sensor1=-2000;` | 14 | 27 |
| 2 | 5 | `sens` + NUL (truncated) | 13 | 40 |
| 3 | `32 - 40` → `SIZE_MAX - 7` | **unbounded**, at `out + 40` | 14 | 54 |
| 4 | `SIZE_MAX - 21` | **unbounded**, at `out + 54` | 13 | 67 |

Call 2 truncates correctly but returns 13. After that, `used` exceeds `size`, the
`size_t` subtraction wraps, and `snprintf` is told it has almost unlimited space.

## The Rule

C11 7.21.6.5p3: `snprintf` returns "the number of characters that would have been
written had `n` been sufficiently large", or a negative value on an encoding error.
Output was truncated if and only if the return value is non-negative and `>= n`.

## Why Review And Testing Missed It

- `used += snprintf(...)` is a common idiom, and it is correct until the first
  truncation.
- `-Wall -Wextra` cannot see that `used` can exceed `size`.
- On Ubuntu, GCC enables `_FORTIFY_SOURCE=3` and `-fstack-protector-strong` by
  default, so a desktop test build may abort, and the abort may be filed as a crash
  rather than understood as an overflow. Embedded toolchains usually have neither.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` | silent |
| `-fanalyzer` | silent |
| Ubuntu default flags (`_FORTIFY_SOURCE=3`) | `*** buffer overflow detected ***: terminated` |
| `-fsanitize=address` | `stack-buffer-overflow ... WRITE of size 15 ... in format_readings` |

ASan identifies the write and the object it overflows:

```text
  This frame has 2 object(s):
    [48, 68) 'values' (line 22)
    [112, 160) 'frame' (line 21) <== Memory access at offset 160 overflows this variable
```

## Fix

Treat `ret < 0 || ret >= remaining` as the end of formatting, and report truncation
to the caller:

```c
--8<-- "examples/c/failure-case-studies/07-snprintf-accumulation/fixed.c:6:25"
```

```text
complete = 0 text = "sensor0=1000;sensor1=-2000;sens"
neighbour = "NEIGHBOUR-OK"
```

Whether truncated output is acceptable (logs) or an error (protocol messages) is an
API decision. The return type makes the caller decide.

## Keep It Fixed

- Put formatting behind one helper with a clamped offset, such as an `append()` on a
  `{char *buf; size_t size; size_t used; bool truncated;}` builder. The Linux kernel
  provides `scnprintf()` for this reason: it returns the number of bytes actually
  written.
- Fuzz or property-test formatters with maximum-length inputs and small buffers.
- Keep `_FORTIFY_SOURCE` and the stack protector enabled in host test builds, and run
  the same tests under ASan.

## Same Root Cause Elsewhere

- `strlcpy`/`strlcat` also return the length they *tried* to create, and the same
  accumulation bug follows.
- Any API that reports the size it *needed*, such as `vsnprintf` or many vendor
  `get_info(buf, len)` calls, needs `ret >= len` handling before the result is used
  as an offset.
- Ignoring the return value entirely is the safe-but-wrong variant: output is silently
  cut and nobody knows.

## Review Question

> When this call truncates, what is the value of the offset afterwards, and is it
> still `<= size`?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Memory And String APIs](../standard-library-and-ecosystem/memory-and-string-apis.md)
- [Arrays, Strings, And Buffers](../language-fundamentals/arrays-strings-and-buffers.md)
- [06. read() Error Passes A Length Check](./06-read-error-passes-length-check.md)

## References

- C11 (N1570) 7.21.6.5, `snprintf`
- [Linux kernel `scnprintf()`](https://docs.kernel.org/core-api/kernel-api.html)
- [GCC `_FORTIFY_SOURCE` and object-size checking](https://gcc.gnu.org/onlinedocs/gcc/Object-Size-Checking.html)
