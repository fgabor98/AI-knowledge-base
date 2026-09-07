---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# GDB, Core Dumps, And Symbols

## Build for diagnosis

Keep optimization appropriate for production, but preserve a separate unstripped binary and matching debug information. Record the compiler, flags, source revision, build ID, and library symbols. Split DWARF or a symbol server can keep deployed images small while retaining postmortem analysis.

## Live debugging

Start with:

```gdb
set pagination off
run
thread apply all bt full
info registers
info proc mappings
```

Inspect ownership and synchronization around the first suspicious frame. For a crash, identify the signal and fault address, then examine all threads; the crashing thread is not always the thread that caused the corruption.

## Core dumps

Configure core collection through the target's service manager and storage policy. Ensure the dump is large enough, retained securely, and associated with the exact executable and libraries. A missing core may be caused by `RLIMIT_CORE`, `core_pattern`, privilege transitions, a read-only/full filesystem, service sandboxing, or deliberate dump suppression.

Load with matching artifacts:

```sh
gdb /path/to/unstripped/app /path/to/core
```

Then run inside GDB (the memory display uses 8-byte units; adapt for the target):

```gdb
thread apply all bt full
info sharedlibrary
x/16gx $sp
```

Optimized code can show unavailable variables, inlined frames, or misleading source lines. Treat a backtrace as evidence, not proof; heap corruption often surfaces long after the original overwrite. Use sanitizers and targeted logging to narrow the cause.

## Match the executable before interpreting addresses

A similar version string is insufficient. Compare the build ID from
`readelf -nW`, retain the exact executable and DSOs, and archive debug
information by identity. Separate debug files may be located through a build
ID or debuglink; verify symbol discovery during release validation rather than
waiting for the first field crash.
See [GDB separate debug files](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Separate-Debug-Files.html).

For target analysis, use a debugger supporting the target architecture and
point it to the target rootfs snapshot. Inside GDB:

```gdb
set sysroot /path/to/matching-rootfs
set debug-file-directory /path/to/debug-symbols
set substitute-path /original/build/source /path/to/matching-source
info files
info sharedlibrary
thread apply all bt full
```

The sysroot supplies matching libraries; source substitution only maps source
paths. It cannot repair mismatched executable bytes.
See [GDB file handling](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Files.html).

## Separate crash cause from crash location

A faulting `memcpy` could have received a stale source pointer from a much
earlier operation. Inspect call arguments, object lifetime and adjacent
threads before blaming libc. For a deadlock, build a wait-for graph from all
thread stacks rather than inspecting only the main thread.

A core is a selected snapshot, not a full machine checkpoint. Dump filtering,
file-backed mappings, size limits and collector policy can omit memory.
Treat unavailable memory as missing evidence. Keep dumps access-controlled
because they can contain credentials, keys and customer data.

## Related topics

- [Stage 14 overview](index.md)
- [Testing and verification](../testing-and-verification/index.md)
