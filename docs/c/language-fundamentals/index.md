---
status: draft
reviewed: false
domain: c
difficulty: beginner
last_reviewed: null
---

# Language Fundamentals

This chapter builds the working language model needed to read, write, review, and debug ordinary C. It starts with source text and declarations, then moves through types, expressions, control flow, functions, aggregates, and initialization.

The examples use mostly C17-compatible syntax because that is still a practical baseline for many embedded toolchains. C23 is the current ISO C edition, but a production project should choose its language version deliberately and make the choice visible in the build.

## What This Chapter Teaches

C becomes predictable when five questions are answered for every piece of code:

1. What tokens did the compiler receive after preprocessing?
2. What does each identifier declare, and what is its type?
3. What values and side effects can each expression produce?
4. Which object owns the storage, and how long does it remain valid?
5. Which behavior comes from ISO C, the implementation, the ABI, the operating system, or the hardware?

The pages in this chapter establish the first four questions. Later chapters go deeper into undefined behavior, object representation, pointers, memory, concurrency, and platform interfaces.

## Recommended Progression

1. [Source Code And Syntax](./source-code-and-syntax.md) — translation units, tokens, and grammar.
2. [Types, Values, And Objects](./types-values-and-objects.md) — representations, ranges, and object identity.
3. [Declarations And Declarators](./declarations-and-declarators.md) — declarations that combine those types.
4. [Expressions And Operators](./expressions-and-operators.md) — values, conversions, grouping, and side effects.
5. [Control Flow](./control-flow.md) — branches, loops, cleanup paths, and state machines.
6. [Functions](./functions.md) — interfaces, contracts, callbacks, and call boundaries.
7. [Arrays, Strings, And Buffers](./arrays-strings-and-buffers.md) — contiguous storage and bounded data.
8. [Structures, Unions, And Enumerations](./structures-unions-and-enums.md) — records, variants, and states.
9. [Initialization](./initialization.md) — object state from startup through runtime.

## Language Boundaries

Always label the kind of claim being made:

| Claim | Example | Where to verify it |
| --- | --- | --- |
| ISO C | An object with static storage duration is initialized before program startup | The selected ISO C edition |
| Implementation | char is signed on this target | Compiler documentation, predefined macros, or a test |
| ABI | A structure has a particular calling convention or padding | Target ABI and compiler output |
| Operating system | read() blocks or returns EINTR | POSIX or OS documentation |
| Hardware | A register write requires a read-modify-write sequence | The chip reference manual |
| Project policy | Public APIs never allocate dynamically | Project guidelines and code review |

Embedded failures often happen when a lower-level assumption is silently presented as if it were a language guarantee.

## Learning Rules

For every example:

- Compile it with the project’s selected standard and strict warnings.
- Read the diagnostics instead of suppressing them automatically.
- Run host tests where possible, then test on the actual target.
- Check zero, one, maximum, empty-buffer, and error paths.
- Ask whether the code can run from an interrupt, multiple threads, a boot stage, or a recovery path.
- Record assumptions about width, alignment, byte order, timing, and ownership.

A useful hosted starting command is:

~~~sh
cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -g -O0 example.c -o example
~~~

Treat this as a starting point; target builds need their own compiler and linker options.

## Coverage Map

| Page | Core question | Embedded payoff |
| --- | --- | --- |
| [Source Code And Syntax](./source-code-and-syntax.md) | How does source become a translation unit? | Understand generated headers, conditional compilation, and diagnostics |
| [Types, Values, And Objects](./types-values-and-objects.md) | What values can an object represent? | Choose widths for registers, protocols, counters, and storage |
| [Declarations And Declarators](./declarations-and-declarators.md) | What exactly does a declaration declare? | Read vendor headers and pointer-heavy APIs |
| [Expressions And Operators](./expressions-and-operators.md) | What is grouped, converted, evaluated, and changed? | Write safe masks, time arithmetic, and state updates |
| [Control Flow](./control-flow.md) | Which path executes, and does it terminate? | Build polling loops, timeouts, recovery paths, and state machines |
| [Functions](./functions.md) | Where is an interface and what does it promise? | Keep drivers testable and document timing |
| [Arrays, Strings, And Buffers](./arrays-strings-and-buffers.md) | How is contiguous storage bounded? | Protect packets, DMA buffers, logs, and command input |
| [Structures, Unions, And Enumerations](./structures-unions-and-enums.md) | How are records, variants, and states represented? | Model configuration without accidental layout assumptions |
| [Initialization](./initialization.md) | What state exists before first use? | Understand data, bss, startup, and reset behavior |

## Practice Sequence

Use one small sensor-packet project while studying:

1. Define a bounded byte buffer and a status enum.
2. Add a parser that validates it.
3. Represent the parsed result with a structure.
4. Add idle, sampling, and fault states.
5. Initialize all state explicitly and test empty, truncated, and maximum-size input.
6. Compile host tests and the target build.
7. Inspect the object and map file only after the language behavior is clear.

## Outcomes

After this chapter you should be able to:

- read a nontrivial C declaration without guessing;
- distinguish an array from a pointer and a string from a byte buffer;
- select integer types and format specifiers deliberately;
- explain precedence and evaluation order in a concrete expression;
- write explicit timeout, cleanup, and fall-through behavior;
- design function interfaces with ownership and error contracts;
- use structures and enums without treating layout as a wire format accidentally;
- distinguish initialization from assignment;
- recognize implementation- and target-dependent results.

## Related Topics

- [Orientation](../orientation/index.md)
- [Semantics And Memory Model](../semantics-and-memory/index.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Advanced Data Structures](../advanced-c/advanced-data-structures.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- [ISO/IEC 9899 standards and drafts](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [WG14 project status](https://open-std.org/jtc1/sc22/wg14/www/projects.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [GCC language extensions](https://gcc.gnu.org/onlinedocs/gcc/Extensions.html)
