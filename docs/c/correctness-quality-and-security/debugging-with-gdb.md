---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Debugging With GDB

GDB is a way to interrogate a running or stopped program, not a substitute for a clear failure model. Effective debugging combines the exact executable and symbols with source, assembly, registers, memory, timing, target state, and the assumptions that were supposed to hold.

## Learning Objectives

- use breakpoints, watchpoints, catchpoints, backtraces, and conditional commands;
- debug optimized C without treating source locations as exact instruction boundaries;
- inspect registers, stack, memory, disassembly, and thread/task state;
- connect GDB to core files, remote targets, and embedded debug probes;
- diagnose faults before `main`, in interrupts, and during bootloader handoff;
- preserve a reproducible crash-dump workflow.

## Build For Debugging

Keep symbols and make the build representative:

~~~sh
cc -std=c17 -Og -g3 -fno-omit-frame-pointer \
   -Wall -Wextra -Wpedantic app.c -o app
gdb -q ./app
~~~

`-g` supplies source and type information; it does not disable optimization. `-Og` is often convenient for interactive debugging, but reproduce release-only failures with the release optimization and linker configuration. Keep the unstripped ELF, map, source revision, target flags, and debug file together.

## A Core Session

~~~text
gdb ./app core
(gdb) set pagination off
(gdb) info files
(gdb) info threads
(gdb) thread apply all bt full
(gdb) info registers
(gdb) x/32gx $sp
(gdb) disassemble /m suspicious_function
(gdb) quit
~~~

For a live process, use `run`, `continue`, `next`, `step`, `finish`, and `until` to control execution. For a core, do not issue commands that assume the program can resume; inspect the captured state and mappings.

## Breakpoints

Breakpoints can be placed by function, file/line, address, or regular expression:

~~~text
(gdb) break packet_decode
(gdb) break packet.c:120
(gdb) break *0x08001234
(gdb) rbreak '^driver_'
(gdb) condition 2 length > 128
(gdb) commands 2
> silent
> printf "length=%zu\\n", length
> continue
> end
~~~

Use conditional breakpoints when stopping on every interrupt or packet would destroy timing. Software breakpoints modify instructions and may not work in read-only flash; hardware breakpoint resources are limited on microcontrollers. Confirm how the debugger inserted the breakpoint.

## Watchpoints

A watchpoint stops when an expression changes; an access watchpoint can stop on reads or writes when the target supports it:

~~~text
(gdb) watch ring->head
(gdb) awatch status_register
(gdb) condition 3 ring->count > ring->capacity
(gdb) info watchpoints
~~~

Hardware watchpoints are precise and fast but scarce and limited in width/alignment. Software watchpoints single-step or repeatedly evaluate state and can be too slow, especially with other threads or interrupts. If a memory corruption occurs before the watchpoint triggers, watch a guard word or set a breakpoint on suspected writers.

## Stack, Registers, And ABI State

When a backtrace is suspect, inspect the ABI state manually:

~~~text
(gdb) info registers
(gdb) p/x $sp
(gdb) p/x $pc
(gdb) x/64wx $sp
(gdb) disassemble /r $pc-32, $pc+64
(gdb) info frame
(gdb) bt full
~~~

Check stack alignment, bounds, saved return address, exception frame format, interrupt nesting, and whether the loaded symbols match the image. Optimizers can inline functions, omit frame pointers, tail-call, split variables, and move instructions; an apparently impossible frame may be an ABI or corruption symptom rather than a GDB failure.

## Embedded Fault Debugging

For a Cortex-M-style fault, collect the stacked registers, configurable fault status registers, fault address, active exception, and reset reason. The exact register names vary by architecture. A reliable workflow is:

1. halt immediately after the fault or reboot into a retention record;
2. identify the image build ID and load address;
3. decode the exception frame according to the architecture;
4. resolve PC and LR against the exact ELF;
5. inspect stack bounds and nearby memory;
6. disassemble the faulting instruction and its callers;
7. classify memory, bus, usage, privilege, timing, or boot-state causes;
8. reproduce with the same optimization and peripheral state.

Do not call printf, malloc, locks, or complex drivers from a fault handler unless their context safety is proven. Store a compact record and format it after reboot.

## Remote Debugging

A common remote flow is:

~~~text
arm-none-eabi-gdb build/firmware.elf
(gdb) target extended-remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) break Reset_Handler
(gdb) continue
~~~

The monitor commands depend on the probe and server. Be explicit about whether `load` changed the running image, whether reset was cold or warm, and which memory regions are accessible. Remote latency and limited hardware breakpoint/watchpoint resources affect debugging strategy.

## Threads, Tasks, And Interrupts

For hosted threads, inspect all threads and synchronization state. For RTOS tasks, use the RTOS-aware GDB plugin or monitor commands if available, but verify that task control blocks and stacks are current. An interrupt can change a variable between two debugger observations, and halting a core can change watchdog, DMA, and peripheral behavior.

Use trace buffers, GPIO markers, event counters, and crash records for timing-sensitive evidence. A breakpoint that “fixes” a race is evidence that timing matters, not that the race disappeared.

## Core Dumps And Crash Artifacts

For a useful crash artifact, preserve:

- exact executable and separate debug file;
- core or fault record;
- shared-library versions and load mappings;
- command line and environment when relevant;
- build ID, source revision, and configuration;
- target architecture and ABI;
- reset reason, task ID, and recent trace events.

Automate symbolization only after validating artifact identity. A crash pipeline should reject a mismatched build rather than produce a confident-looking but wrong source line.

## GDB Automation

Command files and Python extensions can standardize diagnostics:

~~~text
define dump_fault
  info registers
  x/32wx $sp
  bt full
  disassemble /r $pc-32, $pc+64
end
document dump_fault
  Save the minimal CPU fault context.
end
~~~

Keep automation versioned and target-aware. A script that reads the wrong register names or task structure can create misleading evidence.

## Exercises

1. Debug a deliberate heap use-after-free with a breakpoint, watchpoint, and sanitizer comparison.
2. Stop on the first packet whose length violates a boundary using a conditional breakpoint.
3. Corrupt a stack guard and determine whether the first write can be located.
4. Debug the same optimized binary with and without frame pointers.
5. Connect to a remote target and document reset, load, breakpoint, and symbol assumptions.
6. Create a compact fault record and symbolize it offline after a simulated reboot.
7. Write a GDB command file that captures registers, stack, backtrace, and disassembly.

## Common Mistakes

- debugging with symbols from a different build;
- assuming `-O0` reproduces optimized behavior;
- treating a source line as the exact faulting instruction;
- exhausting hardware breakpoint/watchpoint resources unknowingly;
- relying on software watchpoints for real-time or multithreaded behavior;
- using a breakpoint in an ISR or watchdog path without considering timing;
- letting the debugger reset state differently from production reset;
- calling unsafe logging or allocation functions from a fault handler;
- symbolizing a relocated image without its load base;
- trusting automated crash output without build-ID verification.

## Related Topics

- [Debug Information And Binary Inspection](../compilation-linking-and-abi/debug-information-and-binary-inspection.md)
- [Sanitizers And Dynamic Analysis](./sanitizers-and-dynamic-analysis.md)
- [Testing Strategy](./testing-strategy.md)
- [Startup, Reset, And Vector Tables](../embedded-c-and-hardware/startup-reset-and-vector-tables.md)
- [Interrupts, Exceptions, And Faults](../embedded-c-and-hardware/interrupts-exceptions-and-faults.md)

## References

- [GDB documentation](https://sourceware.org/gdb/current/onlinedocs/gdb.html/)
- [GDB breakpoints and watchpoints](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Breakpoints.html)
- [GDB remote debugging](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html)
- [GCC debugging options](https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html)
