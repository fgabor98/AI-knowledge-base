---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Build Artifact Debugging

## What Problem Does This Solve?

When a binary fails on target, the build system may be innocent. The artifact may have the wrong architecture, interpreter, ABI, RPATH, dependency list, or debug information. This page collects the first inspection commands an embedded engineer should run.

## Core Tools

- `file`
- `readelf`
- `objdump`
- `nm`
- `strings`
- `ldd`
- `patchelf`
- `eu-readelf`
- `sha256sum`

## First Five Checks

```bash
file ./app
readelf -h ./app
readelf -l ./app | grep interpreter
objdump -p ./app | grep NEEDED
readelf -d ./app | egrep 'RPATH|RUNPATH|SONAME'
```

These checks answer:

- what architecture is it?
- is it dynamically linked?
- what runtime loader does it expect?
- which shared libraries does it need?
- does it carry suspicious runtime search paths?

## Architecture Mismatch

Symptoms:

- `Exec format error`
- binary works on host but not target
- target loader rejects the file

Check:

```bash
file app
readelf -h app | grep Machine
```

## Missing Dynamic Loader

Symptom:

```text
No such file or directory
```

even though the executable exists.

Check:

```bash
readelf -l app | grep interpreter
ls -l /lib/ld-linux*
```

The interpreter path is hardcoded in the ELF binary.

## Symbol Problems

Check exported and undefined symbols:

```bash
nm -D libfoo.so
readelf -Ws app | grep UND
```

Use this when a binary starts but fails with `undefined symbol`.

## Provenance Checks

For release artifacts:

```bash
sha256sum app libfoo.so image.wic
strings app | grep -i version
```

Checksums and embedded version strings help prove what was deployed.

## Common Mistakes

- running host `ldd` on a target binary and trusting the output blindly
- overlooking the ELF interpreter path
- shipping a library without the SONAME symlink
- stripping all symbols before saving debug artifacts
- debugging source code before checking the actual deployed binary

## Related Topics

- [Shared Libraries, ABI, and Runtime Linking](shared-libraries-abi-and-runtime-linking.md)
- [Object Files and Linking](object-files-and-linking.md)
- [Cross-Compilation](cross-compilation.md)

