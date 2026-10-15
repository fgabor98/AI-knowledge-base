---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Debug Information And Binary Inspection

The final ELF, map file, debug records, disassembly, and image manifest are the ground truth for what was built. Source code and build logs are necessary context, but they cannot answer where a function landed, which library member was selected, whether a vector was retained, or which instruction faulted.

## Learning Objectives

- generate and preserve useful debug information for optimized C;
- inspect ELF headers, sections, segments, symbols, relocations, and sizes;
- resolve a program counter to a source location with the exact image;
- use GDB, `addr2line`, `objdump`, `readelf`, `nm`, and map files together;
- separate debug artifacts from deployable images without losing provenance;
- diagnose embedded faults, stripped production images, and address relocation correctly.

## Debug Formats And Compilation

GCC-family compilers commonly emit DWARF debug information with `-g`. The debug records describe source files, line tables, types, variables, scopes, inline call sites, and machine locations. They usually live in `.debug_*` sections that are not loaded into the target’s runtime memory.

Useful build choices include:

~~~sh
cc -std=c17 -Og -g3 -fno-omit-frame-pointer -c app.c -o app.o
cc -Og -g3 app.o -o app
~~~

`-g` can be combined with optimization. Optimized code may have variables unavailable at a breakpoint, statements reordered, inlined functions represented as nested call sites, and prologues split across ranges. Use `-Og` for interactive diagnosis when it reproduces the issue, but keep a production-like optimized build for optimization-sensitive failures.

Keep compile and link debug settings consistent enough for the debugger to associate the final image with all objects. Store the unstripped ELF even if the device receives a `.bin` or `.hex`.

## Binary Inspection Toolkit

| Tool | Questions it answers |
| --- | --- |
| `readelf -h` | class, machine, entry point, ABI flags |
| `readelf -S` | sections, addresses, sizes, flags, alignments |
| `readelf -l` | loadable segments, permissions, file-to-memory mapping |
| `readelf -s` | symbol binding, type, visibility, size, section |
| `readelf -r` | relocations still present |
| `readelf -d` | dynamic dependencies and loader metadata |
| `objdump -d` | disassembly and instruction addresses |
| `objdump -dr` | disassembly with relocation annotations |
| `nm` | concise symbol presence and address view |
| `size` | text/data/bss summary |
| `strings` | embedded text and unexpected paths |
| `addr2line` | address to file/line/function using debug information |
| `gdb` | interactive state, registers, memory, symbols, and backtraces |

Commands are GNU-family examples; options and output differ by target.

## Worked Session: From A Field Crash To A Source Line

This section uses the toolkit on one small program, from build to a symbolized field
crash. All output comes from `bash examples/c/tool-sessions/binary-inspection/run.sh`
(GCC 13.3, binutils 2.42, x86-64). The program validates frames; a corrupt frame reaches
a `__builtin_trap()` inside a helper that the optimizer inlines twice:

```c
--8<-- "examples/c/tool-sessions/binary-inspection/frame.c:5:33"
```

### Build And Identify

Compile each file separately so the map file can name the object each function came
from. Linking straight from `.c` files leaves temporary names such as `/tmp/ccB5iCVh.o`
in the map.

```text
$ gcc -c -O2 -g -Wall -Wextra -ffunction-sections -fdata-sections main.c -o main.o
$ gcc -c -O2 -g -Wall -Wextra -ffunction-sections -fdata-sections frame.c -o frame.o
$ gcc -c -O2 -g -Wall -Wextra -ffunction-sections -fdata-sections crash_record.c -o crash_record.o
$ gcc main.o frame.o crash_record.o -Wl,-Map=app.map -o app
$ readelf -h app | grep -E 'Class|Type|Machine|Entry'
  Class:                             ELF64
  Type:                              DYN (Position-Independent Executable file)
  Machine:                           Advanced Micro Devices X86-64
  Entry point address:               0x11b0
$ readelf -n app | grep 'Build ID'
    Build ID: 9e7dfeac0dfd49ddcd9bb17ad8e69be2ff5be647
```

`DYN` means a position-independent executable. Every address the tools print below
(`0x11b0`, `0x12a0`) is relative to wherever the loader maps the program at run time. The
build ID is a hash of the linked output, which the linker stores in a note section. It is
the reliable way to match a crash report, a stripped binary, and a debug file.

### Sections, Sizes, Symbols, And The Map

```text
$ readelf -SW app | grep -E '\] \.(text|rodata|data|bss|debug_info|debug_line|symtab)'
  [16] .text             PROGBITS        0000000000001120 001120 0004e7 00  AX  0   0 16
  [18] .rodata           PROGBITS        0000000000002000 002000 00004d 00   A  0   0  4
  [25] .data             PROGBITS        0000000000004000 003000 000010 00  WA  0   0  8
  [26] .bss              NOBITS          0000000000004020 003010 000050 00  WA  0   0 32
  [29] .debug_info       PROGBITS        0000000000000000 00311d 0013f2 00      0   0  1
  [31] .debug_line       PROGBITS        0000000000000000 004b46 000519 00      0   0  1
  [33] .debug_line_str   PROGBITS        0000000000000000 0057c0 0001a0 01  MS  0   0  1
  [36] .symtab           SYMTAB          0000000000000000 005ef8 0004f8 18     37  27  8
$ size app
   text	   data	    bss	    dec	    hex	filename
   3327	    648	     80	   4055	    fd7	app
$ nm -S --size-sort app | tail -5
0000000000001350 0000000000000079 t put_hex
0000000000001130 000000000000007b T main
00000000000012a0 0000000000000085 T frame_accept
0000000000001570 0000000000000097 T crash_record_install
0000000000001450 0000000000000118 t find_build_id
$ grep -A1 -E '^ \.text\.(frame_accept|on_fatal_signal|crash_record_install)' app.map
 .text.frame_accept
                0x00000000000012a0       0x85 frame.o
--
 .text.on_fatal_signal
                0x00000000000013d0       0x71 crash_record.o
--
 .text.crash_record_install
                0x0000000000001570       0x97 crash_record.o
```

- The `.debug_*` sections have address 0 and no `A` (alloc) flag: they are in the file
  but never loaded. They make up most of the 29 KB file, as the stripping step below
  shows.
- `.bss` is `NOBITS`: it has a size (0x50) but takes no file space.
- `size` adds sections by permission, not by name: its `text` column also counts
  `.rodata` and other read-only sections, so it is larger than `.text` alone.
- `nm` type letters: `T` is a global function, `t` a file-local (`static`) one.
  `frame_checksum` and `frame_valid` have no symbol at all, because they were inlined.

### Disassembly: Hot And Cold Parts

```text
$ objdump -d --no-show-raw-insn -M intel --disassemble=frame_accept app | ...
00000000000012a0 <frame_accept>:
    12a0:	endbr64
    ...
    12ec:	cmp    rsi,0xc
    12f0:	je     1120 <frame_accept.cold>
    12f6:	lea    rax,[rdi+0xc]
    12fa:	xor    edx,edx
    12fc:	add    rdi,rsi
    12ff:	nop
    1300:	movzx  ecx,BYTE PTR [rax]
    1303:	add    rax,0x1
    1307:	add    edx,ecx
    1309:	cmp    rdi,rax
    130c:	jne    1300 <frame_accept+0x60>
    130e:	test   dx,dx
    1311:	je     1120 <frame_accept.cold>
    ...
$ objdump -d --no-show-raw-insn -M intel --disassemble=frame_accept.cold app | ...
0000000000001120 <frame_accept.cold>:
    1120:	ud2
```

The checksum loop from `frame_checksum` sits inside `frame_accept`. GCC moved the
unlikely trap path into a separate `frame_accept.cold` fragment at the start of
`.text`, so the hot code stays compact. When a crash address resolves to a symbol with
a `.cold` suffix, the cause is in the parent function, here `frame_accept`. `ud2` is the
x86 instruction `__builtin_trap()` compiles to, and it raises `SIGILL`.

### Struct Layout From DWARF

```text
$ gdb -q -batch -ex 'ptype /o struct frame_header' app
/* offset      |    size */  type = struct frame_header {
/*      0      |       1 */    uint8_t version;
/* XXX  3-byte hole      */
/*      4      |       4 */    uint32_t sequence;
/*      8      |       1 */    uint8_t type;
/* XXX  1-byte hole      */
/*     10      |       2 */    uint16_t length;

                               /* total size (bytes):   12 */
                             }
```

`ptype /o` reads the layout from debug information: offsets, sizes, and padding holes.
That is the layout the compiler actually used, rather than one worked out by hand. See
[case study 10](../failure-case-studies/10-struct-padding-leak.md) for what those holes
can leak.

### Strip For Deployment, Keep The Debug File

```text
$ objcopy --only-keep-debug app app.debug
$ objcopy --strip-debug --strip-unneeded --add-gnu-debuglink=app.debug app app.deployed
$ ls -l app app.debug app.deployed | awk '{print $5, $9}'
29216 app
17856 app.debug
14576 app.deployed
$ readelf -n app.deployed | grep 'Build ID'; readelf -n app.debug 2>/dev/null | grep 'Build ID'
    Build ID: 9e7dfeac0dfd49ddcd9bb17ad8e69be2ff5be647
    Build ID: 9e7dfeac0dfd49ddcd9bb17ad8e69be2ff5be647
```

The deployed file is half the size, and the two files share a build ID, so they can be
matched later. (`readelf -n` on the debug file also prints a harmless error about the
program interpreter, because the debug file keeps no loadable contents; the session
discards it.)

### A Field Crash Record

On the device there is no debugger and no symbol table. The program installs a small
handler that prints only what offline tools need: the signal, the build ID, and the
faulting address *relative to the executable's load base*. The build ID and the load
base are looked up once at start-up with `dl_iterate_phdr`, so the handler only does
arithmetic and calls `write` and `_exit`, both async-signal-safe. Calling `dladdr` in the
handler would be simpler but unsafe: it is not async-signal-safe, and glibc's version
takes the dynamic loader's lock, so a crash inside the loader would deadlock the handler.

```c
--8<-- "examples/c/tool-sessions/binary-inspection/crash_record.c:75:89"
```

```text
$ ./app.deployed
crash: signal 0x4 build-id 9e7dfeac0dfd49ddcd9bb17ad8e69be2ff5be647 module-offset 0x1120
[exit status 132]
```

Signal 4 is `SIGILL` (the `ud2`), and `0x1120` is the address of `frame_accept.cold`. A
raw program counter would be useless here: address-space randomization places a PIE at
a different base on every run, so only the offset is stable.

### Symbolize Offline

```text
$ mkdir -p elsewhere && mv app.debug elsewhere/
$ addr2line -f -i -p -e app.deployed 0x1120
?? ??:0
$ mv elsewhere/app.debug . && addr2line -f -i -p -e app.deployed 0x1120
frame_checksum at ./frame.c:13
 (inlined by) frame_valid at ./frame.c:21
 (inlined by) frame_accept at ./frame.c:32
```

- Without the debug file, the stripped binary cannot be symbolized.
- With `app.debug` next to it, `addr2line` follows the `.gnu_debuglink` section to it,
  and `-i` expands the inline chain: the trap is at `frame.c:13`, in `frame_checksum`,
  inlined into `frame_valid`, inlined into `frame_accept`. A plain `addr2line -f` would
  print only `frame_checksum` and hide the two callers.
- The debug link records only a file name and a CRC. Before trusting the result,
  compare the build ID in the crash record with the debug file's build ID, as above.

### Reproducible Build IDs

The build ID hashes everything in the output, including the debug information, and the
debug information records the build directory. The same sources built in two
directories:

```text
$ for tree in tree_a tree_b; do (cd $tree && gcc -O2 -g main.c frame.c crash_record.c -o app && readelf -n app | grep 'Build ID'); done
    Build ID: c4c06f7d4959e58da6079b5ede70087b206009e2
    Build ID: d309c93d5d0611a1bcd04cf1264b445a0d38a6b5
$ for tree in tree_a tree_b; do (cd $tree && gcc -O2 -g -ffile-prefix-map=$PWD=. main.c frame.c crash_record.c -o app && readelf -n app | grep 'Build ID'); done
    Build ID: 7d137644f66d7fe61b4c0c5cd55e54caee66caa5
    Build ID: 7d137644f66d7fe61b4c0c5cd55e54caee66caa5
```

Without `-ffile-prefix-map` (or `-fdebug-prefix-map`), every CI worker produces a
different build ID for the same commit. That breaks reproducible-build checks and
makes artifact lookup by build ID depend on which machine built the image.

## Map Files

Ask the linker for a map file where supported:

~~~sh
cc objects.o -Wl,-Map=app.map,--cref -o app.elf
~~~

A useful map review checks:

- entry point and vector address;
- output section start, end, size, and alignment;
- flash load addresses and RAM run addresses;
- largest functions and data objects;
- archive members pulled into the image;
- discarded sections;
- duplicate or unexpected symbols;
- linker-defined boundaries and assertions;
- memory-region utilization and unexplained holes.

Make map review partly automated. A CI script can fail on flash/RAM limits, missing metadata, unexpected dependencies, or a changed entry point, while a human reviews meaningful deltas.

## Resolving Addresses

For a hosted non-PIE or fixed-address firmware image:

~~~sh
addr2line -e app.elf -f -C 0x08001234
arm-none-eabi-addr2line -e firmware.elf -f -C 0x08001234
~~~

For an address inside a shared object or PIE, subtract the object’s runtime load base to obtain the link-time-relative address, or let GDB use the loaded mappings. If the target has an exception PC, first determine whether it points to the faulting instruction, the following instruction, a return address, or an architecture-specific frame value.

The address is meaningful only with:

- the exact ELF that ran;
- the exact load base or relocation slide;
- matching debug information;
- the correct instruction-set state and address masking rules;
- knowledge of whether the crash record was captured before or after stack unwinding.

Never resolve a field crash against the newest build because the source line “looks close.” Preserve a build ID and artifact mapping.

## Separate Debug Files And Stripping

Deployment often strips debug sections while retaining a symbol-bearing artifact offline:

~~~sh
objcopy --only-keep-debug app.elf app.elf.debug
strip --strip-debug --strip-unneeded app.elf
objcopy --add-gnu-debuglink=app.elf.debug app.elf
~~~

The exact strip policy depends on the target and whether symbols are needed for field diagnostics. A separate debug file must match the executable; a name match alone is not enough. Use a build ID, debug link, artifact registry, and checksum policy.

For firmware, keep at least:

- unstripped ELF with DWARF;
- stripped or programmer image;
- linker map;
- compiler/linker command manifest;
- source and toolchain revisions;
- post-link checksum/signature inputs;
- memory and symbol reports.

## GDB Workflow

[Debugging With GDB](../correctness-quality-and-security/debugging-with-gdb.md) walks
through a full hosted session: crash, watchpoint, core file, and optimized code. For a
remote embedded target, load the exact ELF for symbols, connect through the debug server, reset and halt according to the board policy, then inspect registers, fault status, stack, and memory. Do not assume a debugger reset is equivalent to a power-on reset; peripherals, retention RAM, watchdogs, and boot ROM state can differ.

When a backtrace is corrupt, inspect:

- stack pointer alignment and bounds;
- saved link/return address encoding;
- stack overflow or overwrite evidence;
- frame-pointer and unwind metadata;
- interrupt nesting and exception frames;
- compiler optimization and tail-call transformations;
- whether the loaded ELF matches the target image.

## Disassembly As A Contract Check

Disassembly can answer whether:

- a function was inlined or eliminated;
- a branch was range-expanded;
- a supposedly constant access became a load;
- a volatile register access was emitted;
- a stack frame fits the ABI alignment rule;
- an unexpected floating-point helper or library call was pulled in;
- a security or memory barrier remains in the generated code;
- code was placed in the intended flash/RAM section.

Use it to test a specific hypothesis, not to replace source-level reasoning. A machine instruction sequence is target-specific and may change with compiler version, flags, link order, or profile data.

## Size And Resource Analysis

Use multiple views:

~~~sh
size firmware.elf
arm-none-eabi-size -A firmware.elf
arm-none-eabi-nm -S --size-sort firmware.elf | tail -n 30
arm-none-eabi-objdump -h firmware.elf
readelf -SW firmware.elf
~~~

Track flash file payload, flash execution region, RAM allocation, zero-fill RAM, retained RAM, stack reservation, heap reservation, and external memory independently. A single “binary size” number hides the resource that may actually fail.

## Embedded Fault Evidence

A production fault record should include, where available:

- reset cause and watchdog state;
- fault status registers;
- program counter and link register;
- stack pointer and a bounded stack snapshot;
- active exception/interrupt number;
- image version and build ID;
- task/thread identifier;
- recent event or trace records;
- memory protection or bus fault address;
- whether caches, DMA, and clocks were configured.

The fault handler must use only initialized, reentrant, nonblocking facilities that are safe in that context. Store a compact record and defer formatting or transport until the next healthy boot.

## Reproducibility And Provenance

A debug artifact is useful only if its provenance is trustworthy. Include:

- source revision and dirty-tree state;
- compiler, assembler, linker, libc, and binutils versions;
- target triple and complete options;
- linker script and generated headers;
- LTO/PGO/profile inputs;
- binary conversion, signing, and flashing steps;
- build ID and hashes of deployable and debug files.

Reproducible builds make it possible to compare a changed instruction or section and to prove that a debug file belongs to the deployed image.

## Exercises

1. Run `examples/c/tool-sessions/binary-inspection/run.sh`, then give `app.deployed` a
   debug file from a *different* build and show that `addr2line` still prints a
   confident, wrong answer. Write the build-ID check that rejects it.
2. Generate a map and write a report of the ten largest code and data symbols.
3. Extend the crash record with the stacked return address and resolve the caller
   offline as well.
4. Compare disassembly with and without inlining, LTO, frame pointers, and section garbage collection.
5. Create a PIE/shared-object address-resolution exercise using a runtime load base.
6. Add a build ID and verify that a mismatched debug file is rejected.
7. Define a field-fault artifact retention policy and test it with two intentionally similar builds.

## Common Mistakes

- debugging a stripped image without preserving matching DWARF;
- using the source revision rather than the exact ELF for address resolution;
- forgetting a PIE/shared-object relocation slide;
- treating a source line as an exact instruction boundary under optimization;
- trusting a corrupt backtrace without inspecting stack and ABI state;
- measuring only `.text` and ignoring bss, stack, heap, or alignment padding;
- relying on `ldd` or one tool’s summary instead of inspecting the ELF;
- printing absolute PCs from a PIE or shared object instead of module-relative offsets;
- building release artifacts without `-ffile-prefix-map`, so build IDs differ by machine;
- letting a fault handler call malloc, printf, locks, or a driver that is not fault-safe;
- omitting linker scripts, generated headers, or post-link tools from provenance;
- assuming debugger reset, watchdog reset, and power-on reset have identical state.

## Related Topics

- [Compiler Modes, Warnings, And Optimization](./compiler-modes-warnings-and-optimization.md)
- [Object Files, Symbols, And Relocations](./object-files-symbols-and-relocations.md)
- [Static And Dynamic Linking](./static-and-dynamic-linking.md)
- [Startup, Runtime, And `main`](./startup-runtime-and-main.md)
- [Debugging With GDB](../correctness-quality-and-security/debugging-with-gdb.md)
- [Bootloaders And Firmware Images](../embedded-c-and-hardware/bootloaders-and-firmware-images.md)

## References

- [GCC debugging options](https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html)
- [GDB documentation](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)
- [GDB separate debug files](https://www.sourceware.org/gdb/current/onlinedocs/gdb.html/Separate-Debug-Files.html)
- [GNU Binary Utilities documentation](https://sourceware.org/binutils/docs/binutils.html)
- [GNU `addr2line` documentation](https://sourceware.org/binutils/docs/binutils/addr2line.html)
- [GNU `objdump` documentation](https://sourceware.org/binutils/docs/binutils/objdump.html)
