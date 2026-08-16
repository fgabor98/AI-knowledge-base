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

This can reduce firmware size, but reachability is not always visible to the linker. Preserve:

- interrupt vector entries;
- linker-collected registration tables;
- bootloader entry points;
- symbols referenced by a debugger, script, or external protocol;
- metadata consumed after programming;
- functions discovered through dynamic lookup or assembly.

Use linker `KEEP`, `used`/retention attributes, explicit references, or post-link checks according to the actual contract. A smaller image that has lost a fault handler is not an optimization success.

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
3. Change archive order and explain the link result.
4. Compare image size with and without function/data section garbage collection.
5. Add an intentionally unreferenced vector-table entry and retain it correctly.
6. Inspect `PT_INTERP`, `DT_NEEDED`, and relocation entries in a shared-object executable.
7. Change a public structure layout, rebuild only the library, and demonstrate the ABI failure with a compatibility test.

## Common Mistakes

- calling a static archive a complete library without considering archive extraction;
- assuming `-static` removes startup or compiler runtime dependencies;
- relying on `ldd` output as a complete security analysis;
- embedding accidental absolute search paths;
- changing archive order to “fix” a link without documenting dependencies;
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
