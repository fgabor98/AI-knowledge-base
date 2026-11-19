---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Undefined Behavior

Undefined behavior means that the C standard imposes no requirements when a program reaches a condition classified as undefined. The compiler may assume that such a condition never occurs and transform surrounding code accordingly. A result that appears stable in a debug build is not a contract.

Undefined behavior is different from a hardware fault, although it can lead to one. It is a source-level violation of the language rules that may be exploited by optimization.

## Learning Objectives

- Distinguish undefined, unspecified, implementation-defined, and constraint-violation behavior.
- Recognize common undefined operations in embedded code.
- Understand why optimizers can remove checks after undefined assumptions.
- Use warnings, sanitizers, static analysis, and tests appropriately.
- Avoid treating volatile, casts, or compiler flags as repairs for undefined source.

## Behavior Categories

| Category | Example | What to do |
| --- | --- | --- |
| Defined | Indexing an element inside its array | Rely on the specified rule |
| Implementation-defined | Whether plain char is signed | Read the implementation documentation |
| Unspecified | Which permitted option an implementation chooses | Do not depend on one result |
| Constraint violation | Invalid initializer or incompatible declaration | Fix the diagnostic |
| Undefined | Signed overflow or dereferencing a dangling pointer | Remove the condition completely |

A compiler may document an implementation-defined choice or extension. That documentation narrows behavior for that implementation; it does not create portable ISO C behavior.

## Common Undefined Operations

Examples include:

- signed integer overflow;
- division or remainder by zero;
- invalid shift counts or invalid signed shifts;
- out-of-bounds array access;
- dereferencing null, dangling, invalid, or misaligned pointers;
- reading an uninitialized or indeterminate value where not permitted;
- accessing an object through an incompatible lvalue type;
- violating a restrict contract;
- modifying a scalar more than once without required sequencing;
- use-after-free, double-free, and invalid free;
- data races on non-atomic objects;
- calling a function through an incompatible function-pointer type;
- overflowing allocation-size calculations and then indexing the undersized object.

A complete review asks which object, type, lifetime, bounds, and execution context make the operation valid.

## Signed Overflow

Signed overflow is not required to wrap:

~~~c
#include <limits.h>

int increment(int value)
{
    if (value == INT_MAX) {
        return INT_MAX;
    }
    return value + 1;
}
~~~

The guard makes the operation representable. If wraparound is the intended mathematical behavior, use an unsigned type or a checked operation whose policy is explicit.

Compiler flags such as fwrapv change implementation behavior for some signed arithmetic in a particular build; they should be a documented project decision, not a reason to leave accidental overflow in source.

## Bounds And Pointer Errors

~~~c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool get_byte(const uint8_t *data, size_t length,
              size_t index, uint8_t *result)
{
    if (data == NULL || result == NULL || index >= length) {
        return false;
    }

    *result = data[index];
    return true;
}
~~~

The check must happen before the access, and the length must describe the same live object as data. A pointer copied from an earlier state can be dangling even when the numeric address still looks plausible.

## Unsequenced Side Effects

Do not modify an object more than once in an expression when the sequencing rules do not establish a safe order:

~~~c
#include <stdint.h>

void safe_update(uint32_t *value)
{
    uint32_t old = *value;
    *value = old + 1u;
}
~~~

Split complex expressions into named intermediate steps. This makes both the standard rule and the intended state transition visible.

## Data Races

A volatile object can still participate in a data race. If one task writes a non-atomic object while another reads or writes it without the required synchronization, the C memory model does not provide a defined result.

Use C atomics, RTOS synchronization, interrupt masking, or a platform-specific protocol as appropriate. Choose based on the execution context and memory-ordering requirement, not just on whether the variable is declared volatile.

## Optimizer Consequences

The optimizer may use undefined-behavior assumptions to:

- remove a null check after a dereference;
- eliminate a branch that would require signed overflow;
- reorder or combine accesses under aliasing rules;
- infer that a loop terminates;
- vectorize accesses based on alignment or restrict;
- fold an impossible condition into a constant.

~~~c
#include <stddef.h>

int find_value(const int *data, size_t length, int wanted)
{
    for (size_t i = 0u; i < length; ++i) {
        if (data[i] == wanted) {
            return (int)i;
        }
    }
    return -1;
}
~~~

The caller must satisfy data and length. If data is null even when length is zero and the implementation evaluates an invalid expression through it, the API contract should define whether null empty ranges are permitted and the implementation should avoid forming invalid accesses.

Do not use observed assembly as evidence that undefined source is safe.

## Detection Tools

Use multiple layers:

- compiler warnings for syntax, constraints, conversion, and suspicious constructs;
- UndefinedBehaviorSanitizer for selected runtime checks;
- AddressSanitizer for many bounds and lifetime errors;
- MemorySanitizer where supported for uninitialized reads;
- ThreadSanitizer for host-representable data races;
- static analysis for paths and target-specific rules;
- fuzzing for parser and boundary coverage;
- target hardware faults, MPU diagnostics, and trace for non-host behavior.

Sanitizers are test instrumentation. They do not prove the absence of undefined behavior and may not support a bare-metal target directly.

## Recovering From Diagnostics

When a sanitizer or compiler reports undefined behavior:

1. Preserve the smallest reproducer and exact toolchain options.
2. Identify the violated rule, not only the failing instruction.
3. Find the object, lifetime, bound, conversion, or synchronization precondition.
4. Fix the interface or state transition that allowed the invalid operation.
5. Add a regression test at the boundary.
6. Check optimized target output after the source is defined.

Suppressions and no-sanitize attributes should be narrow, justified, and reviewed. They are not a substitute for an invariant.

## Exercises

1. Write tests around INT_MAX, invalid shifts, zero divisors, and empty buffers.
2. Run AddressSanitizer and UndefinedBehaviorSanitizer on a deliberately faulty parser.
3. Create a race on a host and fix it with an atomic or mutex; explain why volatile alone is insufficient.
4. Compare optimized and unoptimized assembly for defined code and for a deliberately invalid example.
5. Add checked arithmetic helpers for packet length and allocation size.
6. Build a project rule that treats selected undefined-behavior warnings as errors.

### Model Answers

Try each exercise before opening its answer. Where an exercise asks you to build
something, the answer shows one good solution, not the only one.

??? note "Exercise 1: boundary tests for undefined operations"
    ```c
    #include <assert.h>
    #include <limits.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    bool add_int(int a, int b, int *out)
    {
        if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) {
            return false;
        }
        *out = a + b;
        return true;
    }

    bool div_int(int a, int b, int *out)
    {
        if (b == 0 || (a == INT_MIN && b == -1)) {     /* both are undefined */
            return false;
        }
        *out = a / b;
        return true;
    }

    bool shl_u32(uint32_t value, unsigned int count, uint32_t *out)
    {
        if (count >= 32u) {
            return false;
        }
        *out = value << count;
        return true;
    }

    bool first_byte(const uint8_t *data, size_t length, uint8_t *out)
    {
        if (length == 0) {
            return false;
        }
        *out = data[0];
        return true;
    }

    int main(void)
    {
        int i;
        uint32_t u;
        uint8_t b;

        assert(add_int(INT_MAX - 1, 1, &i) && i == INT_MAX);
        assert(!add_int(INT_MAX, 1, &i) && !add_int(INT_MIN, -1, &i));
        assert(!div_int(1, 0, &i) && !div_int(INT_MIN, -1, &i));
        assert(div_int(INT_MIN, 1, &i) && i == INT_MIN);
        assert(shl_u32(1u, 31u, &u) && u == 0x80000000u);
        assert(!shl_u32(1u, 32u, &u));
        assert(!first_byte(NULL, 0, &b));
        return 0;
    }
    ```

    Each test sits on the boundary that makes the operation undefined, and on the last
    valid value next to it. The easily forgotten case is `INT_MIN / -1`: the mathematical
    result does not fit in `int`, so it is undefined like division by zero, and on x86 it
    traps. Run these tests under `-fsanitize=undefined`, so a guard that is off by one is
    reported instead of silently passing.

??? note "Exercise 2: sanitizers on a faulty parser"
    ```c
    #include <stddef.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    /* Record: 1 length byte, then that many value bytes, packed big-endian into an int. */
    int parse_value(const uint8_t *input, size_t available)
    {
        int value = 0;
        size_t length = input[0];               /* bug 1: no check that length fits */

        for (size_t i = 1; i <= length; i++) {
            value = value << 8 | input[i];      /* bug 2: shifts into the sign bit */
        }
        (void)available;
        return value;
    }

    int main(int argc, char **argv)
    {
        (void)argv;
        const uint8_t record[] = { 4, 0x80, 0x00, 0x00, 0x01 };
        size_t size = argc > 1 ? 3 : sizeof record;          /* a truncated record */
        uint8_t *input = malloc(size);

        memcpy(input, record, size);
        printf("%d\n", parse_value(input, size));
        free(input);
        return 0;
    }
    ```

    ```text
    $ gcc -std=c17 -g -O1 -fsanitize=address,undefined faulty.c -o faulty
    $ ./faulty
    faulty.c:14:23: runtime error: left shift of 8388608 by 8 places cannot be represented in type 'int'
    -2147483647
    $ ./faulty truncated
    ==696855==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000013 ...
    READ of size 1 at 0x502000000013 thread T0
        #0 ... in parse_value faulty.c:14
    0x502000000013 is located 0 bytes after 3-byte region [0x502000000010,0x502000000013)
    ```

    UBSan finds the signed shift on valid input. ASan finds the overread only when the
    input is actually truncated, so sanitizers are only as good as the inputs the tests or a
    fuzzer give them. The fix: check `length <= available - 1` and `length <= 4` before the
    loop, and build the value in `uint32_t`.

??? note "Exercise 3: why volatile does not fix a race"
    `volatile` makes every access to the object actually happen, in program order relative to
    other `volatile` accesses. It does not make an access atomic, and it does not order
    ordinary memory accesses. `counter++` on a `volatile` is still a separate load, add, and
    store, and another thread can store in between:

    ```c
    #include <pthread.h>
    #include <stdio.h>

    static volatile long counter;

    static void *work(void *arg)
    {
        (void)arg;
        for (int i = 0; i < 1000000; i++) {
            counter++;
        }
        return NULL;
    }

    int main(void)
    {
        pthread_t a;
        pthread_t b;

        pthread_create(&a, NULL, work, NULL);
        pthread_create(&b, NULL, work, NULL);
        pthread_join(a, NULL);
        pthread_join(b, NULL);
        printf("%ld\n", counter);
        return 0;
    }
    ```

    Three runs at `-O2` on an x86-64 machine printed 1252054, 1000000, and 1393969, and never
    the expected 2000000. In C11 terms, two unsynchronized accesses to the same non-atomic
    object, at least one of them a write, are a data race and undefined behavior (5.1.2.4p25).
    `volatile` does not change that. Fix it with `_Atomic long counter` (`counter++` is then an
    atomic read-modify-write), with a mutex, or by giving each thread its own counter and
    summing them at the end. For a flag that publishes data, see
    [case study 11](../failure-case-studies/11-spin-wait-data-race.md).

??? note "Exercise 4: optimized versus unoptimized assembly"
    ```c
    #include <stddef.h>

    int read_flag(const int *flag)
    {
        int value = *flag;          /* dereference first ... */

        if (flag == NULL) {         /* ... then check: too late */
            return -1;
        }
        return value;
    }

    int read_flag_fixed(const int *flag)
    {
        if (flag == NULL) {
            return -1;
        }
        return *flag;
    }
    ```

    GCC 13, x86-64, `read_flag`:

    ```text
    -O0                              -O2
        movq  -24(%rbp), %rax            movl  (%rdi), %eax
        movl  (%rax), %eax               ret
        movl  %eax, -4(%rbp)
        cmpq  $0, -24(%rbp)          # the null check is gone:
        jne   .L2                    # *flag already proved
        movl  $-1, %eax              # flag != NULL
        ...
    ```

    `read_flag_fixed` keeps its `testq %rdi, %rdi` at `-O2`. Defined code means the same
    thing at every level. In the invalid version, the dereference lets the compiler assume
    the pointer is not null, and `-O2` removes the check. A null argument crashes instead of
    returning -1. The reverse lesson also holds: `x + 1 > x` for a signed `x` is folded to
    `1` by GCC 13 even at `-O0`. A lower optimization level is not "the unoptimized
    behavior" of undefined code, because no such behavior is promised.

??? note "Exercise 5: checked arithmetic for lengths and sizes"
    ```c
    #include <assert.h>
    #include <stdbool.h>
    #include <stddef.h>
    #include <stdint.h>

    bool size_add(size_t a, size_t b, size_t *out)
    {
        if (a > SIZE_MAX - b) {
            return false;
        }
        *out = a + b;
        return true;
    }

    bool size_mul(size_t a, size_t b, size_t *out)
    {
        if (a != 0 && b > SIZE_MAX / a) {
            return false;
        }
        *out = a * b;
        return true;
    }

    /* Bytes for a packet with header_size + count records of record_size. */
    bool packet_bytes(size_t header_size, size_t count, size_t record_size, size_t *out)
    {
        size_t records;

        return size_mul(count, record_size, &records)
            && size_add(header_size, records, out);
    }

    int main(void)
    {
        size_t n;

        assert(packet_bytes(8, 4, 16, &n) && n == 72);
        assert(!packet_bytes(8, SIZE_MAX / 16 + 1, 16, &n));    /* multiplication wraps */
        assert(!packet_bytes(16, SIZE_MAX / 16, 16, &n));       /* the addition wraps */
        assert(size_mul(0, SIZE_MAX, &n) && n == 0);
        return 0;
    }
    ```

    Unsigned overflow is defined (it wraps), but the wrapped size is the bug: `malloc`
    returns a small buffer and the caller writes the full count into it. GCC and Clang also
    provide `__builtin_mul_overflow` and `__builtin_add_overflow`, and C23 standardizes them as
    `ckd_mul` and `ckd_add` in `<stdckdint.h>`. Hide whichever you use behind these function
    names.

??? note "Exercise 6: a project rule for undefined-behavior warnings"
    In the project's common compiler flags (a Makefile fragment; CMake's `add_compile_options`
    takes the same list):

    ```make
    UB_ERRORS := \
        -Werror=return-type \
        -Werror=implicit-function-declaration \
        -Werror=incompatible-pointer-types \
        -Werror=int-conversion \
        -Werror=sequence-point \
        -Werror=shift-count-overflow \
        -Werror=shift-count-negative \
        -Werror=shift-negative-value \
        -Werror=div-by-zero \
        -Werror=array-bounds \
        -Werror=stringop-overflow \
        -Werror=strict-aliasing \
        -Werror=uninitialized \
        -Werror=aggressive-loop-optimizations

    CFLAGS += -std=c17 -O2 -Wall -Wextra $(UB_ERRORS)
    ```

    All of these flags were accepted by GCC 13. The rule that makes it work:

    - The listed warnings are errors in every build, including developers' builds. Other
      warnings can stay warnings while they are being cleaned up.
    - Several of them (`array-bounds`, `stringop-overflow`, `uninitialized`,
      `strict-aliasing`) work only with optimization, so the CI build that enforces them
      uses `-O2`.
    - `-Wmaybe-uninitialized` is left as a warning, because it has false positives that
      change between GCC versions.
    - A suppression needs `#pragma GCC diagnostic` around the smallest region, with a comment
      giving the reason and a review sign-off. It is never a global `-Wno-`.
    - CI also runs the host tests under `-fsanitize=address,undefined
      -fno-sanitize-recover=all`, so runtime findings fail the build too.

## Common Mistakes

- Saying undefined behavior means “the hardware will wrap.”
- Treating one compiler’s result as a language guarantee.
- Assuming debug and optimized builds differ only in speed.
- Using volatile to repair a race.
- Using a cast, pragma, or no-sanitize attribute to hide a violation.
- Checking a pointer after dereferencing it.
- Assuming sanitizers cover hardware registers, DMA, or every concurrency path.
- Fixing the symptom while leaving the invalid ownership or bounds contract.

## Debugging Checklist

1. Capture compiler version, standard mode, optimization, and target.
2. Reduce the failure to one operation and one violated precondition.
3. Run sanitizers and static analysis on a host-equivalent reproducer.
4. Inspect warnings with conversion, bounds, alignment, and sequencing enabled.
5. Test both boundary values and error paths.
6. Trace lifetime and ownership across task, interrupt, and DMA boundaries.
7. Review optimizer assumptions such as strict aliasing, alignment, and finite loops.
8. Add a regression test and document any justified platform-specific exception.

## Related Topics

- [Semantics And Memory overview](./index.md)
- [Conversions, Promotions, And Aliasing](./conversions-promotions-and-aliasing.md)
- [Pointer Arithmetic And Bounds](./pointer-arithmetic-and-bounds.md)
- [Memory Safety And Lifetime](./memory-safety-and-lifetime.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [C Programming](../index.md)
- [Case study 01: NULL Check After Dereference](../failure-case-studies/01-null-check-after-dereference.md)
- [Case study 02: Signed Overflow Check](../failure-case-studies/02-signed-overflow-check.md)
- [Case study 03: Out-Of-Bounds Loop](../failure-case-studies/03-out-of-bounds-loop.md)
- [Case study 04: Shift Count Equal To Width](../failure-case-studies/04-shift-count-width.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [CERT C undefined-behavior rules](https://wiki.sei.cmu.edu/confluence/display/c)
