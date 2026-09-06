---
status: draft
reviewed: false
domain: linux-userspace
difficulty: intermediate
last_reviewed: null
---

# Memory Protection And Process Hardening

## What problem does this solve?

Memory corruption becomes more difficult to exploit when writable data is not
executable, code is not writable, and unexpected mappings or privilege transitions
are constrained. These protections do not repair a bug; they reduce attack surface
and make failures more diagnosable when enabled and tested deliberately.

## Page permissions

`mmap` and `mprotect` describe read, write, and execute permissions for page-aligned
ranges. A write to read-only memory or execution from a non-executable page usually
causes SIGSEGV. Changing protection can fail because of alignment, resource limits,
or policy.

```c
void *region = mmap(NULL, length, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (region == MAP_FAILED) {
    /* report errno */
}
/* initialize, then remove write permission if the design permits */
if (mprotect(region, length, PROT_READ) == -1) {
    /* do not continue as if protection changed */
}
```

Avoid writable-executable (`RWX`) mappings. JITs and dynamically generated code need
a carefully reviewed W^X transition, cache policy, seccomp/LSM allowance, and a
threat model. Do not make an entire heap executable to silence a crash.

## Build and loader hardening

Common ELF protections include:

- **PIE + ASLR:** position-independent executable and randomized mappings;
- **NX:** non-executable data pages where hardware supports it;
- **RELRO:** read-only relocation structures, often partial or full;
- **stack canaries:** detection of some stack overwrites;
- **FORTIFY-style checks:** selected compile-time/runtime object-size checks.

Inspect rather than assume:

```sh
readelf -hW ./app
readelf -lW ./app | grep -E 'GNU_STACK|GNU_RELRO'
readelf -dW ./app | grep -E 'BIND_NOW|NEEDED'
```

Flags vary by compiler, linker, libc, and target toolchain. A hardening flag can alter
compatibility or performance; make it a build policy with a tested exception process.

## Guard pages and stacks

A guard page is left inaccessible beside a stack or allocation so an overrun faults
near its cause. Pthread attributes can request stack size and guard size. This helps
diagnosis but consumes virtual address space and does not replace bounds checking.

Alternate signal stacks can let a signal handler run after a normal stack overflow,
but the handler must remain async-signal-safe and the process should generally fail
closed after a fatal memory violation.

## Process-level restrictions

Memory protection complements:

- least-privilege UID/GID and capabilities;
- `no_new_privs`, seccomp, namespaces, and service sandboxing;
- read-only system mounts and private runtime directories;
- resource limits and cgroups;
- controlled dynamic-loader environment;
- minimized device and filesystem access.

Each restriction can cause a previously working call to return `EPERM`, `EACCES`,
`ENOMEM`, or a signal. Include the policy in the deployment identity and test the
service as it actually runs.

## Diagnosing protection failures

Capture the signal, fault address, instruction pointer, mapping permissions, build
identity, and core/backtrace. A SIGSEGV can indicate a null pointer, use-after-free,
out-of-bounds access, deliberate protection, or an invalid device mapping. Do not
classify it from the signal name alone.

## Common mistakes

- Disabling ASLR or NX globally to hide a bug.
- Treating hardening flags as proof of memory safety.
- Creating RWX mappings without a reviewed reason.
- Calling `mprotect` on an unaligned range and ignoring its failure.
- Assuming a guard page catches every overflow.
- Testing only as root or without the production sandbox.
- Treating a protection fault as evidence that the kernel or hardware is broken.

## Debugging checklist

- Inspect GNU_STACK, GNU_RELRO, PIE, dependencies, and loader policy.
- Capture `/proc/<pid>/maps` and the faulting address.
- Check mapping alignment, protection transitions, and `mprotect` return values.
- Check stack limits, guard size, and thread attributes.
- Reproduce under ASan/UBSan or a debugger in a controlled host test.
- Test the production UID, capabilities, namespaces, seccomp, and mount policy.

## Related topics

- [Stage 4: Process Memory And Mapping](index.md)
- [Process Address Space](process-address-space.md)
- [Memory Pressure, OOM, And Real-Time Constraints](memory-pressure-oom-and-realtime.md)
- [Userspace Security](../identity-privilege-and-userspace-security/index.md)

## References

- [`mprotect(2)`](https://man7.org/linux/man-pages/man2/mprotect.2.html)
- [`proc_pid_maps(5)`](https://man7.org/linux/man-pages/man5/proc_pid_maps.5.html)
- [Linux kernel self-protection documentation](https://www.kernel.org/doc/html/latest/security/self-protection.html)
- [GCC instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
