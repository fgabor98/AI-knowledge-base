---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Embedded Linux

Embedded Linux combines application-processor hardware with a process-oriented,
virtual-memory operating system. C appears in userspace applications, daemons, shared
libraries, bootloaders, kernel code, drivers, and firmware interfaces. The same word
“pointer” has different ownership and validity rules in each context, so a senior C
engineer must identify the boundary before choosing an API.

## Learning Objectives

- Explain the boot chain from ROM/bootloader through kernel, init, and application.
- Choose between a userspace interface, a kernel driver, UIO/VFIO-style access, and a
  firmware service.
- Use file descriptors, `mmap`, `ioctl`, polling, signals, and sysfs/configuration
  interfaces with correct lifetime and error handling.
- Understand Device Tree as a hardware-description and driver-binding contract.
- Respect kernel context, memory allocation, locking, DMA, and user-pointer rules.

## Boot And Deployment Model

A common embedded Linux image contains:

1. immutable ROM or a first-stage loader;
2. trusted firmware and/or a second-stage bootloader;
3. a kernel image and hardware description (often Device Tree);
4. an initramfs or persistent root filesystem;
5. an init system, device manager, services, and application processes.

Each stage has a different memory map, privilege level, image format, and failure
reporting mechanism. The bootloader may configure clocks, DRAM, secure state, and
device ownership before Linux starts. The kernel then establishes virtual memory,
process isolation, scheduling, drivers, and system calls. Applications should not
assume that a bootloader's temporary mapping or register setup remains valid.

Record the image hand-off: kernel load address, Device Tree address, initrd address,
address width, cache state, secure boot measurements, and reserved-memory regions.
Use the exact kernel configuration and device bindings for the target rather than
copying a board example that happens to boot.

## Hosted C And the Userspace Boundary

An embedded Linux application is usually hosted C. It can use the C library and POSIX
services, but it still faces resource limits, cross-compilation, read-only filesystems,
signals, process termination, and hardware failures. `main` is not the beginning of
the whole machine; it is the entry to one process after the loader has mapped the
executable and shared objects.

Use opaque file descriptors and handles as ownership tokens. Check every return value,
including short reads/writes, `EINTR`, timeout, cancellation, and device removal.
Convert untrusted or device-provided lengths before allocating or indexing. Keep
hardware policy in a driver-facing module so the application can be tested with a
fake descriptor or transport.

## File Descriptors And System Calls

Linux exposes many resources as file descriptors: regular files, device nodes, sockets,
event sources, timers, and special kernel interfaces. A descriptor has a lifetime and
often a current offset or state. `close` can race with another thread that reuses the
same integer, so define ownership and synchronization around descriptor hand-off.

The usual C path is application -> libc wrapper -> syscall -> kernel subsystem. The
wrapper may retry, translate errors, or add cancellation behavior. Direct syscalls
should be reserved for a documented need and a target-specific boundary.

```c
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>

static int write_all(int fd, const void *buffer, size_t length)
{
    const unsigned char *bytes = buffer;
    size_t offset = 0u;

    while (offset < length) {
        ssize_t result = write(fd, bytes + offset, length - offset);
        if (result > 0) {
            offset += (size_t)result;
            continue;
        }
        if (result < 0 && errno == EINTR) {
            continue;
        }
        return -1;
    }
    return 0;
}
```

This function handles short writes and interruption but does not define behavior for a
non-blocking descriptor, a signal policy, or a device that reports a permanent error.
Those policies belong in the caller or a richer transport abstraction.

## `ioctl`, Device Files, And ABI Stability

`ioctl` is a multiplexed device-specific interface. Its request number and argument
layout are part of a userspace/kernel ABI, not merely a private function signature.
Define request structures with fixed-width types where appropriate, specify alignment
and versioning, and validate every pointer, length, flag, and reserved field in the
kernel. Do not pass a userspace pointer through a structure and dereference it in the
kernel without the documented user-copy APIs.

Prefer a clear read/write or netlink/configuration interface when it expresses the
operation naturally. Use `ioctl` when the operation is genuinely device-specific or
needs a compact command ABI. Version structures with a size field or a new command
number, and preserve compatibility when existing products are in the field.

## `mmap` And Memory-Mapped Devices

`mmap` maps a file or device range into a process address space. For a device mapping,
the kernel driver must choose appropriate page attributes and validate the offset and
length. Userspace must treat the mapped region as an opaque device protocol, not as an
ordinary C object with freely reorderable accesses.

```c
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

static int unmap_region(void *address, size_t length)
{
    if (address == MAP_FAILED || length == 0u) {
        errno = EINVAL;
        return -1;
    }
    return munmap(address, length);
}
```

The actual mapping call should check page-size alignment, overflow in offset plus
length, permissions, and the device's lifetime. A mapping can outlive a device reset,
driver removal, or power transition; define what userspace sees in those cases.

`volatile` may be appropriate for a userspace MMIO view only when the driver and
architecture explicitly define that interface. It does not make accesses atomic,
ordered with respect to another thread, or safe after unmap.

## Device Tree And Hardware Discovery

Device Tree describes nodes, properties, address ranges, interrupts, clocks, resets,
power domains, regulators, DMA channels, and compatibility. Linux uses it to identify
the board and populate devices; drivers consume bindings rather than hard-coding every
board address.

A binding should define:

- a stable `compatible` value and fallback compatibility rules;
- required and optional properties, units, ranges, and defaults;
- register and interrupt semantics;
- clock/reset/power dependencies;
- DMA address and coherency constraints;
- schema validation and versioning expectations.

The Device Tree is not a substitute for driver validation. A malformed or stale tree
can describe a resource that is not wired, powered, or safe to access. Fail during
probe with an actionable error when required resources cannot be acquired.

## Userspace Versus Kernel Drivers

Choose the boundary based on latency, isolation, interrupt needs, DMA, security, and
existing kernel support:

| Approach | Strengths | Costs and risks |
| --- | --- | --- |
| Existing kernel subsystem | Stable semantics and reuse | Must fit the subsystem model |
| New kernel driver | Interrupt, DMA, power, and security integration | Kernel memory/locking rules and review burden |
| UIO-like userspace access | Fast iteration for simple devices | Limited interrupt/DMA/security model |
| VFIO/IOMMU-mediated access | Stronger isolation for suitable devices | Requires IOMMU and subsystem support |
| Firmware/remote processor service | Encapsulates proprietary or real-time logic | IPC, versioning, and ownership complexity |
| Direct `/dev/mem`-style access | Quick experiments only | Poor isolation, unsafe mappings, fragile deployment |

The last option should not be a production architecture. It bypasses the resource,
power, security, and coherency policies a driver is supposed to enforce.

## Kernel C Rules

Kernel C is not hosted C. Depending on context, code may be unable to sleep, allocate
with a blocking flag, take a mutex, use floating point, access userspace memory, or
hold a spinlock while performing an operation that can fault. Allocation flags encode
whether reclaim/sleep is allowed; choose them from the execution context, not from
convenience.

Distinguish:

- process context, where sleeping and userspace access may be possible;
- interrupt/softirq context, where latency and non-sleeping rules dominate;
- threaded interrupt or worker context, where deferred work can block under policy;
- atomic/lock-held sections, where operations must be bounded and non-blocking.

Use kernel primitives for locking, refcounts, atomics, RCU, barriers, and DMA. Do not
substitute a userspace pthread primitive or an ISO C library function into kernel code.

## Real-Time Considerations

Embedded Linux can provide bounded behavior only under an explicitly configured and
measured system. Account for scheduler policy, CPU frequency changes, interrupt
affinity, threaded IRQs, page faults, memory pressure, filesystem latency, thermal
throttling, and other kernel work. Lock memory where justified, pre-fault critical
buffers, isolate or reserve CPUs when required, and measure end-to-end latency rather
than only a function's execution time.

## Exercises And Diagnostics

1. Trace a device from Device Tree node to kernel probe, `/dev` node, userspace open,
   operation, interrupt, and removal.
2. Define an `ioctl` structure with a version/size field and write a validation table
   for every field, pointer, range, and error.
3. Compare a userspace polling design with an interrupt-driven kernel driver. Identify
   CPU use, latency, power, and failure-recovery trade-offs.
4. Instrument a daemon's read/write loop to distinguish short I/O, `EINTR`, timeout,
   device removal, and permanent errors.
5. Use `readelf`, `ldd`/loader diagnostics, and a root filesystem manifest to explain
   why an application works on the build host but not on the target.

## Common Mistakes

- Treating an embedded Linux process as if it owns physical memory or device registers.
- Using `/dev/mem` as a replacement for a driver in production.
- Passing unstable pointers or native-layout structs as an undocumented `ioctl` ABI.
- Forgetting short I/O, `EINTR`, descriptor ownership, and hot-unplug/reset behavior.
- Using `volatile` instead of kernel/user synchronization and device access rules.
- Sleeping, allocating, or taking an inappropriate lock from interrupt or atomic context.
- Hard-coding Device Tree resources in a driver or trusting an unvalidated binding.
- Assuming cache coherence, DMA reachability, or memory attributes without checking the
  SoC and kernel DMA contract.

## Related Topics

- [Platform-Specific C overview](./index.md)
- [ARM Cortex-A And AArch64](./arm-cortex-a-and-aarch64.md)
- [Compiler And Vendor Extensions](./compiler-and-vendor-extensions.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Embedded C And Hardware](../embedded-c-and-hardware/index.md)

## References

- [Linux userspace API documentation](https://docs.kernel.org/userspace-api/index.html)
- [Linux driver API documentation](https://docs.kernel.org/driver-api/index.html)
- [Linux Device Tree usage model](https://docs.kernel.org/6.3/devicetree/usage-model.html)
- [Linux Device Tree documentation](https://docs.kernel.org/next/devicetree/index.html)
- [Linux platform device documentation](https://docs.kernel.org/driver-api/driver-model/platform.html)
- [Linux ioctl documentation](https://docs.kernel.org/userspace-api/ioctl/ioctl-number.html)
- The exact kernel version, configuration, SoC manual, binding schema, bootloader, and
  root filesystem build definition
