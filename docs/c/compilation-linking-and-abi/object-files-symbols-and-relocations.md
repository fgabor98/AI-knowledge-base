---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Object Files, Symbols, And Relocations

An object file is the compiler and assembler’s promise that machine code, data, metadata, and unresolved references can be combined later. On many embedded GNU toolchains the format is ELF, but the ideas apply to other object formats as well.

## Learning Objectives

- read ELF sections, segments, symbols, and relocation records;
- explain why a reference remains unresolved in a `.o` file;
- understand archive extraction, symbol binding, visibility, weak definitions, and interposition;
- distinguish link-time addresses from load-time addresses;
- use `readelf`, `objdump`, `nm`, `size`, and map files to investigate a binary;
- recognize object-format and ABI assumptions that are not portable ISO C.

## Sections And Segments

Sections organize link-time content. Common ELF sections include:

| Section | Typical contents | Usual final permissions |
| --- | --- | --- |
| `.text` | executable code | read/execute |
| `.rodata` | string literals and constants | read-only |
| `.data` | initialized writable objects | read/write |
| `.bss` | zero-initialized or no-file-data objects | read/write |
| `.init_array` | initialization function pointers | read-only or read/write by platform |
| `.fini_array` | termination function pointers | read-only or read/write by platform |
| `.debug_*` | DWARF debug information | not loaded at runtime |
| `.rel*`/`.rela*` | relocation records | link-time or dynamic-link metadata |
| `.ARM.exidx` or target equivalent | unwind/index data | target-specific |

The linker maps input sections into output sections, then program headers describe loadable **segments** for an operating-system loader. Sections are useful for link-time organization; segments are the loader’s view of memory permissions and file-to-memory mapping. A bare-metal image may use the section table and a custom script without an OS loader, while a hosted ELF uses program headers to create a process image.

Inspect both views:

~~~sh
readelf -hW app.elf       # ELF class, machine, entry point
readelf -SW app.elf       # sections and flags
readelf -lW app.elf       # loadable segments and permissions
objdump -h app.elf       # section summary
size app.elf             # text/data/bss summary where supported
~~~

Do not add section sizes as if they were all stored bytes. `.bss` occupies memory but normally contributes little or no file payload; alignment and segment padding can also make flash consumption differ from a simple sum.

## Symbols

A symbol associates a name with an address, size, section, binding, and visibility. A symbol can be:

- **defined** in the current object;
- **undefined** and expected from another object or library;
- **absolute**, with a value not relative to a section;
- **common** in toolchain-specific legacy modes;
- **local**, visible only within the object;
- **global**, available to other objects;
- **weak**, replaceable by a strong definition;
- **hidden/protected/default visibility**, controlling dynamic visibility on supported systems.

Inspect symbols at different stages:

~~~sh
nm -C message.o
readelf -Ws message.o
objdump -t message.o
nm -S --size-sort app.elf | tail -n 20
~~~

In `nm` output, `U` commonly means undefined, `T` text, `D` initialized data, `B` bss, `R` read-only data, and lowercase variants often indicate local symbols. Exact letters and meanings are tool-specific; use `readelf` for authoritative ELF binding, type, and visibility fields.

## Relocations

When the assembler cannot know a final address, it emits a relocation. A relocation records a location to update, a symbol, an addend or instruction encoding, and a relocation type defined by the target ABI. Examples include:

- a call from one object to an external function;
- an address stored in a global pointer;
- a reference to a linker-defined symbol;
- a PC-relative branch whose range is checked at link time;
- a GOT or PLT entry for position-independent or dynamically linked code.

View relocations before and after linking:

~~~sh
readelf -rW message.o
objdump -dr message.o
readelf -rW app.elf
~~~

An undefined symbol is not automatically an error in an object file. It becomes a link failure if no compatible definition is available, or a runtime loading failure if the unresolved reference belongs to a dynamic object and cannot be resolved by the loader.

## Static Archives

A static archive is an index plus a collection of object files, usually named `libname.a`. The linker generally extracts only members that satisfy currently unresolved references. This explains several behaviors:

- an unused archive member may not appear in the output;
- library order can matter;
- a cyclic dependency may require repeating an archive or using a group option;
- an object passed directly is usually included more eagerly than an archive member;
- section garbage collection can remove extracted code that has no retained path.

Example:

~~~sh
cc -c crc.c -o crc.o
ar rcs libprotocol.a crc.o
cc -Wl,--start-group app.o libprotocol.a libboard.a -Wl,--end-group -o app
nm -s libprotocol.a
~~~

Prefer a clear dependency graph over arbitrary archive repetition. Map files and linker trace options can show why a member was extracted.

## Binding, Visibility, And Interposition

C’s `static` at file scope gives internal linkage; it does not mean static storage duration when used in a block. For exported interfaces, use a deliberate visibility policy:

~~~c
#if defined(__GNUC__) || defined(__clang__)
#define API_PUBLIC __attribute__((visibility("default")))
#define API_LOCAL  __attribute__((visibility("hidden")))
#else
#define API_PUBLIC
#define API_LOCAL
#endif

API_PUBLIC int protocol_start(void);
API_LOCAL int protocol_decode_internal(const unsigned char *p);
~~~

On hosted systems, default-visible symbols in shared objects can be preempted or interposed depending on the platform and link options. On bare metal, symbol visibility is mostly a link-time organization and debug concern. Do not assume that a symbol’s name alone defines a stable API; document its type, calling convention, ownership, version, and lifetime.

## Weak Symbols

Weak symbols are useful for optional hooks and vendor defaults:

~~~c
void board_idle_hook(void) __attribute__((weak));

void board_idle_hook(void)
{
    /* Default no-op. */
}
~~~

An application can provide a strong definition that overrides the weak one. Weak behavior is toolchain and object-format dependent. It can hide a missing implementation, vary with archive extraction, and interact badly with LTO or section garbage collection. Use it only for an explicit override contract and verify the selected symbol in the map.

## Position-Independent Code

Position-independent code avoids absolute addresses that would require rewriting code when loaded at a different address. On hosted systems this supports shared libraries and PIE; it commonly uses a global offset table and procedure linkage table. On microcontrollers, firmware usually has a fixed link address, but relocatable bootloaders, dual-bank images, execute-in-place designs, and secure loaders may need a relocation model.

The choice affects:

- instruction sequences and register use;
- code size and performance;
- writable relocation data;
- dynamic loader requirements;
- whether function pointers and tables remain valid after image movement;
- how a signature is computed over the image.

Never call an image “relocatable” merely because it is a raw binary. Verify all absolute references, vector entries, data pointers, relocation tables, and loader responsibilities.

## Common Link Failures

| Diagnostic | Likely causes | Useful evidence |
| --- | --- | --- |
| undefined reference | missing object/library, wrong order, hidden symbol | `nm`, link map, full link command |
| multiple definition | duplicate external definition or incompatible archive members | symbol binding and source ownership |
| relocation truncated | branch/address range or wrong memory placement | relocation type, section addresses, target ABI |
| incompatible architecture | host object or wrong multilib | `readelf -h`, driver target, archive members |
| cannot find `-lfoo` | wrong sysroot or library path | `-v`, `--sysroot`, `-L`, file search |
| discarded section referenced | missing `KEEP`, retention, or registration path | map file and `--gc-sections` diagnostics |

## Exercises

1. Inspect an object before and after linking and list every changed relocation.
2. Put two functions into separate sections and compare normal linking with `--gc-sections`.
3. Build an archive with three members and identify which members are extracted.
4. Reverse two library arguments and explain the changed result.
5. Add a weak board hook, override it strongly, and confirm the selected address.
6. Compare a position-dependent and position-independent hosted object with `readelf -r` and disassembly.
7. Intentionally link a host object into a target build and identify the earliest reliable diagnostic.

## Common Mistakes

- confusing sections with loadable segments;
- assuming `.bss` consumes file space exactly like `.data`;
- reading `nm` letters as universal semantics without checking the object format;
- expecting all members of a static archive to be included;
- relying on archive order by accident;
- using weak symbols to hide required implementations;
- treating symbol names as a complete ABI;
- assuming a raw binary is relocatable;
- ignoring target-specific relocation range and alignment limits;
- inspecting only the final executable when the first incorrect decision happened in an object file.

## Related Topics

- [Translation Pipeline](./translation-pipeline.md)
- [Static And Dynamic Linking](./static-and-dynamic-linking.md)
- [Linker Scripts And Memory Layout](./linker-scripts-and-memory-layout.md)
- [ABI, Calling Conventions, And FFI](./abi-calling-conventions-and-ffi.md)
- [Debug Information And Binary Inspection](./debug-information-and-binary-inspection.md)

## References

- [System V ABI and ELF specification](https://refspecs.linuxfoundation.org/elf/)
- [GNU `nm` documentation](https://sourceware.org/binutils/docs/binutils/nm.html)
- [GNU `readelf` documentation](https://sourceware.org/binutils/docs/binutils/readelf.html)
- [GNU `objdump` documentation](https://sourceware.org/binutils/docs/binutils/objdump.html)
- [GNU `ar` documentation](https://sourceware.org/binutils/docs/binutils/ar.html)
- [GNU ld overview](https://sourceware.org/binutils/docs/ld/Overview.html)
