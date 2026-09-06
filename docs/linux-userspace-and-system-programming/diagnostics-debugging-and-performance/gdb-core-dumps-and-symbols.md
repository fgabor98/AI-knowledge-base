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
(gdb) thread apply all bt full
(gdb) info sharedlibrary
(gdb) x/16gx $sp
```

Optimized code can show unavailable variables, inlined frames, or misleading source lines. Treat a backtrace as evidence, not proof; heap corruption often surfaces long after the original overwrite. Use sanitizers and targeted logging to narrow the cause.
