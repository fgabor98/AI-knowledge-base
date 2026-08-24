---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Preprocessor And Macros

The preprocessor transforms source text before the compiler applies the C type system. It is useful for include guards, conditional platform configuration, generated constants, and carefully designed compile-time interfaces. It is also capable of duplicating side effects, hiding control flow, and creating configurations that no one tests.

Prefer typed C constructs when they can express the requirement.

## Learning Objectives

- Explain preprocessing and macro expansion boundaries.
- Write safe object-like and function-like macros.
- Use stringification and token pasting deliberately.
- Build conditional compilation with explicit configuration validation.
- Understand variadic macro portability.
- Inspect macro expansion and generated configuration.
- Decide when an inline function, enum, const object, or build-system feature is better.

## Object-Like Macros

An object-like macro replaces a preprocessing token sequence:

~~~c
#define SENSOR_BUFFER_SIZE 128u
#define FEATURE_DMA 1
#define PROJECT_NAME "sensor"
~~~

Use macros for compile-time configuration, include guards, conditional symbols, and values that must participate in preprocessing. Use an enum or typed const object when the value should be visible to the compiler’s type system and debugger.

Parenthesize macro replacement expressions:

~~~c
#define TIMEOUT_MS (1000u)
#define BIT_MASK(bit) (UINT32_C(1) << (bit))
~~~

A macro has no type, scope in the C sense, or runtime storage. Its replacement can be affected by surrounding tokens and operator precedence.

## Function-Like Macro Hazards

A naive macro can change grouping:

~~~c
#define BAD_SQUARE(x) x * x
#define SQUARE(x) ((x) * (x))
~~~

BAD_SQUARE(a + b) expands into a + b * a + b. SQUARE groups the expression correctly, but it still evaluates x twice:

~~~c
#define SQUARE(x) ((x) * (x))
~~~

SQUARE(read_register()) can perform two hardware reads. Prefer a static inline function when the operation has a known type:

~~~c
static inline uint32_t square_u32(uint32_t value)
{
    return value * value;
}
~~~

A macro that expands to multiple statements should use a do-while wrapper:

~~~c
#define CLEAR_AND_RECORD(flag, counter) \
    do {                                 \
        (flag) = false;                 \
        ++(counter);                    \
    } while (0)
~~~

The caller must provide compatible modifiable objects, and arguments should not themselves contain side effects that are evaluated more than once.

## Stringification

The hash operator turns a macro argument into a string literal:

~~~c
#define STRINGIFY_LITERAL(value) #value

const char *board_name = STRINGIFY_LITERAL(BOARD_REV_B);
~~~

Macro arguments used with stringification are not expanded before stringification. A two-level helper expands first:

~~~c
#define STRINGIFY(value) STRINGIFY_IMPL(value)
#define STRINGIFY_IMPL(value) #value

#define BOARD_REV_B 2
const char *revision_text = STRINGIFY(BOARD_REV_B);
~~~

The result is source spelling, not a runtime conversion of an arbitrary value. Use it for diagnostics and generated labels, not for unvalidated user input.

## Token Pasting

The double-hash operator combines tokens:

~~~c
#define REGISTER_NAME(prefix, number) prefix##number

int REGISTER_NAME(channel_, 0);
~~~

Token pasting is useful for repetitive declarations, but it can hide generated names and make debugging difficult. A pasted token must form a valid preprocessing token after combination.

Use a two-level helper when an argument itself must expand before pasting:

~~~c
#define PASTE(left, right) PASTE_IMPL(left, right)
#define PASTE_IMPL(left, right) left##right
~~~

Keep generated identifiers predictable and document the naming convention.

## Conditional Compilation

Conditional compilation chooses source before C parsing:

~~~c
#if defined(BOARD_REV_B)
#define STATUS_LED_PIN 7u
#elif defined(BOARD_REV_A)
#define STATUS_LED_PIN 3u
#else
#error "No supported board revision selected"
#endif
~~~

Test every supported branch. An inactive branch can contain stale declarations or syntax errors that the normal build never sees.

Prefer feature decisions that are visible in a generated configuration header:

~~~c
#include "project_config.h"

#if PROJECT_FEATURE_DMA
void dma_start(void);
#else
void dma_start_polling(void);
#endif
~~~

Do not use the preprocessor to create large, structurally different programs when a small runtime strategy or function table would be clearer.

## Variadic Macros

C99 variadic macros accept a variable argument tail:

~~~c
#include <stdio.h>

#define LOG_ERROR(format, ...) \
    fprintf(stderr, "error: " format "\n", __VA_ARGS__)
~~~

This form requires at least one argument after format in strictly portable C99 usage. Some compilers support omitting the tail as an extension. C23 and newer compiler modes provide additional facilities; select and document the project dialect.

A safer design is often a typed logging function with a fixed context and a format attribute supplied by the compiler where available.

## Macro Hygiene

Good macro hygiene includes:

- use project-specific uppercase names;
- parenthesize parameters and replacement expressions;
- avoid evaluating an argument more than once;
- avoid names that collide with local variables;
- use a do-while wrapper for statement macros;
- avoid returning or jumping from a macro unexpectedly;
- document required types and side effects;
- keep macros short enough to inspect after expansion.

Reserved identifiers belong to the implementation. Do not create names beginning with double underscores or underscore-uppercase forms.

## Generated Configuration

Generated headers should be reproducible inputs to the build:

~~~c
#ifndef GENERATED_CONFIG_H
#define GENERATED_CONFIG_H

#define TARGET_CPU_CORTEX_M
#define TARGET_FLASH_BYTES 524288u
#define FEATURE_TRACE 1

#if FEATURE_TRACE && !defined(TARGET_CPU_CORTEX_M)
#error "Trace support requires a supported target"
#endif

#endif
~~~

Record the generator version, source configuration, board revision, compiler mode, and command-line definitions. Avoid having the build system and source headers silently disagree about a feature.

## Macro Debugging

Useful investigation commands include:

~~~sh
cc -std=c17 -E -dD source.c > source.i
cc -std=c17 -MMD -MP -c source.c -o source.o
~~~

The first shows expanded source and macro definitions. The second emits dependency information for build systems. Equivalent options exist in other toolchains.

When a macro behaves unexpectedly, reduce it to one argument, inspect expansion, and replace it temporarily with a typed function or named intermediate variable.

## Exercises

1. Replace a side-effect-prone macro with a static inline function.
2. Write a safe statement macro and test it inside an if-else without braces.
3. Build a stringification diagnostic that prints the selected board revision.
4. Generate two configuration headers and compile both feature branches.
5. Use token pasting to define register accessors, then evaluate whether the generated names remain maintainable.
6. Compare the preprocessed output of a host build and target build.

## Common Mistakes

- Omitting parentheses around macro parameters.
- Evaluating macro arguments multiple times.
- Defining macros with names that collide with identifiers or library names.
- Using a multi-statement macro without a do-while wrapper.
- Assuming inactive conditional branches are tested.
- Relying on compiler-specific variadic macro comma swallowing without documenting it.
- Hiding type requirements and side effects in a macro.
- Using macros where enums, const objects, inline functions, or build-system generation are clearer.
- Debugging the original source without inspecting expanded output.

## Debugging Checklist

1. Confirm all command-line defines and generated headers.
2. Generate preprocessed output and inspect the actual expansion.
3. Test each conditional branch in CI.
4. Check macro arguments for side effects and precedence.
5. Search for collisions with reserved or project identifiers.
6. Compile with warnings for macro redefinition and missing declarations.
7. Keep generated configuration artifacts with the build record.
8. Replace complex macros with typed code during diagnosis.

## Related Topics

- [Modular Design And APIs overview](./index.md)
- [Translation Units And Headers](./translation-units-and-headers.md)
- [APIs And Opaque Types](./api-and-opaque-types.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC preprocessor options](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html)
- [The GNU C Preprocessor manual](https://gcc.gnu.org/onlinedocs/cpp/)
