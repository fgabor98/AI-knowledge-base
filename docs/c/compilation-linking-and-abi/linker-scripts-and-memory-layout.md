---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Linker Scripts And Memory Layout

A linker script maps input sections and symbols into an output image. In embedded work it is an executable memory contract: it says which regions exist, where code and data live, what must be copied or zeroed at startup, which image boundaries are available, and which invariants cause the build to fail.

## Learning Objectives

- read and write the core commands of a GNU ld linker script;
- distinguish load memory address (LMA) from virtual/run memory address (VMA);
- place vectors, code, constants, initialized data, bss, stacks, heaps, and metadata;
- export linker-defined symbols safely to C startup code;
- retain registration and vector sections under garbage collection;
- use map files and linker assertions as automated memory checks.

## The Memory Model

Describe the target before writing syntax:

| Region | Stored in image? | Executed/accessed from | Typical contents |
| --- | --- | --- | --- |
| boot ROM | no | fixed hardware address | immutable boot code |
| flash/ROM | yes | flash or execute-in-place | vectors, `.text`, `.rodata` |
| initialized RAM | initial bytes in flash | RAM after startup copy | `.data` |
| zeroed RAM | no initial payload | RAM after startup clear | `.bss`, zeroed stacks |
| retention RAM | optional | RAM across reset/sleep | crash records, state |
| peripheral window | no image payload | MMIO address range | device registers |
| external memory | optional | external controller | framebuffers, large buffers |

The image’s file representation and runtime memory representation can differ. Initialized data may have an LMA in flash and a VMA in RAM; startup copies bytes from the former to the latter.

## A Minimal GNU ld Script

The following is a minimal script, not a drop-in board script. It is the exact file the
session below links (`examples/c/tool-sessions/linker-script/firmware.ld`):

```text
--8<-- "examples/c/tool-sessions/linker-script/firmware.ld"
```

The syntax and target defaults belong to GNU ld or a compatible linker. `MEMORY` describes regions, `SECTIONS` maps input to output, `AT` gives a load address, `NOLOAD` reserves runtime space without file payload, `KEEP` prevents section garbage collection, and `ASSERT` makes a violated invariant a link failure.

Always inspect the actual map and program headers. A script that parses successfully can still place a region incorrectly for the board’s memory controller or boot protocol.

## Link It And Look

The script above, linked for real. The image is built with the host x86-64 compiler as a
freestanding program (`-ffreestanding -nostdlib -static -no-pie`), so it is inspected
rather than run. The linker's behavior is the same as with a microcontroller toolchain.
All output comes from `bash examples/c/tool-sessions/linker-script/run.sh`. The
application has one object of each kind:

```c
--8<-- "examples/c/tool-sessions/linker-script/app.c"
```

### Memory Usage At Link Time

```text
$ gcc -nostdlib -static -no-pie -T firmware.ld -Wl,-Map=firmware.map -Wl,--print-memory-usage startup.o app.o -o firmware.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:         272 B       256 KB      0.10%
             RAM:        4128 B        64 KB      6.30%
```

`--print-memory-usage` reports each `MEMORY` region after placement. Keep it in every
firmware build log, and diff it in review. RAM holds the 4-byte `.data` and the
4096-byte `.bss`, with alignment padding between them. FLASH holds `.text`, the *load copy*
of `.data`, and two note sections, discussed below.

### Load Address Versus Run Address

```text
$ objdump -h firmware.elf | grep -A1 -E ' \.(text|data|bss) '
  0 .text         000000c4  0000000008000000  0000000008000000  00001000  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
--
  3 .data         00000004  0000000020000000  000000000800010c  00002000  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  4 .bss          00001000  0000000020000020  0000000008000110  00002020  2**5
                  ALLOC
$ grep -A1 -E '^\.data ' firmware.map
.data           0x0000000020000000        0x4 load address 0x000000000800010c
                0x0000000020000000                . = ALIGN (0x4)
$ readelf -lW firmware.elf | grep -E 'Type|LOAD'
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  LOAD           0x001000 0x0000000008000000 0x0000000008000000 0x00010c 0x00010c R E 0x1000
  LOAD           0x002000 0x0000000020000000 0x000000000800010c 0x000004 0x000004 RW  0x1000
  LOAD           0x000020 0x0000000020000020 0x0000000008000110 0x000000 0x001000 RW  0x1000
```

The three tools show the same fact in three ways. In `objdump -h` the columns are VMA
then LMA. The map file says `load address`. In `readelf -l` they are `VirtAddr` and
`PhysAddr`. `.data` *runs* at `0x20000000` in RAM, while its initial bytes are *stored* at
`0x0800010c` in flash, which is what `> RAM AT> FLASH` means. `.bss` has `MemSiz`
0x1000 but `FileSiz` 0: `NOLOAD` reserves space without any bytes in the image.

Programming tools and `objcopy -O binary` place bytes by load address. A debugger that
loads an ELF over a probe follows the program headers. A wrong `AT>` therefore produces an
image that looks correct in the debugger's symbol view and fails at reset.

### The Symbols Startup Code Uses

```text
$ nm -n firmware.elf | grep -E '__|boot_count|sample_buffer|firmware_name'
00000000080000b8 T firmware_name
00000000080000c4 T __text_end__
000000000800010c A __data_load_start__
0000000020000000 D boot_count
0000000020000000 D __data_start__
0000000020000004 D __data_end__
0000000020000020 B __bss_start__
0000000020000020 B sample_buffer
0000000020001020 B __bss_end__
```

These are the values `initialize_memory()` below works with. Copy 4 bytes from
`0x0800010c` to `0x20000000`, then zero `0x20000020` up to `0x20001020`. `firmware_name`
appears as `T` because this script puts `.rodata` into the `.text` output section.
`__data_load_start__` is `A` (absolute) because it is assigned from `LOADADDR()` outside
any output section.

### Sections The Script Does Not Mention

The script never names `.note.gnu.build-id` or `.note.gnu.property`, yet the FLASH total
(272 bytes) is larger than `.text` (`0xc4` = 196 bytes) plus the 4-byte `.data` copy. GNU
ld places unmentioned *orphan* sections by its own heuristics. Ask it to report them:

```text
$ gcc -nostdlib -static -no-pie -T firmware.ld -Wl,--orphan-handling=warn startup.o app.o -o firmware_orphans.elf 2>&1 | head -4
/usr/bin/ld: warning: orphan section `.comment' from `startup.o' being placed in section `.comment'
/usr/bin/ld: warning: orphan section `.note.GNU-stack' from `startup.o' being placed in section `.note.GNU-stack'
/usr/bin/ld: warning: orphan section `.note.gnu.property' from `startup.o' being placed in section `.note.gnu.property'
/usr/bin/ld: warning: orphan section `.note.gnu.build-id' from `startup.o' being placed in section `.note.gnu.build-id'
```

A production script should account for every input section. Place the ones it needs
(a build-ID note in flash is useful for crash records), discard the rest with
`/DISCARD/ : { *(.comment) *(.note.GNU-stack) }`, and build with
`--orphan-handling=error` so a new compiler or library cannot silently add sections to
flash.

## Linker-Defined Symbols In C

A linker symbol is an address-like value, not a C object containing storage. A common declaration pattern is:

~~~c
#include <stddef.h>

extern unsigned char __data_load_start__[];
extern unsigned char __data_start__[];
extern unsigned char __data_end__[];
extern unsigned char __bss_start__[];
extern unsigned char __bss_end__[];

static void initialize_memory(void)
{
    size_t data_size = (size_t)(__data_end__ - __data_start__);
    for (size_t i = 0u; i < data_size; ++i) {
        __data_start__[i] = __data_load_start__[i];
    }

    for (unsigned char *p = __bss_start__; p != __bss_end__; ++p) {
        *p = 0u;
    }
}
~~~

The declarations, pointer arithmetic, alignment, and memory access model must match the target and compiler. Linker symbols do not automatically make a region safe to access. The startup code must run only after the relevant memory controller is initialized, and caches or protection units may require additional steps.

For a symbol that represents one address, `extern char symbol[]` is often clearer than declaring an object of an arbitrary type. Use a `uintptr_t` conversion only when the platform and compiler document that conversion and the address is being treated as an integer rather than dereferenced data.

## Sections For Firmware Contracts

Useful project-owned sections include:

- `.isr_vector` for the vector table;
- `.boot_header` for image metadata and version fields;
- `.fastcode` for code that must execute from low-latency RAM;
- `.dma_buffer` for buffers with alignment and cache policy requirements;
- `.noinit` for state intentionally preserved across a warm reset;
- `.retained` for backup or retention RAM;
- `.factory_calibration` for immutable calibration data;
- `.crash_record` for a post-reset diagnostic record.

Every custom section needs four pieces:

1. a C declaration or assembly producer;
2. a linker placement and alignment rule;
3. startup/cache/protection behavior;
4. a post-link or runtime validation test.

Do not create a section only to make a map file look organized. The section should express a real hardware, boot, update, or diagnostic contract.

## Garbage Collection And Retention

With `--gc-sections`, the linker begins from roots such as the entry point and retained sections, then follows relocations. Registration tables and vector entries can look unused because the consumer computes their address externally. Use a targeted rule:

~~~ld
.firmware_metadata :
{
    __metadata_start__ = .;
    KEEP(*(.firmware_metadata))
    __metadata_end__ = .;
} > FLASH
~~~

Pair this with a map-file check that asserts the expected number, size, version, and checksum of records. `KEEP` is not a substitute for proving that the consumer and producer agree.

## Alignment, Padding, And Boundaries

Alignment can consume flash and RAM and can be a hardware requirement. Consider:

- vector-table alignment;
- instruction fetch and execute-in-place alignment;
- DMA burst and cache-line alignment;
- MPU/MMU region granularity;
- erase-block and image-header alignment;
- stack alignment required by the ABI;
- padding inserted between output sections.

Use `ALIGN` deliberately and inspect both start and end addresses. A section can fit in a region while its end plus required guard area does not.

## Memory Assertions

Assertions should encode product limits:

~~~ld
ASSERT(SIZEOF(.text) <= 240K, "application code exceeds flash budget")
ASSERT(__data_end__ <= ORIGIN(RAM) + LENGTH(RAM), "data exceeds RAM")
ASSERT((__stack_top__ - __stack_limit__) >= 8K, "stack reservation too small")
ASSERT(ADDR(.dma_buffer) % 32 == 0, "DMA buffer alignment error")
~~~

Keep arithmetic in the linker’s address space and make the message actionable. Add CI checks for section sizes, load addresses, entry point, image holes, and bootloader boundaries.

With `.bss` grown past the 64 KB region, the example script fails with three messages:

```text
$ gcc -nostdlib -static -no-pie -T firmware.ld -Wl,--print-memory-usage startup.o app_big.o -o firmware_big.elf
/usr/bin/ld: firmware_big.elf section `.bss' will not fit in region `RAM'
/usr/bin/ld: RAM overflow
/usr/bin/ld: region `RAM' overflowed by 4496 bytes
Memory region         Used Size  Region Size  %age Used
           FLASH:         272 B       256 KB      0.10%
             RAM:       70032 B        64 KB    106.86%
collect2: error: ld returned 1 exit status
[exit status 1]
```

The first and third messages come from the `MEMORY` region check, the second from the
script's own `ASSERT`. A region check catches only "does not fit". An assertion can state
a product budget that is tighter than the hardware (`SIZEOF(.text) <= 240K`), an alignment
rule, or a minimum stack reservation, none of which the region check knows about.

## Bootloader And Multiple Images

For an application behind a bootloader, define explicitly:

- vector-table address and relocation behavior;
- application link address;
- boot metadata and signature coverage;
- image maximum size and erase alignment;
- RAM shared between bootloader and application;
- handoff register, stack, interrupt, cache, and clock state;
- rollback or trial-boot markers;
- whether initialized data is copied by the bootloader or application.

A linker script cannot by itself perform a safe handoff. It can expose symbols and enforce boundaries; startup code and the boot protocol must consume them correctly.

## Exercises

1. Add a custom `.firmware_metadata` section, retain it, and inspect its exact address and bytes.
2. Create separate flash and RAM LMAs/VMAs and implement the copy/clear loops in a test harness.
3. Add an intentionally oversized object and verify that an `ASSERT` fails the build.
4. Compare the map and raw image with and without section garbage collection.
5. Reserve a `.noinit` region and test warm-reset preservation without reading it before hardware initialization.
6. Add a DMA buffer with an alignment assertion and verify the resulting address.
7. Generate two application layouts behind a bootloader and document every handoff invariant.

## Common Mistakes

- treating a linker script as mere syntax rather than a hardware contract;
- confusing LMA with VMA;
- declaring a linker symbol as if it were an initialized C object;
- copying `.data` before flash or RAM is usable;
- clearing `.bss` over a region reserved for retention or a bootloader;
- forgetting vector-table alignment or instruction-region constraints;
- relying on section names without retention rules;
- using `NOLOAD` while still expecting bytes in the programmed image;
- omitting assertions for stack, heap, DMA, or bootloader boundaries;
- validating only the link result and not the actual programmed image.

## Related Topics

- [Object Files, Symbols, And Relocations](./object-files-symbols-and-relocations.md)
- [Startup, Runtime, And `main`](./startup-runtime-and-main.md)
- [Cross-Compilation And Sysroots](./cross-compilation-and-sysroots.md)
- [Bootloaders And Firmware Images](../embedded-c-and-hardware/bootloaders-and-firmware-images.md)
- [Memory Layout And Allocation](../semantics-and-memory/memory-layout-and-allocation.md)

## References

- [GNU ld linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html)
- [GNU ld simple linker script commands](https://sourceware.org/binutils/docs/ld/Simple-Commands.html)
- [GNU ld section garbage collection](https://sourceware.org/binutils/docs/ld/Options.html#index-gc-sections)
- [System V ABI and ELF specification](https://refspecs.linuxfoundation.org/elf/)
