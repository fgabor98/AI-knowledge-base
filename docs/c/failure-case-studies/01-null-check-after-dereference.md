---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# 01. NULL Check After Dereference

A defensive NULL check that comes *after* the first dereference does nothing. Once the
pointer has been dereferenced, the compiler may assume it was not NULL and delete the
check.

## Symptom

Read in review, the function returns `-1` for a NULL pointer. In the shipped binary
there is no NULL check at all. On a
hosted system the dereference usually faults first. On a microcontroller, or in a
kernel where address 0 is mapped, the read succeeds and execution continues with a
"validated" pointer.

## Reproducer

```c
--8<-- "examples/c/failure-case-studies/01-null-check-after-dereference/bug.c"
```

The dereference and the check are two lines apart. In real code they are often further
apart, for example when a local variable is initialized from `dev->...` at the top of
the function and the validation block comes after the declarations.

## What The Compiler Did

```text
$ gcc -S -O2 bug.c                      # device_flags
device_flags:
	mov	eax, DWORD PTR [rdi]
	ret

$ gcc -S -O2 -fno-delete-null-pointer-checks bug.c
device_flags:
	test	rdi, rdi
	mov	eax, -1
	cmovne	eax, DWORD PTR [rdi]
	ret
```

At `-O2` the function is just a load. The compiler reasons that if `dev` were NULL,
`dev->flags` would already have been undefined, so on every defined execution
`dev != NULL` and the `return -1` path is unreachable. `-fdelete-null-pointer-checks`
is enabled by default on most GCC targets at `-O2`; disabling it brings the test back.

## The Rule

Applying unary `*` (or `->`) to a null pointer is undefined (C11 6.5.3.2p4). The
compiler is not required to make the dereference trap, and it may assume that no
defined execution performs it. Everything the compiler can prove *after* that point
is based on `dev != NULL`.

This is how optimization normally works, not a compiler bug. The same deduction removes a
redundant check when a function already validated the pointer, and most code benefits
from it.

## Why Review And Testing Missed It

- The check is present in the source, so a reviewer looking for "is NULL handled?"
  sees it.
- Unit tests that pass NULL crash at the dereference. A crash in a negative test is
  often reported as "NULL is rejected".
- On hosted Linux, page 0 is unmapped, so the bug looks like an ordinary segfault.
  It becomes exploitable or silent only where address 0 is readable.

## Detection

| Tool | Result |
| --- | --- |
| `gcc -Wall -Wextra` | silent |
| `-fanalyzer` on the function alone (`-c -DNO_MAIN`) | silent |
| `-fanalyzer` with `main` passing NULL in the same file | `dereference of NULL 'dev' [CWE-476]` |
| `-fsanitize=undefined`, called with NULL | `runtime error: member access within null pointer of type 'const struct device'` |

The analyzer only reports when it can see a path on which the pointer is NULL. A
library function whose callers live in another translation unit gets no diagnostic.

## Fix

Validate before the first use:

```c
--8<-- "examples/c/failure-case-studies/01-null-check-after-dereference/fixed.c:11:17"
```

```text
$ ./fixed          device_flags = 5
$ ./fixed null     device_flags = -1
```

If NULL is a precondition violation rather than a valid input, say so in the API and
drop the check. A check that cannot be relied on is worse than no check, because it
implies a guarantee the binary does not provide.

## Keep It Fixed

- Put pointer validation in the first lines of a function, before any initializer
  that dereferences it. Some teams enforce "no dereferencing initializers before
  argument checks" in review or with a coding-standard checker.
- If a code base runs where address 0 is readable (bare-metal, some RTOS
  configurations, kernels), decide deliberately whether to build with
  `-fno-delete-null-pointer-checks`. The Linux kernel does, because of this exact bug
  class. Treat the flag as a mitigation, not as a fix for the source.
- Run negative tests with UBSan so that "it crashed" is reported as a failure, not
  as rejection.

## Same Root Cause Elsewhere

Any check after an operation whose validity depends on that check: bounds checks
after the indexed read, length checks after `memcpy`, "is this object still alive?"
after accessing a member. The compiler is allowed to run your reasoning in reverse.

## Precedent

CVE-2009-1897: the Linux `tun` driver read `tun->sk` before checking `tun` for NULL.
GCC removed the check, and because page 0 could be mapped by user space, this became
a local privilege escalation. The kernel has built with
`-fno-delete-null-pointer-checks` since then.

## Review Question

> For every validation in this function: is there any use of the validated value
> *above* it, including in declarations?

## Related Topics

- [Failure Case Studies overview](./index.md)
- [Undefined Behavior](../semantics-and-memory/undefined-behavior.md)
- [Pointer Fundamentals](../semantics-and-memory/pointer-fundamentals.md)
- [Compiler Optimization And Undefined Behavior](../advanced-c/compiler-optimization-and-undefined-behavior.md)

## References

- C11 (N1570) 6.5.3.2p4, unary `*` on an invalid pointer
- [GCC `-fdelete-null-pointer-checks`](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [CVE-2009-1897](https://nvd.nist.gov/vuln/detail/CVE-2009-1897)
