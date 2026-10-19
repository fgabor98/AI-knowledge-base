---
status: draft
reviewed: false
domain: c
difficulty: intermediate
last_reviewed: null
---

# Static And Dynamic Linking

Linking combines separately compiled code into a program. **Static linking** copies selected object code into the output at build time. **Dynamic linking** leaves shared-object relationships for a loader to resolve at process startup or during execution. Embedded systems can use either model, but fixed-address firmware most often uses static linking plus a custom image layout.

## Learning Objectives

- compare archive, static library, shared object, and runtime loader behavior;
- understand startup objects, compiler runtime libraries, and libc dependencies;
- control library search order, symbol visibility, and dependency boundaries;
- use section garbage collection and LTO without discarding required entry points;
- decide when static, dynamic, or hybrid linking fits an embedded or hosted product;
- inspect dependencies and verify that the produced image has the intended runtime contract.

## Worked Examples On This Page

The transcripts below come from `bash examples/c/tool-sessions/linking/run.sh` (GCC 13.3,
GNU ld 2.42, glibc on x86-64). They use one small library, `libsensor`, built from three
files. One of them, `sensor_debug.c`, is a diagnostics module that the application never
calls:

```c
--8<-- "examples/c/tool-sessions/linking/sensor.h"
```

## Static Linking

In a static link, the linker resolves references and copies required code and data from relocatable objects and archives into the output. A typical hosted command may include compiler-provided startup and runtime components implicitly:

~~~sh
cc -static app.o libprotocol.a -o app-static
~~~

The exact result depends on the platform and availability of static libc and other libraries. Static does not mean “no runtime initialization”; the output still needs an entry point, startup code, compiler support, and often a libc.

Embedded static linking commonly adds:

- a reset handler and vector table;
- a target linker script;
- board or RTOS startup objects;
- a selected libc and compiler runtime;
- hardware initialization and image metadata;
- post-link conversion to a programming format.

Advantages include a self-contained image, predictable deployed dependencies, and easy flash-image hashing. Costs include duplicated library code across applications, larger updates, and the need to rebuild for library fixes.

## Dynamic Linking

In a dynamically linked hosted executable, the link editor records shared-object dependencies and a program interpreter. At load time, the operating system and dynamic linker map segments, resolve symbols, and apply relocations. ELF commonly represents the interpreter using a `PT_INTERP` program header.

Inspect a dynamic program:

~~~sh
readelf -lW app                 # look for PT_INTERP
readelf -dW app                 # NEEDED, SONAME, RPATH/RUNPATH
ldd app                         # platform tool; do not use on untrusted files
objdump -p app | sed -n '/Dynamic Section:/,/Version References:/p'
~~~

The loader may resolve a function through a procedure linkage table and global offset table, either eagerly or lazily depending on platform and configuration. This affects startup time, writable memory, attack surface, and failure timing.

A shared library built with a SONAME, then linked and run:

```text
$ gcc -O2 -g -fPIC -shared -Wl,-soname,libsensor.so.1 sensor_read.c sensor_version.c -o libsensor.so.1.0
$ ln -sf libsensor.so.1.0 libsensor.so.1 && ln -sf libsensor.so.1 libsensor.so
$ gcc main.o -L. -lsensor -o app_dynamic
$ ./app_dynamic
./app_dynamic: error while loading shared libraries: libsensor.so.1: cannot open shared object file: No such file or directory
[exit status 127]
$ readelf -d app_dynamic | grep -E 'NEEDED|RUNPATH'
 0x0000000000000001 (NEEDED)             Shared library: [libsensor.so.1]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
```

The link succeeded and the program still did not start, because two different programs
searched for the library:

- the *link editor* (`ld`) found `libsensor.so` through `-L.` at build time;
- the *dynamic loader* (`ld.so`) looks for the name recorded in `NEEDED` at run time. That
  name is the library's SONAME, `libsensor.so.1`, not the file name passed to the linker.
  The loader does not know about `-L`.

The loader searches `LD_LIBRARY_PATH`, the executable's `RUNPATH`, the `ldconfig` cache,
and the default directories. For an application that ships its own libraries, record a
search path relative to the executable:

```text
$ gcc main.o -L. -lsensor -Wl,-rpath,'$ORIGIN' -o app_dynamic
$ ./app_dynamic
sensor 1.0: 420
$ readelf -d app_dynamic | grep -E 'NEEDED|RUNPATH'
 0x0000000000000001 (NEEDED)             Shared library: [libsensor.so.1]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 0x000000000000001d (RUNPATH)            Library runpath: [$ORIGIN]
$ LD_DEBUG=bindings ./app_dynamic 2>&1 | grep -E "symbol .sensor_(read|version)" | ...
binding file ./app_dynamic [0] to ./libsensor.so.1 [0]: normal symbol `sensor_read'
binding file ./app_dynamic [0] to ./libsensor.so.1 [0]: normal symbol `sensor_version'
```

`LD_DEBUG=bindings` makes glibc's loader report every symbol binding: which object
requested each symbol, and which object supplied it. `LD_DEBUG=help` lists the other
categories (`libs` shows the search, `versions` the symbol-version checks). It is the
direct way to answer "which library did this call actually go to?"

Dynamic linking can reduce deployed duplication and allow library updates, but it introduces search paths, version compatibility, loader availability, relocation policy, and supply-chain concerns. A Linux process and a microcontroller firmware image should not be discussed as if they share the same loading model.

## Library Search And Link Order

The compiler driver translates options such as `-lprotocol` and `-Ldir` into linker inputs. Search paths and library ordering are part of the build contract:

~~~sh
cc app.o -Lbuild/lib -Wl,-rpath,'$ORIGIN/../lib' -lprotocol -o app
~~~

The shell quoting above is intentional so `$ORIGIN` reaches the linker. Avoid embedding a developer’s absolute path in a release binary. Prefer a controlled sysroot, explicit dependency directories, and a documented runtime search policy.

For static archives, a library generally satisfies unresolved references that exist when it is encountered. Thus:

~~~sh
cc main.o -lconsumer -lprovider -o app
~~~

can differ from the reversed order if `consumer` needs definitions from `provider`. Use dependency-aware build rules and group options only when a genuine cyclic archive dependency exists.

The simplest case, an archive placed before the object that needs it:

```text
$ ar rcs libsensor.a sensor_read.o sensor_version.o sensor_debug.o
$ gcc -L. -lsensor main.o -o app_static
/usr/bin/ld: main.o: in function `main':
./main.c:6:(.text.startup+0x6): undefined reference to `sensor_read'
/usr/bin/ld: ./main.c:6:(.text.startup+0xd): undefined reference to `sensor_version'
collect2: error: ld returned 1 exit status
[exit status 1]
$ gcc main.o -L. -lsensor -Wl,-Map=app_static.map -o app_static
$ ./app_static
sensor 1.0: 420
$ sed -n '1,6p' app_static.map
Archive member included to satisfy reference by file (symbol)

./libsensor.a(sensor_read.o)  main.o (sensor_read)
./libsensor.a(sensor_version.o)
                              main.o (sensor_version)
```

When `ld` reached `-lsensor` first, no undefined symbols were pending yet, so it took
nothing from the archive and never went back to it. The map file's first section lists
each archive member that was pulled in and the reference that pulled it. `sensor_debug.o`
is not listed: an archive member that resolves nothing is not linked at all.

## Startup Objects And Runtime Libraries

The driver may add files and libraries that are not visible in the source command:

- `crt1.o`, `crti.o`, `crtn.o`, or target equivalents for process startup and termination sections;
- `libgcc` or another compiler runtime for division, wide arithmetic, atomics, unwinding, or built-ins;
- libc and platform libraries;
- dynamic loader metadata or static initialization support.

Reveal driver decisions:

~~~sh
cc -v app.o -o app
cc -### app.o -o app
cc -print-file-name=libgcc.a
cc -print-file-name=crt1.o
~~~

Do not use `-nostdlib`, `-nodefaultlibs`, or `-nostartfiles` merely to make a linker command shorter. If you remove defaults, you take responsibility for every startup and runtime contract they provided.

## Dead-Code Elimination

Compile each function and data object into its own section, then ask the linker to remove unreferenced sections:

~~~sh
cc -ffunction-sections -fdata-sections -c driver.c -o driver.o
cc -Wl,--gc-sections driver.o -o app
~~~

The same code linked three ways:

```text
$ gcc main.o sensor_read.o sensor_version.o sensor_debug.o -o app_objects
$ gcc main.o gc_sensor_read.o gc_sensor_version.o gc_sensor_debug.o -Wl,--gc-sections -Wl,--print-gc-sections -o app_gc
...
/usr/bin/ld: removing unused section '.text.sensor_scale' in file 'gc_sensor_read.o'
/usr/bin/ld: removing unused section '.rodata.sensor_debug_dump.str1.1' in file 'gc_sensor_debug.o'
/usr/bin/ld: removing unused section '.text.sensor_debug_dump' in file 'gc_sensor_debug.o'
/usr/bin/ld: removing unused section '.rodata.sensor_calibration_dump' in file 'gc_sensor_debug.o'
$ size app_objects app_static app_gc
   text	   data	    bss	    dec	    hex	filename
   5963	    608	     16	   6587	   19bb	app_objects
   1570	    600	      8	   2178	    882	app_static
   1526	    592	      8	   2126	    84e	app_gc
```

- Objects named on the command line are always linked whole, including the 4 KB
  calibration table nobody uses (`app_objects`).
- The archive dropped `sensor_debug.o` because nothing referenced it, but it can only
  drop whole members (`app_static`).
- `--gc-sections` works per section, so it can also remove the out-of-line copy of
  `sensor_scale`, which became unused once `-O2` inlined it into `sensor_read`
  (`app_gc`). `--print-gc-sections` lists every removal, which makes a useful CI
  artifact to review.

This can reduce firmware size, but reachability is not always visible to the linker. Preserve:

- interrupt vector entries;
- linker-collected registration tables;
- bootloader entry points;
- symbols referenced by a debugger, script, or external protocol;
- metadata consumed after programming;
- functions discovered through dynamic lookup or assembly.

Use linker `KEEP`, `used`/retention attributes, explicit references, or post-link checks according to the actual contract. A smaller image that has lost a fault handler is not an optimization success.

## Symbol Export And Interposition

In a shared library built with default options, every function and object without
`static` is exported, including internal helpers:

```text
$ nm -D --defined-only libsensor.so.1.0 | grep sensor
0000000000001130 T sensor_read
0000000000001120 T sensor_scale
0000000000001140 T sensor_version
```

Exported symbols are part of the ABI, and on ELF they can be *interposed*: the first
definition in the loader's search order wins, and the executable comes first. An
application that happens to define its own `sensor_scale` replaces the library's,
including for calls *inside* the library:

```c
--8<-- "examples/c/tool-sessions/linking/main_scale.c:4:8"
```

```text
$ gcc -O2 main_scale.c -L. -lsensor -Wl,-rpath,'$ORIGIN' -o app_scale
$ ./app_scale
sensor 1.0: 42 (application scale of 7: 7)
$ objdump -d --no-show-raw-insn -M intel --disassemble=sensor_read libsensor.so.1.0 | ...
0000000000001130 <sensor_read>:
    1130:	endbr64
    1134:	mov    edi,0x2a
    1139:	jmp    1050 <sensor_scale@plt>
```

The library now returns 42 instead of 420, with no error and no warning. The
disassembly shows why: because `sensor_scale` is exported, the compiler had to call it
through the PLT, where interposition can redirect it, and could not inline it.

Export only the API. Build with `-fvisibility=hidden` and mark the public declarations,
as `SENSOR_API` does in `sensor.h`:

```text
$ gcc -O2 -g -fPIC -shared -fvisibility=hidden -Wl,-soname,libsensor.so.1 sensor_read.c sensor_version.c -o libsensor.so.1.0
$ nm -D --defined-only libsensor.so.1.0 | grep sensor
0000000000001110 T sensor_read
0000000000001120 T sensor_version
$ ./app_scale
sensor 1.0: 420 (application scale of 7: 7)
```

The helper is no longer exported, cannot be interposed, and can be inlined again. A
helper used by only one file should simply be `static`. Visibility control is for
internals that several files of the library share.

## Static Versus Dynamic: Decision Table

| Concern | Static | Dynamic |
| --- | --- | --- |
| Deployment dependencies | mostly self-contained | loader and shared objects required |
| Update granularity | rebuild/redeploy application | library can often update independently |
| Firmware predictability | usually strong | uncommon on small MCUs |
| Memory sharing | duplicates code per image/process | shared pages possible |
| Failure timing | usually build-time | build, load, or first-call time |
| Attack surface | fewer loader paths | loader, search path, relocations, interposition |
| ABI compatibility | link-time checked | runtime compatibility matters |
| Debugging | symbols in one image | exact executable and all shared objects |
| Typical embedded use | bare metal, RTOS, firmware | embedded Linux processes, plugins |

Choose based on update architecture, memory protection, recovery, licensing, field diagnostics, and ABI governance—not only binary size.

## ABI Compatibility And Versioning

A shared library can preserve symbol names while breaking callers by changing:

- structure size or field offsets;
- enum representation or integer width assumptions;
- calling convention or alignment;
- ownership, lifetime, or thread-safety behavior;
- error values and initialization requirements.

For public binary interfaces, use opaque handles, fixed-width types where appropriate, explicit version fields, size-tagged structures, and compatibility tests. Keep private symbols local and publish a symbol version policy for hosted products.

## Exercises

1. Build the same three-module application as a normal dynamically linked executable and a static executable.
2. Use `-v` and `-###` to identify hidden startup and runtime inputs.
3. Run `examples/c/tool-sessions/linking/run.sh`, then make `sensor_version.o` depend on
   a function in a second archive and find a link order that works without
   `--start-group`.
4. Add `-flto` to the `app_gc` build and compare `size` output with the three variants above.
5. Add an intentionally unreferenced vector-table entry and retain it correctly.
6. Inspect `PT_INTERP`, `DT_NEEDED`, and relocation entries in a shared-object executable.
7. Change a public structure layout, rebuild only the library, and demonstrate the ABI failure with a compatibility test.

## Common Mistakes

- calling a static archive a complete library without considering archive extraction;
- assuming `-static` removes startup or compiler runtime dependencies;
- relying on `ldd` output as a complete security analysis;
- embedding accidental absolute search paths;
- changing archive order to “fix” a link without documenting dependencies;
- shipping a shared library that exports every non-static helper, so applications can
  interpose them by accident;
- confusing link-time search (`-L`) with run-time search (`RUNPATH`, `LD_LIBRARY_PATH`);
- enabling section garbage collection without retaining external entry points;
- treating shared-library symbol compatibility as source compatibility;
- assuming static linking always produces a smaller or safer image;
- removing default libraries without replacing their runtime support;
- forgetting that dynamic relocation and loader behavior affect startup and memory permissions.

## Related Topics

- [Object Files, Symbols, And Relocations](./object-files-symbols-and-relocations.md)
- [Linker Scripts And Memory Layout](./linker-scripts-and-memory-layout.md)
- [Startup, Runtime, And `main`](./startup-runtime-and-main.md)
- [ABI, Calling Conventions, And FFI](./abi-calling-conventions-and-ffi.md)
- [Embedded libc Implementations](../standard-library-and-ecosystem/embedded-libc.md)

## References

- [GCC link options](https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)
- [GNU ld overview](https://sourceware.org/binutils/docs/ld/Overview.html)
- [GNU ld options](https://sourceware.org/binutils/docs/ld/Options.html)
- [ELF dynamic linking](https://refspecs.linuxfoundation.org/elf/gabi4%2B/ch5.dynamic.html)
- [ELF program loading and dynamic linking](https://refspecs.linuxfoundation.org/elf/gabi4%2B/ch5.intro.html)
