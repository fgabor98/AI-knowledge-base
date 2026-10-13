---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 03. Out-Of-Bounds Loop

An off-by-one loop bound reads one element past an array. The optimizer does not
compile that extra read faithfully. It uses the read to conclude that the loop must
return before reaching it, and turns the whole lookup into `return true`.

## Symptom

A membership test reports "found" for values that are not in the table: an
allow-list accepts everything, or a "known device" check accepts any ID. The debug
build behaves correctly.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/03-out-of-bounds-loop/bug.c"
```

```text
$ gcc -O0 bug.c -o bug-O0 && ./bug-O0
exists_in_table(42) = 0
$ gcc -O2 -Wall -Wextra -Warray-bounds=2 bug.c -o bug-O2 && ./bug-O2
exists_in_table(42) = 1
```

The table contains only zeros, so 42 is not in it.

## What The Compiler Did

```text
$ gcc -S -O2 bug.c                      # exists_in_table
exists_in_table:
	mov	eax, 1
	ret
```

The reasoning goes as follows:

1. Iteration `i == 4` reads `table[4]`, which is undefined.
2. So in any defined execution, the loop returns before `i == 4`.
3. The loop can only return early through `return true`.
4. So every defined execution returns `true`, and the loop and the table read can be
   removed.

GCC calls this "aggressive loop optimizations": it derives an upper bound on the
iteration count from undefined accesses inside the loop.

## The Rule

Pointer arithmetic and subscripting are defined only within an array object and one
element past its end, and the one-past-the-end element must not be read
(C11 6.5.6p8). `table[4]` on an `int[4]` is undefined even though the address exists
and the read does not trap.

## Why Review And Testing Missed It

- `<=` against a literal `4` looks like "the table has four entries".
- At `-O0` the extra read returns whatever follows the array, usually zero. The
  function then gives correct results for every non-zero value, which covers most
  test inputs.
- `-Wall -Wextra -Warray-bounds=2` was silent at `-O2`, even though the whole loop
  was removed.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra -Warray-bounds=2` | silent |
| `-fanalyzer` | silent |
| `-fsanitize=undefined` | `runtime error: index 4 out of bounds for type 'int [4]'` |
| `-fsanitize=address` | `global-buffer-overflow ... 0 bytes after global variable 'table'` |

Both sanitized builds printed `exists_in_table(42) = 0`, the correct answer. The
instrumentation keeps the read, so the optimizer can no longer derive the loop bound
from it, and the wrong answer disappears in the build you test. Treat a sanitizer
report as a failure even if the program's output is correct.

## Fix

Derive the bound from the array and use a half-open range:

```c
--8<-- "examples/c/failure-case-studies/03-out-of-bounds-loop/fixed.c:6:19"
```

```text
exists_in_table(42) = 0
exists_in_table(0) = 1
```

## Keep It Fixed

- Use `i < ARRAY_SIZE(x)` (or the API's length parameter) for loop bounds. Do not
  repeat a literal size.
- Pass arrays together with their length. A function that receives only a pointer
  cannot check its bound.
- Run table-driven tests under UBSan and ASan, including a value that is *not*
  present. That is the input this bug gets wrong.

## Same Root Cause Elsewhere

GCC 4.8 introduced this analysis, and its release notes warn that non-conforming
programs may stop working. SPEC CPU 2006 `464.h264ref` was one of them: a loop read
one element past an array, and the loop was compiled as if it ran forever. The same
pattern appears when a loop reads `buf[i + 1]` on its last iteration, or when the
last element of a struct array is used as a sentinel that was never allocated.

## Review Question

> What is the last index this loop reads, and is it strictly less than the length of
> the object?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Pointer Arithmetic And Bounds](../semantics-and-memory/pointer-arithmetic-and-bounds.md)
- [Arrays, Strings, And Buffers](../language-fundamentals/arrays-strings-and-buffers.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)

## References

- C11 (N1570) 6.5.6p8, additive operators and array bounds
- [GCC 4.8 release notes: aggressive loop optimizations](https://gcc.gnu.org/gcc-4.8/changes.html)
- [GCC `-faggressive-loop-optimizations`](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
