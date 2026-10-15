---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Language Fundamentals Test-Out

Experienced C programmers can use this page to decide which Language Fundamentals pages
to read and which to skip. Each question targets one page and tests whether you know
the *rule*, not just what one compiler prints.

## How To Use It

- Answer each question before opening its answer. Do not compile anything yet.
- For a "what does it print" question, also name the rule behind the output, and say
  whether the result is portable, implementation-defined, unspecified, or undefined.
- Count a question as correct only if both the result and the reason are right.
- Use [the score table](#score-and-reading-plan) at the end to decide what to read.

Outputs are for GCC 13 on x86-64 Linux (LP64, 32-bit `int`, little-endian). Every program
and its expected output live in `examples/c/diagnostics/language-fundamentals/`, and
`examples/c/diagnostics/check.py` verifies the whole answer key against the compiler.
It runs as part of `scripts/validate-c.sh`.

## Q1. Literals

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q01-literals.c"
```

What does it print?

??? success "Answer"
    `34 3`, portable.

    - `010` is octal (8) and `0x10` is hexadecimal (16), so the sum is 8 + 16 + 10 = 34.
      A leading zero makes an integer constant octal, which matters for values such as
      `0644`, or a zero-padded table where `09` does not compile at all.
    - `"\x41" "1"` is two literals. Escape sequences are converted in translation phase
      5, *before* adjacent string literals are joined in phase 6 (C11 5.1.1.2). The
      result is `"A1"`, plus the terminator: 3 bytes. Written as one literal, `"\x411"`
      would be a single hexadecimal escape with the value 0x411. That does not fit in a
      `char`, which is a constraint violation (6.4.4.4p9). GCC only warns
      (`hex escape sequence out of range`) and compiles it anyway. Splitting the literal
      is the standard way to end a hex escape.

    Missed it? Read [Source Code And Syntax](./source-code-and-syntax.md).

## Q2. What ISO C Guarantees

Which of these does ISO C guarantee for every conforming implementation?

1. `sizeof(char) == 1`
2. `CHAR_BIT == 8`
3. `sizeof(int) >= 2`
4. `uint32_t` exists
5. plain `char` is signed

??? success "Answer"
    Only **1**.

    1. Guaranteed: `sizeof` measures in units of `char` (C11 6.5.3.4p4).
    2. Not guaranteed. `CHAR_BIT` is *at least* 8 (5.2.4.2.1). DSPs with 16- or 32-bit
       `char` exist; TI's C28x family is a common one.
    3. Not guaranteed. `int` must hold at least -32767 to 32767, so it has at least 16
       bits. With a 16-bit `char` that is one byte, so `sizeof(int)` is 1 on C28x.
    4. Not guaranteed. The exact-width types are optional, and an implementation must
       provide `uint32_t` only if it has a suitable type (7.20.1.1p3).
       `uint_least32_t` always exists.
    5. Implementation-defined (6.2.5p15). It is signed on x86-64 Linux and unsigned on
       Arm Linux. Code that stores bytes in plain `char` and compares them with `0x80`
       behaves differently on the two.

    Missed it? Read [Types, Values, And Objects](./types-values-and-objects.md).

## Q3. Narrow Arithmetic

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q02-narrow-arithmetic.c"
```

What does it print?

??? success "Answer"
    `1 0` on this target. The first value depends on the width of `int`.

    - `a + 1`: `a` is promoted to `int` before the addition (C11 6.3.1.1p2), so the result
      is 65536, and 65536 > 65535. No wraparound happens.
    - `b + 1`: `uint32_t` is `unsigned int` here, and unsigned arithmetic wraps modulo
      2^32 (6.2.5p9), so the result is 0, which is not greater than `b`.
    - On a target with 16-bit `int`, `uint16_t` promotes to `unsigned int`, and the first
      comparison also prints `0`. The same source behaves differently because the
      promotion depends on the target.

    Missed it? Read [Expressions And Operators](./expressions-and-operators.md) and
    [case study 05](../failure-case-studies/05-unsigned-short-promotion.md).

## Q4. A Status Check

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q03-precedence.c"
```

What does it print?

??? success "Answer"
    `0`.

    `==` binds tighter than `&`, so the expression is
    `flags & (READY_MASK == READY_MASK)`, which is `0x6 & 1`, which is 0. The intended
    test is `(flags & READY_MASK) == READY_MASK`. GCC's `-Wall` reports it:
    `suggest parentheses around comparison in operand of '&'`. The bitwise operators
    rank below the comparisons for historical reasons, so always parenthesize a
    mask test.

    Missed it? Read [Expressions And Operators](./expressions-and-operators.md).

## Q5. Nested Conditions

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q04-dangling-else.c"
```

What does it print?

??? success "Answer"
    Only `end`.

    An `else` belongs to the nearest unmatched `if` (C11 6.8.4.1p3), whatever the
    indentation. Here that is `if (fault)`, which is never reached because `armed` is 0.
    GCC warns: `suggest explicit braces to avoid ambiguous 'else'`. Use braces on every
    branch and the question never comes up.

    Missed it? Read [Control Flow](./control-flow.md).

## Q6. Two Calls In One Statement

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q05-argument-order.c"
```

What can it print? Is the program's behavior defined?

??? success "Answer"
    It can print `1 2` or `2 1`. GCC 13 on x86-64 prints `2 1`. The behavior is
    *unspecified*, not undefined.

    The order in which function arguments are evaluated is unspecified (C11 6.5p3,
    6.5.2.2p10). The two calls to `next()` are *indeterminately sequenced*: each call
    runs completely before or after the other, so `counter` is never modified by two
    unsequenced operations at once. Compare `printf("%d %d", i++, i++)`, where two
    unsequenced modifications of `i` make the behavior undefined.

    The fix is the same for both: if the order matters, make each call a separate
    statement.

    Missed it? Read [Functions](./functions.md).

## Q7. Sizes Of Strings

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q06-sizeof-arrays.c"
```

What does it print?

??? success "Answer"
    `7 6 8`.

    - `sizeof name` is 7: an array initialized from a string literal includes the
      terminator.
    - `strlen(name)` is 6: the characters before the terminator.
    - `parameter_size` returns 8: an array parameter is adjusted to a pointer (C11
      6.7.6.3p7), so the `16` in `char buffer[16]` does not change `sizeof`. GCC warns
      `'sizeof' on array function parameter 'buffer' will return size of 'char *'`.

    GCC also warns that the call passes a 7-byte array to a parameter declared with 16
    bytes. It reads the declared size as a statement of the caller's obligation, and
    that is a good reason to write meaningful sizes in array parameters, or to use
    `char buffer[static 16]`, which makes the minimum size a requirement on the caller.

    Missed it? Read [Arrays, Strings, And Buffers](./arrays-strings-and-buffers.md).

## Q8. An Exactly Sized String

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q07-exact-size-string.c"
```

Does it compile as C? What is in `id`?

??? success "Answer"
    It compiles without a warning. `id` holds `'A' 'B' 'C' 'D'` and **no terminator**.

    When the array has a known size, the terminator of the initializing literal is
    stored only "if there is room" (C11 6.7.9p14). That is useful for fixed-width fields
    in protocols and file formats. Passing `id` to `strlen` or `printf("%s")` reads past
    the array. C++ rejects the declaration (`initializer-string for 'char [4]' is too
    long`), so headers shared with C++ cannot use it.

    Missed it? Read [Initialization](./initialization.md) and
    [Arrays, Strings, And Buffers](./arrays-strings-and-buffers.md).

## Q9. Enumerators And Layout

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q08-enums-and-layout.c"
```

What does it print?

??? success "Answer"
    `6 5 12 8` on this target. The enumerator values are portable; the sizes are not.

    - An enumerator without `=` is one more than the previous one (C11 6.7.2.2p3):
      `MODE_C` is 6. Two enumerators may have the same value: `MODE_D` is 5.
    - `struct first` has `char`, 3 bytes of padding, `int`, `char`, and 3 bytes of
      tail padding: 12 bytes. Putting the `int` first leaves only 2 bytes of tail
      padding: 8. Padding is implementation-defined, but ordering members from largest
      to smallest alignment is a reliable way to reduce it.

    Missed it? Read [Structures, Unions, And Enumerations](./structures-unions-and-enums.md).

## Q10. Reading A Declaration

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q09-declarator.c"
```

Describe `install` in words.

??? success "Answer"
    `install` is a function that takes an `int` and a pointer to a function taking
    `int` and returning `void`, and that returns a pointer to a function taking `int`
    and returning `void`. It has the same shape as the standard `signal` function, and
    the file compiles cleanly.

    Read from the identifier outwards: `install(...)` is a function call. `*install(...)`
    means the result is a pointer. `(*install(...))(int)` means it points to a function
    taking `int`. The leading `void` is that function's return type. A typedef makes
    the intent visible:

    ```c
    typedef void signal_handler(int);
    signal_handler *install(int signal_number, signal_handler *handler);
    ```

    Missed it? Read [Declarations And Declarators](./declarations-and-declarators.md).

## Q11. Const And Pointers

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q10-const-pointers.c"
```

Which line does the compiler reject?

??? success "Answer"
    Line C: `error: assignment of read-only variable 'fixed'`.

    - `const int *read_only` is a pointer to `const int`. The pointer itself can change
      (line A is fine), but `*read_only = ...` would be rejected.
    - `int *const fixed` is a `const` pointer to `int`. The pointed-to object can change
      (line B is fine), but the pointer cannot.

    Read the declaration from right to left: "`fixed` is a const pointer to int",
    "`read_only` is a pointer to int that is const".

    Missed it? Read [Declarations And Declarators](./declarations-and-declarators.md).

## Q12. Partial Initializers

```c
--8<-- "examples/c/diagnostics/language-fundamentals/q11-brace-elision.c"
```

What does it print?

??? success "Answer"
    `2 0 0 | 0 9 0 0 7 8`, portable.

    - `{ 1, 2 }` sets `mode` to 1. Because the inner braces are omitted, the `2` goes
      to the next scalar, `thresholds[0]` (brace elision, C11 6.7.9p20). Everything not
      initialized explicitly becomes zero (6.7.9p21), including `retries`. GCC's
      `-Wall` warns `missing braces around initializer`, which is a hint that `2` may
      not have gone where the author intended.
    - `[4] = 7, 8` sets index 4, and the next initializer continues at index 5
      (6.7.9p17). `[1] = 9` then jumps back to index 1. Unnamed elements are zero.

    Missed it? Read [Initialization](./initialization.md).

## Score And Reading Plan

| Question | Page it tests |
| ---: | --- |
| Q1 | [Source Code And Syntax](./source-code-and-syntax.md) |
| Q2 | [Types, Values, And Objects](./types-values-and-objects.md) |
| Q3, Q4 | [Expressions And Operators](./expressions-and-operators.md) |
| Q5 | [Control Flow](./control-flow.md) |
| Q6 | [Functions](./functions.md) |
| Q7 | [Arrays, Strings, And Buffers](./arrays-strings-and-buffers.md) |
| Q8 | [Initialization](./initialization.md), [Arrays, Strings, And Buffers](./arrays-strings-and-buffers.md) |
| Q9 | [Structures, Unions, And Enumerations](./structures-unions-and-enums.md) |
| Q10, Q11 | [Declarations And Declarators](./declarations-and-declarators.md) |
| Q12 | [Initialization](./initialization.md) |

- **All 12 correct, with reasons:** skip the section's pages and read only their
  *Common Mistakes* lists. Continue with the
  [Semantics And Memory test-out](../semantics-and-memory/test-out.md).
- **One or two wrong:** read the pages those questions point to.
- **More than two wrong:** work through the section in order; it is short.

Being right for the wrong reason counts as wrong. "GCC prints `2 1`" is not an answer to
Q6. "The order is unspecified, so the code must not depend on it" is.
