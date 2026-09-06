---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# ELF Executables And Dynamic Linking

## What problem does this solve?

An executable can be present and executable by mode bits yet fail before `main`.
The kernel may reject its architecture, the requested ELF interpreter may be absent,
or the interpreter may fail to load a needed shared object or symbol version. Inspect
the binary’s deployment contract instead of guessing from the error string.

## ELF at runtime

An ELF executable contains headers describing the machine, class, entry point, and
program segments. The program headers—not the section table—tell the kernel and
loader what must be mapped. A dynamically linked executable contains a `PT_INTERP`
segment naming its program interpreter, such as an architecture-specific `ld-linux`
path. The kernel starts that interpreter, which maps the executable’s dependencies,
performs relocations, and transfers control to the program startup code.

```text
execve(path)
  |
  +-- kernel validates ELF and maps interpreter
  +-- interpreter loads executable and DT_NEEDED libraries
  +-- relocations, TLS, constructors, and libc startup
  +-- main(argc, argv, envp)
```

The interpreter is a runtime dependency. If it is missing, `execve` can report
`ENOENT` even though the application file exists.

## Inspect the artifact

```sh
file ./app
readelf -hW ./app
readelf -lW ./app
readelf -dW ./app
readelf --version-info ./app
objdump -p ./app | sed -n '/INTERP\|NEEDED\|RPATH\|RUNPATH/p'
```

Record:

- ELF class (32 or 64 bit), endianness, machine, and ABI flags;
- requested interpreter;
- `DT_NEEDED` shared libraries;
- RPATH/RUNPATH and default loader paths;
- symbol versions required by the executable and libraries;
- PIE/RELRO/NX-related program properties where security policy requires them.

`ldd` is convenient for trusted artifacts, but it is loader-assisted inspection and
must not be used on an untrusted object as a sandbox. Use `readelf`, a target-rootfs
loader, or a controlled execution environment for release diagnosis.

## Library lookup

The dynamic loader searches according to the object’s dependency metadata, loader
configuration, environment, and architecture-specific defaults. `LD_LIBRARY_PATH`
and `LD_PRELOAD` are useful for controlled debugging but dangerous for set-user-ID
programs and inappropriate as hidden service configuration. Prefer packaged libraries
in known target paths and explicit RPATH/RUNPATH policy when justified.

```sh
LD_DEBUG=libs,versions ./app 2>/tmp/loader.log   # glibc diagnostic; use carefully
readelf -dW ./app | grep -E 'NEEDED|RPATH|RUNPATH'
```

The target may use a different libc, loader, or library directory convention. A
glibc-built application is not automatically compatible with musl, and a newer
symbol version is not automatically available on an older target.

## Static and dynamic linking

| Choice | Benefits | Costs |
| --- | --- | --- |
| Dynamic | Shared storage, smaller individual binaries, libc/security updates | Loader and library dependencies, ABI deployment, startup diagnostics |
| Static | Fewer runtime library files, simpler single-file deployment | Larger image, duplicated code, libc licensing/feature limits, NSS/plugin caveats |

Static linking does not remove kernel or UAPI dependencies and does not make a
program portable across architectures. It can also make security updates harder if
many applications contain their own old copy of a library.

## Scripts and interpreters

A script’s shebang is an interpreter dependency too:

```text
#!/bin/sh
```

The interpreter path must exist in the target rootfs. Environment lookup with
`#!/usr/bin/env python3` adds a `PATH` dependency and may be unsuitable for a tightly
controlled service. Verify executable mode, line endings, interpreter path, and the
service’s environment.

## ABI and UAPI are different

The ELF ABI makes the process executable and lets functions cross library boundaries.
A device UAPI adds its own structure sizes, alignment, ioctl numbers, endianness,
pointer rules, and versioning. A binary can load correctly and still be incompatible
with the kernel driver or device firmware.

## Deployment diagnostic sequence

```sh
sha256sum ./app
file ./app
readelf -lW ./app | grep INTERP
readelf -dW ./app | grep NEEDED
readelf --version-info ./app
# On a target with the program installed:
strace -f -e trace=execve,openat ./app
cat /proc/$(pidof app)/maps 2>/dev/null
```

If startup fails before a PID exists, inspect the loader and service-manager log.
Compare the exact binary checksum, rootfs package revision, interpreter path, and
library symbol versions.

## Common mistakes

- Interpreting `ENOENT` as proof that the application pathname is absent.
- Copying a host libc or loader into the target image ad hoc.
- Assuming `ldd` is safe for untrusted files.
- Using `LD_LIBRARY_PATH` to hide a packaging error.
- Treating static linking as a complete portability or security solution.
- Ignoring symbol versions, architecture-specific loader names, or script shebangs.
- Stripping the only copy of debug symbols and build identity.

## Debugging checklist

- Check mode, architecture, ELF class, interpreter, and target triple.
- Check `DT_NEEDED`, RPATH/RUNPATH, loader defaults, and symbol versions.
- Check target libc, rootfs packages, environment, and service sandbox.
- Capture loader diagnostics without leaking secrets into persistent logs.
- Check UAPI structure layout separately from executable compatibility.
- Preserve exact binary, symbols, compiler, linker, and rootfs identities.

## Related topics

- [Stage 1: Linux Runtime, Filesystem, And Rootfs](index.md)
- [Host, Target, ABI, And Rootfs Lab](../environment-and-mental-model/host-target-abi-and-rootfs-lab.md)
- [Target Triples, Sysroots, And ABI](../../build-systems/target-triples-and-sysroots.md)
- [Shared Libraries, ABI, And Runtime Linking](../../build-systems/shared-libraries-abi-and-runtime-linking.md)

## References

- [`execve(2)`](https://man7.org/linux/man-pages/man2/execve.2.html)
- [`ld.so(8)`](https://man7.org/linux/man-pages/man8/ld.so.8.html)
- [ELF specification](https://refspecs.linuxfoundation.org/elf/gabi4+/contents.html)
- [GNU binutils `readelf`](https://sourceware.org/binutils/docs/binutils/readelf.html)
