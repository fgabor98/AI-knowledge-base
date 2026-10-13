---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 09. Pointer Into A Reallocated Array

A pointer to an element of a growable array is saved, then the array grows. `realloc()`
may move the storage, which leaves the saved pointer pointing into freed memory. Whether
it moves depends on the allocator's state, so in testing the bug is usually invisible.
In this reproducer glibc grew the block in place, and the program printed the correct
answer.

## Symptom

Rare corruption in an object that "has not been touched", a crash in unrelated code
much later, or a wrong value that only appears under load or after many insertions.
The failure appears long after the bug and far from the code that caused it.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/09-pointer-into-reallocated-array/bug.c"
```

```text
$ gcc -O2 -Wall -Wextra bug.c -o bug && ./bug
first = alpha
```

The output is correct, but only because of how glibc's allocator behaved in this
run.

## What Happened

The third `list_push` grows the array from 2 to 4 entries. In the plain glibc build
the block happened to have free space after it, so `realloc` grew it in place and
returned the same address. (Printing `before != list.items` in a variant of the
reproducer gave `moved = 0`.) With a different heap history (more allocations, another
thread, a different libc, an RTOS heap), the block moves, `first` points into freed
memory, and the read returns whatever the allocator or the next user put there.

AddressSanitizer's `realloc` always allocates a new block and puts the old one in
quarantine, which is why the ASan build reports the bug deterministically.

## The Rule

`realloc` deallocates the old object and returns a pointer to a new object, which
"may have the same value" as the old pointer (C11 7.22.3.5p2–4). After an object's
lifetime ends, the value of every pointer to it becomes indeterminate (C11 6.2.4p2).
So `first` is invalid even in the run where the address did not change. Code that
checks "did it move?" by comparing the old and new pointers relies on that
indeterminate value.

## Why Review And Testing Missed It

- Caching a pointer is local and looks harmless. The growth happens in a different
  function.
- Tests that insert only a few elements never trigger growth.
- When growth does happen, it is often done in place, and the stale pointer is still
  correct by accident.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` | silent |
| `-fanalyzer` | `use after 'free' of 'first' [CWE-416] [-Wanalyzer-use-after-free]`, with the path through "when 'realloc' succeeds, moving buffer" |
| `-fsanitize=address` | `heap-use-after-free ... READ of size 8`, with the `realloc` stack as both "freed by" and "previously allocated by" |

The analyzer also reported `dereference of NULL 'first'`. This is a separate, real
issue: the reproducer ignores the result of `list_push`, so if the first `realloc`
fails, `list.items` is NULL. The fixed version checks every result.

The analyzer finds this case because the growth and the cached pointer are in the same
translation unit. When the container is in a library, ASan with a test that forces
growth is the reliable detector.

## Fix

Store a position instead of an address, and look it up again after any operation that
can reallocate:

```c
--8<-- "examples/c/failure-case-studies/09-pointer-into-reallocated-array/fixed.c:27:43"
```

The fixed program runs cleanly under ASan.

Other designs, depending on what the code needs:

- Allocate elements individually and store pointers to them, so growing the index
  array does not move the elements.
- Use a chunked or pooled container that never moves existing elements.
- Reserve the final capacity up front when the maximum is known. Embedded systems
  often prefer this anyway.

## Keep It Fixed

- Document in the container API which operations invalidate element pointers, as
  C++ does for `std::vector`.
- Test with a growth factor and initial capacity that force reallocation on almost
  every insertion, and run those tests under ASan.
- In debug builds, make the grow function always allocate a new block, copy, and
  `free` the old one, as ASan's `realloc` does. This stops in-place growth from
  hiding the bug.

## Same Root Cause Elsewhere

- Pointers into a buffer that is compacted, defragmented, or swapped (double
  buffering).
- Pointers returned by a "get element" API, kept across calls that can resize.
- `char *` pointers into a string being built with `realloc`-based appends.
- In RTOS code: a pointer to a queue slot or pool block kept after the slot has been
  released to another task.

## Review Question

> Which operations can move this storage, and does any pointer into it live across
> one of them?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Memory Safety And Lifetime](../semantics-and-memory/memory-safety-and-lifetime.md)
- [Ownership And Resource Lifetimes](../modular-design-and-apis/ownership-and-resource-lifetimes.md)
- [Advanced Data Structures](../advanced-c/advanced-data-structures.md)

## References

- C11 (N1570) 7.22.3.5, `realloc`; 6.2.4p2, pointers to objects whose lifetime has
  ended
- [AddressSanitizer documentation](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [GCC `-Wanalyzer-use-after-free`](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html)
