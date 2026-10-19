---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Semantics And Memory Test-Out

This section covers the parts of C where experienced programmers most often rely on
habit rather than the rules: scope and linkage, conversions, qualifiers, pointer validity,
object representation, lifetime, and undefined behavior. Use this page to find the pages
worth your time.

## How To Use It

- Answer before you open the answer, and without compiling.
- Name the rule and classify the behavior: portable, implementation-defined,
  unspecified, or undefined. A correct output with the wrong classification counts as
  wrong, because undefined behavior can print the "right" value today.
- Use [the score table](#score-and-reading-plan) at the end.

Outputs are for GCC 13 on x86-64 Linux (LP64, little-endian). The programs and their
expected outputs live in `examples/c/diagnostics/semantics-and-memory/`, and
`examples/c/diagnostics/check.py` verifies them as part of `scripts/validate-c.sh`.

## Q1. Shadowing And Linkage

```c
--8<-- "examples/c/diagnostics/semantics-and-memory/q01-block-scope-extern.c"
```

What does it print?

??? success "Answer"
    `1`, portable.

    The inner `int x = 2` has no linkage and hides the file-scope `x`. The block-scope
    `extern int x` then declares `x` again. Because the visible prior declaration has
    *no* linkage, the new one gets external linkage (C11 6.2.2p4), so it refers to the
    file-scope object, whose value is 1. GCC also warns that the local `x` is unused,
    which is true: nothing reads it.

    Missed it? Read [Storage Duration, Scope, And Linkage](./storage-duration-scope-and-linkage.md).

## Q2. Comparing Signed And Unsigned

```c
--8<-- "examples/c/diagnostics/semantics-and-memory/q02-mixed-signedness.c"
```

What does it print here? What would it print on 32-bit Arm Linux, or on 64-bit Windows?

??? success "Answer"
    `0 1` here; `0 0` on both of the others.

    - `i < u`: `int` and `unsigned int` have the same rank, so `i` converts to
      `unsigned int` (C11 6.3.1.8p1), and -1 becomes `UINT_MAX`. `UINT_MAX < 1` is false.
      `-Wextra` reports it (`comparison of integer expressions of different signedness`).
    - `i < (long)u`: now `int` meets `long`. On LP64 Linux, `long` is 64 bits and can
      represent every `unsigned int` value, so the comparison happens in `long` and
      -1 < 1 is true.
    - On ILP32 (32-bit Arm) and LLP64 (64-bit Windows), `long` is 32 bits. It cannot
      represent every `unsigned int`, so both operands convert to `unsigned long`, and
      the result is false again.

    A cast to a "wider" type fixes a signedness comparison only if it really is wider
    on every target. Compare in a type you control, or check the sign first.

    Missed it? Read [Conversions, Promotions, And Aliasing](./conversions-promotions-and-aliasing.md).

## Q3. Three Conversions

```c
--8<-- "examples/c/diagnostics/semantics-and-memory/q03-conversions.c"
```

What does it print? Classify each conversion.

??? success "Answer"
    `255 -56 -3`.

    - `(unsigned char)-1` is portable: conversion to an unsigned type is modulo 2^N
      (C11 6.3.1.3p2), giving 255.
    - `(signed char)200` is implementation-defined. The value does not fit, so the
      result is implementation-defined, or an implementation-defined signal is raised
      (6.3.1.3p3). GCC documents modulo wrapping, giving -56.
    - `(int)-3.99` is portable: floating-to-integer conversion truncates toward zero
      (6.3.1.4p1), giving -3. If the truncated value does not fit in the target type,
      for example `(int)1e10` or `(unsigned)-1.0`, the behavior is undefined.

    Missed it? Read [Conversions, Promotions, And Aliasing](./conversions-promotions-and-aliasing.md).

## Q4. Casting Away Const

```c
--8<-- "examples/c/diagnostics/semantics-and-memory/q04-const-cast.c"
```

What does it print?

??? success "Answer"
    Anything: the behavior is undefined. GCC 13 prints `20 20` at `-O0` and `10 20` at
    `-O2`.

    Modifying an object *defined* with a `const`-qualified type through a non-`const`
    lvalue is undefined (C11 6.7.3p6). At `-O2` the compiler treats `limit` as the
    constant 10 and never reloads it. A `static const` object in `.rodata` would
    instead fault when written. Casting away `const` is defined only when the object
    itself was not defined `const`, for example a `const char *` parameter that points
    to a writable buffer.

    Missed it? Read [Const, Volatile, And Restrict](./qualifiers-const-volatile-restrict.md).

## Q5. Pointer Validity

Given `int a[4];` and `int *end = a + 4;`, which of these are defined?

1. computing `a + 4`
2. evaluating `*end`
3. computing `a + 5`, without dereferencing it
4. evaluating `&a[4]`
5. evaluating `end - a`
6. `end > a + 1`
7. `&a[0] < &other[0]`, where `other` is a different array

??? success "Answer"
    Defined: **1, 4, 5, 6**.

    1. Defined: a pointer may point one past the last element (C11 6.5.6p8).
    2. Undefined: the one-past pointer must not be dereferenced (6.5.6p8).
    3. Undefined: *forming* a pointer outside `[a, a + 4]` is undefined even if it is
       never used. This is why `p + n > end` is not a valid bounds check (see
       [case study 02](../failure-case-studies/02-signed-overflow-check.md)). Compare
       lengths instead: `n > (size_t)(end - p)`.
    4. Defined: `&a[4]` means `&*(a + 4)`, and `&*` cancels without dereferencing
       (6.5.3.2p3).
    5. Defined: 4, of type `ptrdiff_t` (6.5.6p9).
    6. Defined: both pointers are into, or one past, the same array (6.5.8p5).
    7. Undefined: relational comparison of pointers to different objects (6.5.8p5).
       Equality (`==`) is defined. For an ordering across objects, compare
       `(uintptr_t)` values, which is implementation-defined but meaningful on flat
       address spaces.

    Missed it? Read [Pointer Arithmetic And Bounds](./pointer-arithmetic-and-bounds.md).

## Q6. Sizing An Allocation

```c
--8<-- "examples/c/diagnostics/semantics-and-memory/q05-allocation-size.c"
```

What does it print?

??? success "Answer"
    `4 non-null null`.

    `count * 4` is computed in `size_t` and wraps modulo 2^64 to 4 (the multiplication
    is defined because `size_t` is unsigned). `malloc` receives 4 and succeeds, and any
    code that then writes `count` elements overflows the heap. `calloc` receives the two
    factors separately, so it can detect that the product does not fit. glibc's `calloc`
    returns NULL. At `-O2` GCC warns about the `calloc` argument
    (`exceeds maximum object size`) because it can see the constant.

    Use `calloc`, or check `count > SIZE_MAX / size` before multiplying. In C23,
    `ckd_mul` does the check for you.

    Missed it? Read [Memory Layout And Allocation](./memory-layout-and-allocation.md).

## Q7. Looking At The Bytes

```c
--8<-- "examples/c/diagnostics/semantics-and-memory/q06-byte-view.c"
```

What does it print? Is the cast allowed?

??? success "Answer"
    `44 11`. The cast and the accesses are defined, and the byte order is
    implementation-defined.

    Any object may be read through an lvalue of character type (C11 6.5p7, last item),
    so `unsigned char *` is the one pointer type that may inspect another object's
    representation without violating aliasing. The order of the bytes is
    implementation-defined: x86-64 is little-endian, so the least significant byte
    `0x44` comes first. Any other pointer type here would be an aliasing violation (see
    [case study 08](../failure-case-studies/08-strict-aliasing-type-pun.md)).

    Missed it? Read [Object Representation, Alignment, And Padding](./object-representation-alignment-and-padding.md).

## Q8. Returned Pointers

```c
--8<-- "examples/c/diagnostics/semantics-and-memory/q07-lifetimes.c"
```

Which functions return a pointer the caller may use? Which does GCC warn about?

??? success "Answer"
    `name_a` and `defaults_d` are fine. `name_b` and `defaults_c` return dangling
    pointers. GCC 13 warns only about `name_b`.

    - `name_a`: a string literal has static storage duration (C11 6.4.5p6).
    - `name_b`: `name` is an automatic array initialized from the literal. It ceases to
      exist when the function returns. GCC: `function returns address of local variable`.
    - `defaults_c`: a compound literal at block scope has *automatic* storage
      duration, tied to the enclosing block (6.5.2.5p5). It is just as dangling as
      `name_b`, but GCC does not warn.
    - `defaults_d`: a `static` array lives for the whole program.

    A missing warning does not mean the code is valid. Lifetime bugs like `defaults_c`
    are found by ASan or by review, not by `-Wall`. With GCC 13, an ASan build that
    reads through the pointer from `defaults_c` reports `stack-use-after-return` without
    extra options.

    Missed it? Read [Memory Safety And Lifetime](./memory-safety-and-lifetime.md).

## Q9. Adding Const Two Levels Down

```c
--8<-- "examples/c/diagnostics/semantics-and-memory/q08-nested-const.c"
```

Why does the first initialization get a diagnostic, while adding `const` at one level
(`char *` to `const char *`) never does?

??? success "Answer"
    Because allowing it would let the program modify a `const` object without a cast.
    The C standard's own example (C11 6.5.16.1, EXAMPLE 3):

    <!-- c-check: skip -- the standard's example: bare statements with a deliberate constraint violation -->

    ```c
    const char **cpp;
    char *p;
    const char c = 'A';

    cpp = &p;     /* constraint violation */
    *cpp = &c;    /* p now points to c */
    *p = 0;       /* modifies a const object */
    ```

    The implicit conversion may add qualifiers only to the type pointed to directly
    (6.5.16.1p1). `char **` to `const char *const *` is safe, because with the middle
    `const` nothing can be stored through it, but C still requires a cast there. C++
    allows that one implicitly. GCC 13 reports the first line as a warning
    (`initialization of 'const char **' from incompatible pointer type 'char **'`).
    GCC 14 makes `-Wincompatible-pointer-types` an error by default.

    Missed it? Read [Pointer Fundamentals](./pointer-fundamentals.md) and
    [Const, Volatile, And Restrict](./qualifiers-const-volatile-restrict.md).

## Q10. Classify The Behavior

Assume 32-bit `int`. For each expression, is the behavior defined, implementation-defined,
or undefined?

1. `INT_MIN / -1`
2. `-1 >> 1`
3. `1 << 31`
4. `(unsigned int)INT_MAX + 1`
5. `i = i++ + 1;`
6. `printf("%d\n", sizeof(int));`
7. `int x; return x;`, where `x` is automatic and its address is never taken

??? success "Answer"
    1. **Undefined.** The quotient 2^31 is not representable (C11 6.5.5p6). On x86-64
       the `idiv` instruction traps, and the process dies with `SIGFPE` ("Floating point
       exception", although no floating point is involved). It is a real crash, not
       only a theoretical one.
    2. **Implementation-defined** (6.5.7p5). GCC defines it as an arithmetic shift,
       giving -1.
    3. **Undefined.** Shifting a 1 into the sign bit of a signed type is undefined
       (6.5.7p4). Write `1u << 31`.
    4. **Defined.** The addition is unsigned, and the result is 2^31.
    5. **Undefined.** `i` is modified twice without sequencing (6.5p2).
    6. **Undefined.** `sizeof` yields `size_t`, which does not match `%d` (7.21.6.1p9).
       Use `%zu`. GCC's `-Wformat` (in `-Wall`) reports it.
    7. **Undefined.** The value is indeterminate, and an automatic object that could
       have been declared `register` (its address is never taken) makes the read
       undefined (6.3.2.1p2).

    Missed it? Read [Undefined Behavior](./undefined-behavior.md) and the
    [failure case studies](../failure-case-studies/index.md).

## Score And Reading Plan

| Question | Page it tests |
| ---: | --- |
| Q1 | [Storage Duration, Scope, And Linkage](./storage-duration-scope-and-linkage.md) |
| Q2, Q3 | [Conversions, Promotions, And Aliasing](./conversions-promotions-and-aliasing.md) |
| Q4 | [Const, Volatile, And Restrict](./qualifiers-const-volatile-restrict.md) |
| Q5 | [Pointer Arithmetic And Bounds](./pointer-arithmetic-and-bounds.md) |
| Q6 | [Memory Layout And Allocation](./memory-layout-and-allocation.md) |
| Q7 | [Object Representation, Alignment, And Padding](./object-representation-alignment-and-padding.md) |
| Q8 | [Memory Safety And Lifetime](./memory-safety-and-lifetime.md) |
| Q9 | [Pointer Fundamentals](./pointer-fundamentals.md), [Const, Volatile, And Restrict](./qualifiers-const-volatile-restrict.md) |
| Q10 | [Undefined Behavior](./undefined-behavior.md) |

- **All correct, with the right classification:** read only the *Common Mistakes* lists
  in this section, then go to the [failure case studies](../failure-case-studies/index.md).
  They test the same rules against real optimizer behavior.
- **Q4, Q5, or Q10 wrong:** read [Undefined Behavior](./undefined-behavior.md) first.
  Gaps there change how you read every other page.
- **Otherwise:** read the pages the missed questions point to.
