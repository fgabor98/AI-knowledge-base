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

Embedded Linux systems fail in the field when a binary was built against one ABI but deployed with another runtime library set. This page explains static libraries, shared libraries, symbol resolution, SONAMEs, runtime loader behavior, and the checks needed before a binary enters a root filesystem.

## Core Concepts

- static library
- shared library
- ABI
- API
- ELF
- SONAME
- dynamic loader
- `rpath`
- `runpath`
- symbol visibility
- sysroot
- target root filesystem

## Mental Model

```text
source
-> object files
-> static or shared library
-> linked executable
-> runtime loader
-> target rootfs libraries
```

The linker proves that the binary can be produced. The target dynamic loader proves that the binary can start on the device. Those are related but not the same check.

## Static Vs Shared

A static library is an archive of object files:

```bash
ar rcs libsensor.a sensor.o protocol.o
```

A shared library is an ELF object loaded at runtime:

```bash
gcc -fPIC -c sensor.c
gcc -shared -Wl,-soname,libsensor.so.1 -o libsensor.so.1.0 sensor.o
```

Static linking simplifies deployment for small tools, but increases binary size and can complicate license obligations. Shared linking reduces duplication and supports updates, but requires correct runtime library deployment.

## Runtime Loader Checks

Use target-aware tools when possible:

```bash
file app
readelf -l app | grep 'interpreter'
readelf -d app
readelf -Ws libsensor.so
objdump -p app | grep NEEDED
```

Important questions:

- Is the binary for the correct architecture?
- Is the dynamic interpreter path valid on the target?
- Are all `NEEDED` libraries present?
- Does the deployed library provide the expected SONAME?
- Is the binary accidentally using host paths?

## Embedded Linux Failure Patterns

Common failures:

- `No such file or directory` even though the binary exists because the dynamic loader path is missing.
- `not found` from `ldd` because a shared library is absent from the rootfs.
- `wrong ELF class` because a host library was copied into a target sysroot.
- `undefined symbol` because runtime library ABI is older than build-time ABI.
- program starts in the SDK sysroot but fails in the actual image.

## Practical Checklist

Before deploying a binary:

- run `file`
- inspect `NEEDED`
- inspect dynamic loader path
- check architecture and ABI
- check for absolute build paths
- confirm libraries are packaged into the image
- confirm SDK sysroot and image rootfs agree

## Related Topics

- [Object Files and Linking](object-files-and-linking.md)
- [Target Triples and Sysroots](target-triples-and-sysroots.md)
- [Build Artifact Debugging](build-artifact-debugging.md)

