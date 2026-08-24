---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Semantics And Memory

This chapter explains the rules beneath C syntax: what an object is, when its value exists, how pointers relate to objects, how conversions change values, and why apparently plausible code can become undefined behavior.

The language fundamentals chapter taught how to write the constructs. This chapter teaches how to decide whether those constructs are valid, portable, and safe on a real embedded target.

## The Core Mental Model

For every access, track five things:

1. The object: which region of storage is involved?
2. The type: what value representation and access rules apply?
3. The lifetime: does the object exist at this point?
4. The bounds and alignment: is this access within the object and correctly aligned?
5. The execution context: can another task, interrupt, DMA engine, or device change what is observed?

Most serious C defects are failures in one of these questions rather than failures of syntax.

## Recommended Progression

1. [Storage Duration, Scope, And Linkage](./storage-duration-scope-and-linkage.md) — identify names, objects, and lifetimes.
2. [Pointer Fundamentals](./pointer-fundamentals.md) — understand addresses, indirection, null, and ownership.
3. [Pointer Arithmetic And Bounds](./pointer-arithmetic-and-bounds.md) — keep pointer calculations inside array objects.
4. [Const, Volatile, And Restrict](./qualifiers-const-volatile-restrict.md) — express access and optimization contracts.
5. [Conversions, Promotions, And Aliasing](./conversions-promotions-and-aliasing.md) — reason about type changes and legal access paths.
6. [Object Representation, Alignment, And Padding](./object-representation-alignment-and-padding.md) — separate values from bytes.
7. [Memory Layout And Allocation](./memory-layout-and-allocation.md) — connect C objects to sections, stacks, heaps, and pools.
8. [Undefined Behavior](./undefined-behavior.md) — recognize where the standard stops defining a result.
9. [Memory Safety And Lifetime](./memory-safety-and-lifetime.md) — turn the rules into ownership and review practices.

Read the undefined-behavior page early, then return to it after each other page. It is the boundary condition for all of the examples.

## Categories Of Behavior

Use precise language when describing a result:

| Category | Meaning | Engineering response |
| --- | --- | --- |
| Fully defined | The standard requires the result | Rely on it within the selected standard |
| Implementation-defined | The implementation chooses and documents a result | Check compiler, ABI, and target documentation |
| Unspecified | The implementation may choose among permitted results without documenting which | Do not depend on one choice |
| Locale-dependent | Library behavior depends on the active locale | Set and document the locale or avoid the dependency |
| Constraint violation | A required diagnostic applies to a source program | Fix the program; a diagnostic does not make it safe |
| Undefined behavior | The standard imposes no requirements | Remove the condition; testing one result proves nothing |

An implementation may document an extension or a choice, but that does not make it portable ISO C. A target manual can further constrain what is safe for a particular register, interrupt, or memory region.

## Why Embedded Code Needs This Chapter

Bare-metal and RTOS code frequently uses:

- addresses supplied by linker scripts or device manuals;
- volatile objects mapped onto hardware;
- fixed-size storage and custom allocators;
- interrupt-shared state;
- byte-level protocol parsing;
- compiler extensions for sections, packing, or calling conventions.

These are valid engineering tools only when the language rule, compiler behavior, ABI, and hardware contract are all identified. A cast to a register address does not create a valid object, a volatile qualifier does not make an access atomic, and a structure layout does not become a protocol format automatically.

## Investigation Workflow

When a low-level result is surprising:

1. Reduce the code to one object, one access, and one observable result.
2. State the intended object, type, lifetime, bounds, alignment, and context.
3. Compile with strict warnings and the intended language version.
4. Run a host test with sanitizers when the operation is host-representable.
5. Inspect preprocessed source, object code, disassembly, and map files as appropriate.
6. Compare the result with the ISO C rule and the target ABI or hardware manual.
7. Replace assumptions with an explicit check, conversion, ownership rule, or abstraction.

Do not start by changing optimization flags. First determine whether the source has defined behavior.

## Chapter Outcomes

After completing this chapter, you should be able to:

- draw a lifetime and ownership graph for a C data structure;
- explain why a pointer value is not the same thing as a valid object access;
- calculate legal array bounds and one-past pointers;
- distinguish const, volatile, restrict, and atomic responsibilities;
- predict common integer and pointer conversions;
- explain effective type and strict-aliasing constraints;
- inspect alignment, padding, and object representation;
- choose between static storage, heap allocation, pools, and arenas;
- identify undefined behavior before relying on a test result;
- design interfaces that make invalid states difficult to express.

## Exercises Across The Chapter

Use one packet-processing component as a running exercise:

1. Start with a borrowed pointer and explicit length.
2. Add a parsed structure whose lifetime is owned by the caller.
3. Add a scratch arena with an explicit reset point.
4. Serialize the result with explicit byte order and alignment-safe operations.
5. Test truncated, misaligned, aliased, and concurrently modified inputs.
6. Run host tests under AddressSanitizer and UndefinedBehaviorSanitizer.
7. Review the component for every implementation-defined or target-specific assumption.

## Related Topics

- [Language Fundamentals](../language-fundamentals/index.md)
- [Modular Design And APIs](../modular-design-and-apis/index.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Correctness, Quality, And Security](../correctness-quality-and-security/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)
- [C Programming](../index.md)
- [Topic Map](../../topic-map.md)

## References

- [ISO/IEC 9899 standards and drafts — WG14](https://open-std.org/jtc1/sc22/wg14/www/standards.html)
- [C11 public draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
- [Clarifying the C memory object model — WG14 N2012](https://open-std.org/jtc1/sc22/wg14/www/docs/n2012.htm)
- [GCC optimization options and strict aliasing](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
