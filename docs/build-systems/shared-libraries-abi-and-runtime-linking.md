---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Shared Libraries, ABI, And Runtime Linking

## What Problem Does This Solve?

Embedded Linux systems fail in the field when a binary was built against one set of libraries and deployed with another. The link on the build machine succeeds. Then the target says `cannot open shared object file`, `version 'GLIBC_2.38' not found`, or `undefined symbol`, or, worse, it loads a different library than you tested.

This page explains how a shared library is named, versioned, found, and loaded. It shows the checks that prove a binary will start on the target before it goes into a root filesystem. The basics of static archives, `-fPIC`, SONAME, `$ORIGIN`, and symbol export are covered with transcripts in [Static and Dynamic Linking](../c/compilation-linking-and-abi/static-and-dynamic-linking.md). This page builds on them.

## Core Concepts

- real name, SONAME, and linker name
- `-dev` versus runtime packages
- `DT_NEEDED`, `DT_SONAME`, `DT_RPATH`, `DT_RUNPATH`
- the dynamic loader (`ld-linux-*.so`, `PT_INTERP`)
- loader search order and `ldconfig`
- symbol versioning and version scripts
- glibc symbol versions (`GLIBC_2.xx`)
- `--as-needed`, `-z defs`
- position-independent executables
- ABI versus API

## Mental Model

Two different programs search for libraries, at two different times, using different rules:

```text
build host                                   target device
----------                                   -------------
ld (link editor)                             ld-linux (dynamic loader)
  finds libfoo.so via -L and the sysroot       finds libfoo.so.1 via RPATH, LD_LIBRARY_PATH,
  reads its SONAME: libfoo.so.1                RUNPATH, ld.so.cache, default dirs
  records DT_NEEDED: libfoo.so.1               checks every required symbol version
  records required symbol versions             binds symbols, runs the program
```

The link proves that the binary can be produced against the libraries in the **sysroot**. Only the loader proves that it can start against the libraries in the **root filesystem**. Every embedded shared-library bug is a difference between those two sets.

## Syntax / API / Mechanism

All transcripts on this page come from `bash examples/build-systems/tool-sessions/shared-libraries/run.sh` (GCC 13.3, binutils 2.42, glibc 2.39, x86-64). The commands are the same with a cross toolchain, except that the program cannot run on the build host.

### Three Names For One Library

A shared library has three names, each used by a different party:

| Name | Example | Used by | Shipped in |
| --- | --- | --- | --- |
| real name | `libsensor.so.1.0.0` | the file itself | runtime package |
| SONAME | `libsensor.so.1` | the loader, through `DT_NEEDED` | runtime package (symlink) |
| linker name | `libsensor.so` | `ld`, when you write `-lsensor` | `-dev` package (symlink) |

```text
$ gcc -O2 -fPIC -shared -Wl,-soname,libsensor.so.1 -Wl,--version-script=libsensor-1.0.map sensor_v1.c -o libsensor.so.1.0.0
$ readelf -d libsensor.so.1.0.0 | grep SONAME
 0x000000000000000e (SONAME)             Library soname: [libsensor.so.1]
$ gcc app_v1.c -L. -lsensor -o app_v1
/usr/bin/ld: cannot find -lsensor: No such file or directory
collect2: error: ld returned 1 exit status
[exit status 1]
$ ldconfig -n . && ln -sf libsensor.so.1 libsensor.so && ls -l libsensor.so* | awk '{print $9, $10, $11}'
libsensor.so -> libsensor.so.1
libsensor.so.1 -> libsensor.so.1.0.0
libsensor.so.1.0.0
$ gcc app_v1.c -L. -lsensor -Wl,-rpath,'$ORIGIN' -o app_v1 && readelf -d app_v1 | grep NEEDED
 0x0000000000000001 (NEEDED)             Shared library: [libsensor.so.1]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
```

- `-lsensor` failed while only the real name existed: `ld` looks for `libsensor.so` (or `libsensor.a`). In a sysroot this exact error means the library's **`-dev` package** is missing. The runtime package alone is not enough to link against.
- `ldconfig -n .` created the SONAME symlink from the SONAME stored in the file. On a target, `ldconfig` (run by the package manager or at image creation) does the same for system library directories and rebuilds `/etc/ld.so.cache`.
- The executable records the **SONAME**, not the file name passed to the linker. The loader only needs `libsensor.so.1` on the target.

Bump the SONAME (`libsensor.so.2`) only when you break the ABI: removed functions, changed structure layouts, changed semantics. Compatible additions keep the SONAME and add a symbol version.

### Symbol Versions

A version script both limits exported symbols (`local: *;`) and assigns them to version nodes. Version 1.1 of the library adds a function in a new node:

```text
--8<-- "examples/build-systems/tool-sessions/shared-libraries/libsensor-1.1.map"
```

An application linked against 1.1 records which version of each symbol it needs:

```text
$ gcc -O2 -fPIC -shared -Wl,-soname,libsensor.so.1 -Wl,--version-script=libsensor-1.1.map sensor_v2.c -o libsensor.so.1.1.0
$ ln -sf libsensor.so.1.1.0 libsensor.so.1 && gcc app.c -L. -lsensor -Wl,-rpath,'$ORIGIN' -o app && ./app
read 420, average 421
$ objdump -T app | grep sensor
0000000000000000      DF *UND*	0000000000000000 (LIBSENSOR_1.1) sensor_read_average
0000000000000000      DF *UND*	0000000000000000 (LIBSENSOR_1.0) sensor_read
$ readelf -V app | sed -n '/.gnu.version_r/,$p' | grep -E 'File|Name'
  000000: Version: 1  File: libsensor.so.1  Cnt: 2
  0x0010:   Name: LIBSENSOR_1.0  Flags: none  Version: 5
  0x0020:   Name: LIBSENSOR_1.1  Flags: none  Version: 3
  0x0030: Version: 1  File: libc.so.6  Cnt: 2
  0x0040:   Name: GLIBC_2.2.5  Flags: none  Version: 4
  0x0050:   Name: GLIBC_2.34  Flags: none  Version: 2
```

Now deploy the old library, as happens when an application update ships without its library update:

```text
$ ln -sf libsensor.so.1.0.0 libsensor.so.1 && ./app
./app: ./libsensor.so.1: version `LIBSENSOR_1.1' not found (required by ./app)
[exit status 1]
$ ./app_v1
read 420
```

The SONAME matched, so without symbol versions the program would have started and crashed at the first call to `sensor_read_average` (`symbol lookup error`). With versions, the loader refuses to start it and names the missing version. The older application still runs against the older library.

### The Same Check Against glibc

glibc versions every symbol. The newest `GLIBC_` version a binary needs is the oldest glibc it can run on:

```text
$ objdump -T app | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -n 1
GLIBC_2.34
```

This is the cause of the most common embedded deployment failure: a binary built with the **host** compiler or a **newer SDK** than the image is copied to the target, and the loader reports `version 'GLIBC_2.3x' not found`. Do not "fix" this by copying a newer libc onto the target. Build with the toolchain and sysroot that match the image: the image's SDK, or the recipe itself.

### Where The Loader Looks

For each `DT_NEEDED` entry without a slash, glibc's loader searches in this order:

1. `DT_RPATH` of the executable, **only if it has no `DT_RUNPATH`**
2. `LD_LIBRARY_PATH` (ignored for setuid programs)
3. `DT_RUNPATH` of the object that needs the library
4. `/etc/ld.so.cache`, built by `ldconfig` from `/etc/ld.so.conf`
5. the default directories (`/lib`, `/usr/lib`, or the multilib/multiarch equivalents)

Modern linkers write `DT_RUNPATH` for `-rpath`. `--disable-new-dtags` writes the legacy `DT_RPATH`. The difference is whether `LD_LIBRARY_PATH` can override it:

```text
$ gcc which_app.c -Lv1 -lwhich -Wl,-rpath,'$ORIGIN/v1' -o app_runpath && readelf -d app_runpath | grep -E 'RPATH|RUNPATH'
 0x000000000000001d (RUNPATH)            Library runpath: [$ORIGIN/v1]
$ gcc which_app.c -Lv1 -lwhich -Wl,-rpath,'$ORIGIN/v1' -Wl,--disable-new-dtags -o app_rpath && readelf -d app_rpath | grep -E 'RPATH|RUNPATH'
 0x000000000000000f (RPATH)              Library rpath: [$ORIGIN/v1]
$ ./app_runpath && LD_LIBRARY_PATH=$PWD/v2 ./app_runpath
loaded libwhich from v1
loaded libwhich from v2
$ LD_LIBRARY_PATH=$PWD/v2 ./app_rpath
loaded libwhich from v1
```

`LD_DEBUG=libs` shows the actual search, which is the fastest way to answer "which file did it load, and why":

```text
$ LD_DEBUG=libs LD_LIBRARY_PATH=$PWD/v2 ./app_rpath 2>&1 | grep -E 'find library=libwhich|search path=|trying file=.*libwhich' | sed -E 's/^[[:space:]]+[0-9]+:[[:space:]]+//' | head -n 4
find library=libwhich.so [0]; searching
search path=./v1/glibc-hwcaps/x86-64-v3:./v1/glibc-hwcaps/x86-64-v2:./v1		(RPATH from file ./app_rpath)
trying file=./v1/glibc-hwcaps/x86-64-v3/libwhich.so
trying file=./v1/glibc-hwcaps/x86-64-v2/libwhich.so
```

Policy for embedded images:

- Install libraries into the standard directories of the image. No `RPATH`/`RUNPATH` is then needed, and none should be present: Yocto's `rpaths` QA check fails the build for an RPATH that points into the build tree, and `useless-rpaths` flags ones that point at default directories.
- Use `$ORIGIN`-relative `RUNPATH` only for self-contained application bundles (for example under `/opt/vendor-app/lib`).
- Do not put `LD_LIBRARY_PATH` in service files to paper over a packaging problem.
- `LD_DEBUG` is available on the target with glibc. musl's loader does not support it.

### `--as-needed`: Record Only What Is Used

```text
$ gcc plain.c -lm -o plain_default && readelf -d plain_default | grep NEEDED
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
$ gcc plain.c -Wl,--no-as-needed -lm -o plain_all && readelf -d plain_all | grep NEEDED
 0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
```

Ubuntu's GCC passes `--as-needed` by default. Yocto adds `-Wl,--as-needed` to `TARGET_LDFLAGS` (`ASNEEDED` in `bitbake.conf`). Other toolchains may not. With `--as-needed`, unused libraries do not become runtime dependencies, so they also do not become package dependencies. The flip side: order matters, because a library listed **before** the objects that use it is considered unused. Libraries belong in `LDLIBS`, after the objects.

### `-z defs`: Catch Missing Libraries When Building The Library

By default, a shared library may contain undefined symbols, which are expected to be resolved at load time:

```text
$ gcc -fPIC -c needs_log.c -o needs_log.o && gcc -shared needs_log.o -o libscale.so && echo linked
linked
$ gcc -shared -Wl,-z,defs needs_log.o -o libscale.so
/usr/bin/ld: needs_log.o: in function `scaled_log':
needs_log.c:(.text+0x1b): undefined reference to `log'
collect2: error: ld returned 1 exit status
[exit status 1]
$ gcc -shared -Wl,-z,defs needs_log.o -lm -o libscale.so && readelf -d libscale.so | grep NEEDED
 0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
```

The first library "works" only if every program that loads it happens to link `libm` itself. Otherwise the result is a runtime `undefined symbol: log`. Build libraries with `-Wl,-z,defs` (CMake: add it to the shared-library link flags; Meson does it by default with `b_lundef=true`) so every library declares its own dependencies in `DT_NEEDED`.

### Position-Independent Executables

```text
$ file app | cut -d, -f1-2 && readelf -h app | grep Type
app: ELF 64-bit LSB pie executable, x86-64
  Type:                              DYN (Position-Independent Executable file)
$ gcc -no-pie plain.c -o plain_nopie && readelf -h plain_nopie | grep Type
  Type:                              EXEC (Executable file)
```

Most distributions build PIE by default; Yocto does it by configuring GCC with `--enable-default-pie` (`GCCPIE` in `security_flags.inc`) so that ASLR can randomize the program's address. A PIE is technically a shared object with an entry point. Old tools that check for `EXEC` can misreport it as a library.

## API Versus ABI

The **API** is what source code compiles against: names, types, and semantics in headers. The **ABI** is what a compiled binary depends on: symbol names and versions, calling convention, structure sizes and layouts, enum values, alignment, and the SONAME. A change can keep the API and break the ABI:

| Change | API | ABI | Action |
| --- | --- | --- | --- |
| add a new function | compatible | compatible | new symbol version node |
| add a field at the end of a public struct the caller allocates | compatible | **broken** | bump SONAME, or use opaque handles |
| reorder enum values | compatible | **broken** | bump SONAME |
| change `int` parameter to `long` | mostly compatible | **broken** | new versioned symbol or SONAME bump |
| make an internal helper `static` or hidden | compatible | compatible if never exported | prefer from day one |

Tools such as `abidiff` (libabigail) compare two builds of a library and report ABI changes; run it in CI for libraries that ship separately from their users.

## Embedded Linux Failure Patterns

| Symptom on the target | Meaning | First check |
| --- | --- | --- |
| `./app: No such file or directory` although `ls` shows it | the `PT_INTERP` loader path does not exist (wrong libc or architecture) | `readelf -l app \| grep interpreter` |
| `error while loading shared libraries: libfoo.so.1: cannot open shared object file` | a `NEEDED` library is not in the image or not in the search path | `readelf -d app`, then `find / -name 'libfoo.so*'` |
| `version 'GLIBC_2.3x' not found` | built against a newer glibc than the image | `objdump -T app \| grep GLIBC_` |
| `version 'LIBFOO_1.1' not found` | application newer than the deployed library | `readelf -V app` |
| `symbol lookup error: undefined symbol: foo` | unversioned library is older, or a library lacks its own `NEEDED` | `nm -D --defined-only libfoo.so`, `-z defs` |
| `wrong ELF class: ELFCLASS64` | 64-bit library where a 32-bit one is required, or the reverse | `file` on both |
| program works in the SDK, fails in the image | SDK sysroot and image differ | compare package versions in both manifests |

`ldd` runs the loader, so it only works on binaries the current machine can execute. For target binaries on the build host, use `readelf -d` and `objdump -p`. On the target itself, `ldd` is often missing from minimal images; `LD_TRACE_LOADED_OBJECTS=1 ./app` gives the same output with glibc.

## Practical Checklist

Before a binary goes into an image:

- `file`: correct architecture, ABI, and `dynamically linked, interpreter <path>` for the target libc
- `readelf -d`: every `NEEDED` entry is provided by a package in the image; no `RPATH`/`RUNPATH` pointing into the build tree
- `objdump -T | grep GLIBC_`: the newest required glibc version is not newer than the image's glibc
- `readelf -V`: required versions of your own libraries are provided by the deployed versions
- `strings -a app | grep /home/`: no absolute build paths
- in the image build: the library's runtime package is installed, its `-dev` package is not (unless intended)

In Yocto most of this is automated: package QA checks `arch`, `ldflags`, `rpaths`, and `dev-so`, and the `shlibs` scan turns each `NEEDED` entry into a runtime package dependency. See [Packaging, QA, and Package Feeds](advanced/yocto-openembedded/packaging-qa-and-feeds.md).

## Exercises

1. Run `examples/build-systems/tool-sessions/shared-libraries/run.sh` and explain every failure in the transcript before reading the explanation.
2. Add a `LIBSENSOR_1.2` node with a changed `sensor_read` (same name, new behavior) using `.symver`, and keep old binaries working.
3. Build the session's `app` with a Yocto SDK, copy it to a target with an older image, and record the exact loader error.
4. Run `LD_DEBUG=libs,versions` on the target for one of your applications and save the output as a reference.

## Related Topics

- [Static and Dynamic Linking](../c/compilation-linking-and-abi/static-and-dynamic-linking.md)
- [ABI, Calling Conventions, and FFI](../c/compilation-linking-and-abi/abi-calling-conventions-and-ffi.md)
- [Object Files and Linking](object-files-and-linking.md)
- [Target Triples and Sysroots](target-triples-and-sysroots.md)
- [Build Artifact Debugging](build-artifact-debugging.md)

## References

- [glibc manual: dynamic linker (`ld.so(8)`)](https://man7.org/linux/man-pages/man8/ld.so.8.html)
- [`ldconfig(8)`](https://man7.org/linux/man-pages/man8/ldconfig.8.html)
- [GNU ld: VERSION command (version scripts)](https://sourceware.org/binutils/docs/ld/VERSION.html)
- [GNU ld options: `--as-needed`, `-z defs`, `--enable-new-dtags`](https://sourceware.org/binutils/docs/ld/Options.html)
- Ulrich Drepper, [*How To Write Shared Libraries*](https://akkadia.org/drepper/dsohowto.pdf)
- [libabigail (`abidiff`)](https://sourceware.org/libabigail/)
