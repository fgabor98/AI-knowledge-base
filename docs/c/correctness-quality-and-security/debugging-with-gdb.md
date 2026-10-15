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

## A Worked Session

The rest of this page follows one bug from the crash to its cause. The program keeps a
ring buffer of sensor samples behind a short device name:

```c
--8<-- "examples/c/tool-sessions/gdb/sensor_log.c:13:47"
```

`device_set_name` never checks the name's length against `sizeof dev->name`. A long name
runs into `ring.head`, and the program crashes later, inside `ring_push`. The crash happens
far from the write that caused it, which is the usual shape of memory corruption.

Every transcript below comes from
`bash examples/c/tool-sessions/gdb/run.sh` (GCC 13.3, GDB 15.1, x86-64). The commands use
`gdb -q -nx -batch -iex 'set debuginfod enabled off'` so they are repeatable; below, that
prefix is shortened to `gdb -q -batch`. Interactively, type the same commands at the
`(gdb)` prompt.

```text
$ gcc -Og -g3 -Wall -Wextra sensor_log.c -o sensor_log
$ ./sensor_log
Segmentation fault (core dumped)
[exit status 139]
$ ./sensor_log adc0
adc0: 4 samples
```

The program works with a short name, and it compiles without a warning.

## Step 1: Where It Crashed, And With What State

```text
$ gdb -q -batch -ex run -ex bt -ex 'p ring->head' -ex 'p/x ring->head' \
      -ex 'x/s (char *)&ring->head' ./sensor_log

Program received signal SIGSEGV, Segmentation fault.
0x000055555555516c in ring_push (ring=ring@entry=0x555555558048 <dev+8>, sample=sample@entry=...) at sensor_log.c:42
42	    ring->slots[ring->head] = sample;
#0  0x000055555555516c in ring_push (ring=ring@entry=0x555555558048 <dev+8>, sample=sample@entry=...) at sensor_log.c:42
#1  0x000055555555525a in main (argc=<optimized out>, argv=<optimized out>) at sensor_log.c:62
$1 = 8317697107157152373
$2 = 0x736e65732d657275
0x555555558048 <dev.0+8>:	"ure-sensor-0"
```

Reading it:

- The faulting statement indexes `slots` with `ring->head`, and `head` is about 8.3 × 10^18,
  not a value from 0 to 7.
- Printed in hex, the value is ASCII bytes. `x/s` on the field's address shows the text
  `"ure-sensor-0"`: the tail of `"temperature-sensor-0"`, so the name has overwritten
  the ring buffer.
- `ring=0x...8048 <dev+8>` says the ring starts 8 bytes into `dev`, right after
  `char name[8]`.

The backtrace answers *where*. The memory contents point to *who*: something that writes
names.

## Step 2: Find The Writer With A Watchpoint

A watchpoint stops the program when a memory location changes. Set it on the field that
ends up corrupted, before the corruption happens:

```text
$ gdb -q -batch -ex 'break main' -ex run -ex 'watch -l dev.ring.head' -ex continue \
      -ex bt -ex 'p i' -ex 'p length' -ex 'p sizeof dev->name' ./sensor_log
Breakpoint 1 at 0x11eb: file sensor_log.c, line 55.

Breakpoint 1, main (argc=1, argv=0x7fffffffd528) at sensor_log.c:55
55	{
Hardware watchpoint 2: -location dev.ring.head

Hardware watchpoint 2: -location dev.ring.head

Old value = 0
New value = 117
device_set_name (dev=dev@entry=0x555555558040 <dev>, name=0x555555556004 "temperature-sensor-0") at sensor_log.c:35
35	    for (size_t i = 0; i <= length; i++) {   /* no check against sizeof dev->name */
#0  device_set_name (dev=dev@entry=0x555555558040 <dev>, name=0x555555556004 "temperature-sensor-0") at sensor_log.c:35
#1  0x0000555555555208 in main (argc=<optimized out>, argv=<optimized out>) at sensor_log.c:59
$1 = 8
$2 = 20
$3 = 8
```

- `New value = 117` is `'u'`, the ninth character of `"temperature-sensor-0"`.
- `i = 8` is the first index past the end of `name`. `length` is 20, and the array holds 8.
- The program stops on the loop line *after* the write, because a hardware watchpoint
  triggers once the store has completed.

Practical notes:

- `watch -l` (`-location`) watches the address, not the expression. Without it, GDB
  re-evaluates `dev.ring.head` in each scope and stops watching when the expression
  goes out of scope.
- Hardware watchpoints are fast but limited, typically four debug registers on x86-64 and
  two to four comparators on Cortex-M. A watchpoint on a larger object falls back to a
  software watchpoint, which single-steps the program; the GDB manual describes software
  watchpoints as hundreds of times slower.
- Conditions narrow the stop: `watch -l ring->count if ring->count > 8`.
- If the corruption happens before you can set the watchpoint, watch a guard word placed
  next to the object, or break on the suspected writers instead.

## Step 3: Capture A Core File, Analyze It Offline

On this machine `/proc/sys/kernel/core_pattern` hands crashes to Ubuntu's `apport`, so no
`core` file appears in the working directory. GDB can write one itself at the moment of
the crash:

```text
$ gdb -q -batch -ex run -ex 'generate-core-file sensor_log.core' ./sensor_log

Program received signal SIGSEGV, Segmentation fault.
0x000055555555516c in ring_push (ring=ring@entry=0x555555558048 <dev+8>, sample=sample@entry=...) at sensor_log.c:42
42	    ring->slots[ring->head] = sample;
warning: Memory read failed for corefile section, 4096 bytes at 0xffffffffff600000.
Saved corefile sensor_log.core
```

The warning concerns the kernel's legacy `vsyscall` page, which cannot be read. It does
not affect the analysis. In production, set `core_pattern` and `ulimit -c` deliberately
(or use `systemd-coredump`), and store each core with the exact executable and debug file.

A core is analyzed without running the program. A versioned command file makes the
report the same every time (see [Automating GDB](#automating-gdb)):

```text
$ gdb -q -batch -x crash-report.gdb -ex crash_report ./sensor_log sensor_log.core
Core was generated by `./sensor_log'.
Program terminated with signal SIGSEGV, Segmentation fault.
...
== registers
rip            0x55555555516c      0x55555555516c <ring_push+3>
rsp            0x7fffffffd3e8      0x7fffffffd3e8
rbp            0x0                 0x0
== faulting instruction
=> 0x55555555516c <ring_push+3>:	mov    QWORD PTR [rdi+rax*8+0x10],rsi
== locals, every frame
...
#1  0x000055555555525a in main (argc=<optimized out>, argv=<optimized out>) at sensor_log.c:62
sample = <optimized out>
tick = 0
dev = {name = "temperat", ring = {head = 8317697107157152373, count = 808284783, slots = {{timestamp = 0, value = 0}, ...}}}
```

The faulting instruction is a store to `rdi + rax*8 + 0x10`: `rdi` is `ring`, `rax` is
`head`, `0x10` is the offset of `slots`, and 8 is `sizeof(struct sample)`. Mapping a
faulting instruction back to the C expression like this is how you check a backtrace you
are not sure of. `count` is corrupted too: the 20-byte name plus terminator wrote past
`head` into it.

## Step 4: Trace Without Stopping

A breakpoint changes timing. `dprintf` prints at a location and continues, much like
adding a `printf` without rebuilding:

```text
$ gdb -q -batch -ex 'dprintf ring_push,"push: head=%zu count=%zu value=%d\n", ring->head, ring->count, sample.value' \
      -ex 'run adc0' ./sensor_log
Dprintf 1 at 0x1169: file sensor_log.c, line 42.
push: head=0 count=0 value=200
push: head=1 count=1 value=201
push: head=2 count=2 value=202
push: head=3 count=3 value=203
adc0: 4 samples
[Inferior 1 (process 20025) exited normally]
```

Other ways to place a breakpoint, for reference:

~~~text
(gdb) break packet_decode            # function
(gdb) break packet.c:120             # file and line
(gdb) break *0x08001234              # exact address, e.g. a fault PC
(gdb) rbreak '^driver_'              # every function matching a regex
(gdb) condition 2 length > 128       # stop only when the condition holds
(gdb) tbreak main                    # one-shot breakpoint
~~~

Use conditional breakpoints when stopping on every interrupt or packet would destroy timing. Software breakpoints modify instructions and may not work in read-only flash; hardware breakpoint resources are limited on microcontrollers. `info breakpoints` shows how each breakpoint was inserted.

## Step 5: The Same Crash In An Optimized Build

Release builds behave differently in a debugger. The same program at `-O2 -g`:

```text
$ gcc -O2 -g sensor_log.c -o sensor_log_O2
$ gdb -q -batch -ex run -ex bt -ex 'info frame' ./sensor_log_O2

Program received signal SIGSEGV, Segmentation fault.
0x00005555555550e6 in ring_push (sample=..., ring=0x555555558048 <dev+8>) at sensor_log.c:42
42	    ring->slots[ring->head] = sample;
#0  0x00005555555550e6 in ring_push (sample=..., ring=0x555555558048 <dev+8>) at sensor_log.c:42
#1  main (argc=<optimized out>, argv=<optimized out>) at sensor_log.c:62
Stack level 0, frame at 0x7fffffffd410:
 rip = 0x5555555550e6 in ring_push (sensor_log.c:42); saved rip = 0x7ffff7c2a1ca
 inlined into frame 1
...
$ gdb -q -batch -ex 'break device_set_name' -ex 'run adc0' -ex 'info locals' ./sensor_log_O2
...
Breakpoint 1, device_set_name (dev=0x555555558040 <dev>, name=0x7fffffffdae5 "adc0") at sensor_log.c:33
33	    size_t length = strlen(name);
length = <optimized out>
```

- `#1 main` has no address, and `info frame` says `inlined into frame 1`: `ring_push`
  no longer exists as a function. GDB rebuilds the inlined frame from DWARF so the
  backtrace still reads like the source.
- `length = <optimized out>` means that at this program counter, the variable is not
  stored anywhere the debug information can describe. This is not a GDB bug. Step
  further, or read the value from the register that holds it in the disassembly.
- Source lines are approximate in optimized code: one line can map to scattered
  instructions, and `next` may appear to jump backwards. Use `disassemble /s` and
  `stepi` when the order of events matters.

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

Check stack alignment, bounds, saved return address, exception frame format, interrupt nesting, and whether the loaded symbols match the image. Optimizers can inline functions, omit frame pointers, tail-call, split variables, and move instructions; an apparently impossible frame may be an ABI or corruption symptom rather than a GDB failure. Step 3 shows the cross-check: decode the faulting instruction and confirm that its operands match the source expression.

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

Do not call printf, malloc, locks, or complex drivers from a fault handler unless their context safety is proven. Store a compact record and format it after reboot. [Debug Information And Binary Inspection](../compilation-linking-and-abi/debug-information-and-binary-inspection.md#a-field-crash-record) shows the hosted equivalent end to end: a handler that records the build ID and a module-relative PC, then offline symbolization with the matching debug file.

## Remote Debugging

A common remote flow (not reproduced on this page; it needs a debug probe and a target):

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

## Automating GDB

The report in Step 3 comes from a command file kept in version control:

```text
--8<-- "examples/c/tool-sessions/gdb/crash-report.gdb"
```

`frame apply all -s` runs a command in every frame and skips frames where it fails.
Keep automation versioned and target-aware: a script that reads the wrong register
names (`rip` here, `pc` and `xpsr` on Cortex-M) or the wrong task structure produces
misleading evidence. For anything beyond a fixed command list, use GDB's Python API.

## Exercises

1. Run `examples/c/tool-sessions/gdb/run.sh`, then repeat Steps 1–2 interactively and
   find the corrupting write without reading the source first.
2. Fix `device_set_name`, then confirm with the watchpoint from Step 2 that `ring.head`
   is written only by `ring_push`.
3. Stop on the first packet whose length violates a boundary using a conditional breakpoint.
4. Debug the same optimized binary with and without `-fno-omit-frame-pointer` and
   compare the backtraces.
5. Extend `crash-report.gdb` to print the memory around the faulting address and the
   shared-library load addresses (`info sharedlibrary`).
6. Connect to a remote target and document reset, load, breakpoint, and symbol assumptions.
7. Build the same session with `-O0`, `-Og`, and `-O2`, and record which variables become
   `<optimized out>` at each level.

## Common Mistakes

- debugging with symbols from a different build;
- assuming `-O0` reproduces optimized behavior;
- treating a source line as the exact faulting instruction;
- watching an expression instead of a location (`watch` vs `watch -l`) and losing the
  watchpoint when the scope ends;
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
- [GDB `dprintf`](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Dynamic-Printf.html)
- [GDB remote debugging](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Remote-Debugging.html)
- [GCC debugging options](https://gcc.gnu.org/onlinedocs/gcc/Debugging-Options.html)
