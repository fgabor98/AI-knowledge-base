---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Stage 1: Linux Runtime, Filesystem, And Rootfs

Learn the runtime filesystem and rootfs assumptions that userspace applications depend on during boot and operation.

This stage is a collection of focused draft pages. Read the overview first, then study the leaf pages in order while extending one small C utility or service.

## Learning Materials

1. [Filesystem Hierarchy And Path Resolution](filesystem-hierarchy-and-path-resolution.md)
2. [Files, Inodes, Metadata, And Links](files-inodes-metadata-and-links.md)
3. [Safe Paths And Temporary File Operations](safe-path-and-temporary-file-operations.md)
4. [Pseudo-filesystems And Device Nodes](pseudo-filesystems-and-device-nodes.md)
5. [Mounts, Initramfs, And Rootfs Layout](mounts-initramfs-and-rootfs-layout.md)
6. [Read-Only Rootfs, Overlayfs, And Persistent State](read-only-rootfs-overlayfs-and-persistent-state.md)
7. [ELF Executables And Dynamic Linking](elf-executables-and-dynamic-linking.md)

## Study Pattern

For each page:

1. Read the contract and identify the libc, POSIX, Linux, kernel UAPI, or init-system layer.
2. Implement the smallest host-side example.
3. Add error, timeout, ownership, and cleanup paths.
4. Observe the result with the relevant Linux tools.
5. Repeat on the target and record differences.
6. Integrate the mechanism into the running capstone service.

## Stage Outcomes

By the end of this stage, you should be able to:

- explain and demonstrate filesystem hierarchy and path resolution;
- explain and demonstrate files, inodes, metadata, and links;
- explain and demonstrate safe paths and temporary file operations;
- explain and demonstrate pseudo-filesystems and device nodes;
- explain and demonstrate mounts, initramfs, and rootfs layout;
- connect the mechanism to an embedded Linux failure, test, or service-design decision;
- produce evidence that distinguishes application, kernel, deployment, and hardware causes.

## Completion Criteria

- The examples compile with warnings and debug information.
- Normal, interrupted, missing-resource, and teardown paths are tested.
- Resource ownership and target assumptions are documented.
- At least one failure has been diagnosed using observable evidence.
- The work is linked to the next stage or an existing capstone.

## Related Topics

- [Linux Userspace And System Programming](../index.md)
- [C Programming](../../c/index.md)
- [Linux Kernel Programming](../../linux-kernel/index.md)
- [Embedded Linux](../../embedded-linux/index.md)
