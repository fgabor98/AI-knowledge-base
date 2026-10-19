---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 06. read() Error Passes A Length Check

`read()` returns `-1` on error. Comparing that `ssize_t` with `sizeof(...)` converts
`-1` to the largest `size_t`, so the "too short" check passes and the error is treated
as a very large frame.

## Symptom

A receive path that should drop short reads instead tries to parse a frame of
18 quintillion bytes. This happens rarely, only when the descriptor is non-blocking,
interrupted by a signal, or reset by the peer, which is why it reaches the field.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/06-read-error-passes-length-check/bug.c"
```

```text
$ gcc -O2 -Wall bug.c -o bug && ./bug
received = -1 accepted; payload_length = 18446744073709551611
```

The read fails deterministically with `EAGAIN`, because the pipe is non-blocking and
empty.

## What Happened

There is no undefined behavior here, and no optimizer involvement. The program does
exactly what the standard says:

1. `received` is `ssize_t` (`long`); `sizeof(struct header)` is `size_t`
   (`unsigned long`).
2. They have the same rank, so the usual arithmetic conversions convert the signed
   operand to unsigned.
3. `-1` converts to `ULONG_MAX` (C11 6.3.1.3p2: add `ULONG_MAX + 1` until the value
   is in range).
4. `ULONG_MAX < 4` is false, so the error return passes the check.
5. `received - sizeof(struct header)` is computed in `size_t` and gives
   `ULONG_MAX - 4`.

## The Rule

C11 6.3.1.8p1 (usual arithmetic conversions): if the unsigned operand's rank is
greater than or equal to the signed operand's rank, the signed operand is converted to
the unsigned type. 6.3.1.3p2 defines that conversion modulo 2^N, so the result is fully
defined and wrong.

## Why Review And Testing Missed It

- `if (n < sizeof hdr)` reads naturally, and the `n == -1` case is easy to forget once
  a function has an "error handling" block somewhere else.
- `-Wall` is silent for C. `-Wsign-compare` is part of `-Wall` only for C++. For C it
  comes with `-Wextra`.
- Tests with blocking sockets on a quiet loopback never hit `EAGAIN` or `EINTR`.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall` | silent |
| `gcc -Wall -Wextra` | `comparison of integer expressions of different signedness: 'ssize_t' and 'long unsigned int' [-Wsign-compare]` |
| `-fanalyzer` | silent |
| UBSan / ASan | not applicable: the arithmetic is defined, and in this reproducer nothing is accessed |

In a real parser the next step would be an out-of-bounds read, which ASan or a
fuzzer would catch, but only if a test hits the error path.

## Fix

Handle the error domain first. Once the value is known to be non-negative, convert
it once, with a comment saying why the conversion is exact:

```c
--8<-- "examples/c/failure-case-studies/06-read-error-passes-length-check/fixed.c:15:41"
```

```text
read failed: Resource temporarily unavailable
```

The fixed program builds cleanly with `-Wconversion -Wsign-conversion`.

## Keep It Fixed

- Build C with `-Wextra` (or at least `-Wsign-compare`). Consider
  `-Wsign-conversion` for I/O and parsing code. It reports more, but the
  findings are in the code that handles untrusted lengths.
- Wrap I/O in a helper that returns a status and a `size_t` count separately, so
  signed "count or error" values do not spread through the parser.
- Test I/O paths with injected `EINTR`, `EAGAIN`, `ECONNRESET`, and short reads, for
  example by using a fake `read` or `LD_PRELOAD` shim in host tests.

## Same Root Cause Elsewhere

- `recv`, `write`, `snprintf` (see [case 07](./07-snprintf-accumulation.md)), `ftell`,
  and many vendor HAL functions return "count or negative error" in a signed type.
- A signed `int len` from a wire protocol fails in the opposite direction. With
  `len > (int)sizeof buffer` the comparison is signed, a negative length passes, and
  `memcpy(dst, src, len)` then converts it to a huge `size_t`. With
  `len > sizeof buffer` the comparison is unsigned and rejects negative values, but by
  accident. The fix is the same in both cases: reject negative values explicitly,
  then convert once.

## Review Question

> Can this signed value be negative here, and does any comparison with a `size_t`
> come before the check for that?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Conversions, Promotions, And Aliasing](../semantics-and-memory/conversions-promotions-and-aliasing.md)
- [I/O, Diagnostics, And Errors](../standard-library-and-ecosystem/io-diagnostics-and-errors.md)
- [POSIX And System Interfaces](../standard-library-and-ecosystem/posix-and-system-interfaces.md)

## References

- C11 (N1570) 6.3.1.3p2, signed to unsigned conversion; 6.3.1.8p1, usual arithmetic
  conversions
- [POSIX `read()`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/read.html)
- [GCC `-Wsign-compare`](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- SEI CERT C INT02-C, *Understand integer conversion rules*
