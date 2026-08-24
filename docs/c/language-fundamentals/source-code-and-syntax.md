---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Source Code And Syntax

C source is text organized into preprocessing directives, declarations, definitions, statements, and expressions. The compiler does not see the file exactly as it appears in an editor: comments are removed, macros are expanded, included headers are inserted, and conditional sections may disappear before the C grammar is parsed.

## Learning Objectives

- Explain what a translation unit is.
- Distinguish preprocessing tokens from C tokens.
- Recognize identifiers, keywords, literals, punctuators, declarations, statements, and expressions.
- Separate a declaration from a definition.
- Use preprocessor output to investigate an unexpected build result.
- Identify source assumptions that are not portable across encodings or toolchains.

## Translation Units And Phases

A source file becomes a translation unit after headers are included and conditional preprocessing is resolved. A translation unit can contain one .c file plus many headers. Each is compiled separately; the linker later combines the object files.

A useful simplified pipeline is:

1. Source characters are mapped into the compiler’s source character set.
2. Lines are processed, including backslash-newline splicing.
3. Comments are replaced with whitespace.
4. Include, define, and if directives are processed.
5. The resulting preprocessing tokens are parsed as C.
6. The compiler checks and lowers the translation unit to an object file.
7. The linker resolves references between objects and libraries.

To inspect post-preprocessor input with GCC-like tools:

~~~sh
cc -std=c17 -E -dD source.c > source.i
~~~

The E option stops after preprocessing. Equivalent options exist in other toolchains.

## Tokens And Names

Important categories include:

- keywords such as if, return, struct, and static;
- identifiers such as sample_count and read_sensor;
- integer, floating, character, and string literals;
- operators and punctuators such as +, ->, braces, and semicolons;
- preprocessing tokens, which also include header names and macro forms.

Identifiers are case-sensitive. count, Count, and COUNT are different. Use a naming convention that makes scope and ownership visible, such as driver_init, DRIVER_BUFFER_SIZE, and struct driver_context.

C has several identifier namespaces. A tag such as struct packet, a member name such as packet.length, and an ordinary variable such as packet do not all compete in the same namespace. This is legal, but reusing a spelling in several roles can harm readability.

Do not use implementation-reserved names. Names beginning with an underscore followed by an uppercase letter, or with two underscores, are reserved broadly; names beginning with an underscore at file scope are also commonly reserved.

## Source Character Set And Comments

C source is not necessarily UTF-8. The compiler has a source character set and an execution character set, and options or locale settings can affect mapping.

For portable embedded source:

- keep identifiers and operators in the basic source character set;
- define the encoding policy for comments and string data;
- do not assume a character literal’s number is an ASCII code unless the target contract says so.

Comments are replaced by whitespace before normal parsing:

~~~c
/* A block comment can span lines. */
int sample_count;  // A line comment ends at the newline.
~~~

A block comment cannot be nested. Comments can hide tokens accidentally:

~~~c
/* int disabled = 1; */
~~~

When macros or conditional compilation are involved, inspect preprocessed output rather than only the original file.

## Declarations, Definitions, Statements, And Expressions

- A declaration introduces an identifier and describes its type or linkage.
- A definition is a declaration that also creates the function body or object storage required for a definition.
- An expression computes a value, produces a side effect, or both.
- A statement controls execution: expression, compound, selection, iteration, and jump statements are common forms.
- A translation unit is a sequence of external declarations.

~~~c
#include <stdint.h>

#define SAMPLE_LIMIT 16u

static uint16_t samples[SAMPLE_LIMIT];

static uint16_t sample_sum(const uint16_t *values, uint32_t count);

static uint16_t sample_sum(const uint16_t *values, uint32_t count)
{
    uint16_t total = 0u;
    for (uint32_t i = 0u; i < count; ++i) {
        total = (uint16_t)(total + values[i]);
    }
    return total;
}
~~~

The declaration before the definition is a prototype. Put such declarations in a header when the function is part of a module interface.

## Literals

Literal syntax is part of the language, but type and value can surprise:

~~~c
int decimal = 42;
unsigned octal = 052;
unsigned hexadecimal = 0x2Au;
unsigned binary = 0b101010u;
double ratio = 0.5;
char letter = 'A';
const char *text = "ready";
~~~

Integer constants can be decimal, octal, or hexadecimal; suffix and value determine their type. A leading zero changes the base, so 010 is eight. Floating constants are double by default unless suffixed with f or L.

'A' is an integer character constant. "A" is an array containing 'A' and a terminating null character. Their encoding is not an ISO C promise of ASCII.

Use macros such as UINT32_C(42) when a constant must have a particular stdint.h type family. Binary constants may depend on the selected language edition or compiler mode, so verify the project standard.

## Preprocessor Boundaries

The preprocessor is textual, not type-aware. It is useful for configuration and include guards, but macro substitution can change meaning:

~~~c
#define SQUARE(x) ((x) * (x))

int value = SQUARE(3 + 1);
~~~

Even this macro evaluates its argument twice. SQUARE(read_register()) may read hardware twice. Prefer an inline function when type checking and single evaluation matter.

Test every supported conditional-compilation configuration:

~~~c
#if defined(BOARD_REV_B)
#define STATUS_LED_PIN 7u
#else
#define STATUS_LED_PIN 3u
#endif
~~~

An unused branch can contain stale declarations or syntax errors that no normal build sees.

## Exercises

1. Write a file with an object definition, prototype, function definition, and call. Identify each role.
2. Run the preprocessor on a file using an include guard and an if block; find the final declaration visible to the compiler.
3. Explain the values of 010, 10, and 0x10.
4. Create a macro that evaluates an argument twice, then replace it with an inline function.
5. Introduce a missing brace and a missing semicolon. Record where each diagnostic points.

## Common Mistakes

- Assuming the compiler sees only the .c file rather than the complete preprocessed translation unit.
- Treating macros as typed functions.
- Using a leading zero on a decimal-looking integer.
- Modifying a string literal through char pointer.
- Relying on ASCII numeric values without a target contract.
- Defining objects in headers without understanding linkage.
- Testing only one conditional-compilation configuration.
- Using reserved identifiers.

## Debugging Checklist

1. Confirm the selected standard and compiler command line.
2. Generate preprocessed output and search for the declaration or macro.
3. Check whether an if branch removed expected code.
4. Inspect the preceding line for an unclosed comment, string, parenthesis, or brace.
5. Read the first warning or error; later diagnostics may be cascades.
6. Reduce the source to the smallest reproducer.
7. Compare host and target preprocessing only after checking defines and include paths.

## Related Topics

- [Language Fundamentals overview](./index.md)
- [Declarations And Declarators](./declarations-and-declarators.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Preprocessor And Macros](../modular-design-and-apis/preprocessor-and-macros.md)
- [C Programming](../index.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [GCC language extensions](https://gcc.gnu.org/onlinedocs/gcc/Extensions.html)
- [Clang language extensions](https://clang.llvm.org/docs/LanguageExtensions.html)
