---
status: draft
reviewed: false
domain: linux-userspace
difficulty: beginner
last_reviewed: null
---

# Stage 0: Environment And Mental Model

Linux userspace programming is the discipline of asking a process to do work through
contracts exposed by the C library, the Linux kernel, the root filesystem, and the
target product. The first challenge is not memorizing system calls. It is learning
which layer owns a behavior, which assumptions are guaranteed, and which observations
are only accidental properties of one development machine.

This chapter establishes that foundation. Work through the pages in order while
building and observing the repository file
`examples/c/linux-userspace-environment-and-mental-model.c` through the
[chapter lab](#chapter-lab).

## What this chapter solves

It gives you a method for answering questions such as:

- Did the program fail in its own code, in libc, in the kernel, or because the target
  image is missing a runtime dependency?
- Is an interface portable ISO C, POSIX, Linux-specific, a subsystem UAPI, or a
  private product convention?
- Why does a binary compiled on the host fail with “No such file or directory” on the
  target even though the file is visibly present?
- What exactly does a successful `read`, `write`, `malloc`, or `open` promise?
- Who owns a descriptor, buffer, thread, device session, or temporary file, and what
  happens when cancellation, a signal, a timeout, or process exit occurs?

## The core model

Keep these two diagrams in mind. They describe different dimensions of the same
program.

### The execution path

```text
source code
    |
    v
compiler and linker ----> ELF executable and shared-library dependencies
    |
    v
process in user mode
    |
    +--> libc / dynamic loader
    |        |
    |        v
    +--> system-call entry
             |
             v
          kernel subsystem / VFS / scheduler / driver
             |
             v
          hardware, firmware, storage, or network
```

The arrows are not all function calls. The dynamic loader runs before `main`; libc
may implement a function without entering the kernel; a system call may end in a
pure-kernel operation; and a driver may communicate with hardware asynchronously.
`strace` can show many system calls, but it cannot by itself prove what a device did
after the kernel accepted a request.

### The deployment identity

```text
program file
  + ELF class and machine
  + ABI and calling conventions
  + requested program interpreter
  + required shared libraries and symbol versions
  + configuration and environment
  + credentials, limits, namespaces, and capabilities
  + kernel features and UAPI
  + mounted target root filesystem and device state
```

The same source can produce binaries with different deployment identities. A host
run proves only that one identity works in one host environment. It does not prove
that the target has the same architecture, loader, libc, kernel features, mounts,
permissions, clock behavior, or hardware.

## Learning materials

1. [Userspace, Kernel, And Hardware Boundary](userspace-kernel-and-hardware-boundary.md)
2. [POSIX, Linux, libc, And Manual Pages](posix-linux-libc-and-manual-pages.md)
3. [Host, Target, ABI, And Rootfs Lab](host-target-abi-and-rootfs-lab.md)
4. [Blocking, Failure, And Ownership Model](blocking-failure-and-ownership-model.md)

## Vocabulary to keep precise

| Term | Meaning in this chapter |
| --- | --- |
| **Program** | A file containing an executable image, usually an ELF file. |
| **Process** | A running instance with an address space, credentials, file descriptors, signal state, and other resources. |
| **Userspace** | Code executing with restricted privileges in a process address space. |
| **Kernel** | Privileged software that implements scheduling, memory management, filesystems, networking, drivers, and system-call entry points. |
| **libc** | A user-space library implementing ISO C facilities and commonly POSIX/Linux wrappers; it is not the kernel. |
| **ABI** | The binary contract: architecture, calling convention, data layout, object format, loader behavior, and library compatibility. |
| **Rootfs** | The target’s mounted `/` hierarchy and the filesystems mounted below it; it includes programs, libraries, configuration, and often runtime directories. |
| **UAPI** | A deliberately exposed user/kernel interface such as a device node, `ioctl`, sysfs attribute, netlink protocol, or standard subsystem API. |
| **Resource** | Anything with ownership and lifetime: memory, an FD, a mapping, a lock, a thread, a process, a device claim, or persistent state. |

Do not use “the OS” as an explanation when a more precise owner is available. Say
“the dynamic loader could not find the interpreter,” “the kernel returned `EACCES`,”
“the service user lacks permission,” or “the driver reported `ENODEV`.”

## Study method

For each leaf page:

1. Write down the contract layer before reading implementation details.
2. Identify inputs, outputs, side effects, blocking points, failure returns, and
   ownership transfers.
3. Compile the probe with warnings and debug information.
4. Observe it with `file`, `readelf`, `strace`, and `/proc`.
5. Repeat the observations on the target or a target-like rootfs when possible.
6. Record differences as evidence. Do not silently “fix” a difference by changing
   the host until you know which assumption changed.

## Chapter lab

Build and run from the repository root:

```sh
mkdir -p /tmp/linux-userspace-mental-model
cc -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g \
    examples/c/linux-userspace-environment-and-mental-model.c \
    -o /tmp/linux-userspace-mental-model/probe

file /tmp/linux-userspace-mental-model/probe
readelf -lW /tmp/linux-userspace-mental-model/probe | grep 'Requesting program interpreter'
/tmp/linux-userspace-mental-model/probe
strace -f -o /tmp/linux-userspace-mental-model/trace.log \
    /tmp/linux-userspace-mental-model/probe
sed -n '1,80p' /tmp/linux-userspace-mental-model/trace.log
```

While the probe is running, inspect the process from another terminal if it waits
long enough for you to do so:

```sh
/tmp/linux-userspace-mental-model/probe & probe_pid=$!
cat "/proc/$probe_pid/status"
ls -l "/proc/$probe_pid/fd"
readlink "/proc/$probe_pid/exe"
readlink "/proc/$probe_pid/ns/mnt"
wait "$probe_pid"
```

The probe is intentionally small. It prints identity facts and opens `/dev/null`; it
does not pretend that a host can emulate a board driver. Use it to learn the shape of
the evidence, then extend the same observation method to the real service.

## Completion criteria

You have completed this chapter when you can:

- draw the path from a C call to a kernel subsystem and state where the proof stops;
- classify an interface as ISO C, libc, POSIX, Linux, subsystem UAPI, or product API;
- explain an ELF interpreter or shared-library failure using inspected evidence;
- produce a host/target matrix covering architecture, ABI, libc, kernel, rootfs,
  credentials, namespaces, and hardware availability;
- write a contract table for a blocking operation with its timeout, interruption,
  partial-progress, cleanup, and recovery behavior;
- distinguish “not implemented,” “not present,” “not permitted,” “temporarily
  unavailable,” and “application bug” rather than collapsing them into “it failed.”

## Related topics

- [Linux Userspace And System Programming](../index.md)
- [C Programming](../../c/index.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Native Linux Userspace Builds](../../build-systems/native-linux-userspace-builds.md)
- [Debugging](../../debugging/index.md)

## References

- [Linux man-pages project](https://www.kernel.org/doc/man-pages/)
- [The Open Group Base Specifications, Issue 8](https://pubs.opengroup.org/onlinepubs/9799919799/)
- [The GNU C Library Reference Manual](https://sourceware.org/glibc/manual/)
- [Linux kernel userspace API documentation](https://www.kernel.org/doc/html/latest/userspace-api/index.html)
