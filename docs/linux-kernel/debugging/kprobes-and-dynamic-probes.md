---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kprobes And Dynamic Probes

## What Problem Does This Solve?

Static tracepoints exist only where someone placed them. Kprobes let you observe almost any kernel instruction on a running system: who calls a function, with which arguments, and what it returned. No rebuild, no reboot, and no change to the code under investigation.

This is often the fastest way to answer a question about a vendor driver, or about a kernel you cannot rebuild.

## Core Concepts

- kprobe, kretprobe
- kprobe events in tracefs
- fetch arguments: `$argN`, `$retval`, `+offset(...)`
- fprobe events
- `register_kprobe()` in a module
- blacklist and `notrace`
- tracepoint versus probe

## Choosing A Tool

| Question | Tool |
| --- | --- |
| does this function run, from where | function tracer, `stacktrace` trigger |
| arguments or return value of an existing function | kprobe or fprobe event |
| documented subsystem event | tracepoint |
| value inside a function | kprobe at an offset, `perf probe` with debug info |
| aggregation, histograms, logic | bpftrace, histogram triggers |
| permanent instrumentation of your driver | add a tracepoint |

Prefer a tracepoint when one exists. It has named fields and survives refactoring; a kprobe depends on a function that may be renamed or inlined in the next release.

## Mental Model

```text
kprobe:     instruction replaced by a breakpoint or an ftrace hook
            -> handler runs with the register state
            -> original instruction executes
kretprobe:  entry probe replaces the return address
            -> handler runs on return with the return value
```

Handlers run in atomic context. They cannot sleep, and they cannot fault in userspace memory.

## Kprobe Events

Everything happens under `/sys/kernel/tracing` as root. Syntax:

```text
p[:GROUP/EVENT] [MODULE:]SYMBOL[+OFFSET] [NAME=ARG:TYPE ...]     probe
r[:GROUP/EVENT] [MODULE:]SYMBOL          [NAME=ARG:TYPE ...]     return probe
-:GROUP/EVENT                                                    remove
```

Example: log failing `clk_set_rate()` calls.

```sh
cd /sys/kernel/tracing
echo 'p:dbg/rate clk_set_rate rate=$arg2:u64'    >> kprobe_events
echo 'r:dbg/rate_ret clk_set_rate ret=$retval:s32' >> kprobe_events
echo 'ret < 0' > events/dbg/rate_ret/filter
echo 1 > events/dbg/enable
# reproduce
cat trace
echo 0 > events/dbg/enable
echo '-:dbg/rate' >> kprobe_events
echo '-:dbg/rate_ret' >> kprobe_events
```

Append with `>>`. A plain `>` clears all probes, including those of other users. An event must be disabled before it can be removed.

Common fetch arguments:

| Argument | Meaning |
| --- | --- |
| `$arg1` ... `$argN` | function arguments (x86-64 and arm64) |
| `$retval` | return value, return probes only |
| `$comm`, `$stack` | task name, stack address |
| `%di`, `%x0` | CPU register, architecture specific |
| `+8($arg1):u32` | memory at argument 1 plus 8 bytes |
| `+0($arg2):string` | kernel string |
| `+0($arg2):ustring` | userspace string |

On 32-bit ARM use registers: the first four arguments are `%r0` to `%r3`.

Reading a structure member needs its offset. With debug information, `pahole -C clk_core vmlinux` prints it. `perf probe` resolves names and source lines for you:

```sh
sudo perf probe -k vmlinux -V 'demo_write'
sudo perf probe -k vmlinux 'demo_write count'
```

On kernels built with BTF, recent versions accept structure fields directly in [fprobe and kprobe events](https://docs.kernel.org/trace/fprobetrace.html).

## Filters, Triggers, And Stack Traces

Probe events behave like any other trace event:

```sh
echo 'rate > 100000000' > events/dbg/rate/filter
echo stacktrace > events/dbg/rate/trigger
echo 'hist:keys=comm' > events/dbg/rate/trigger
cat events/dbg/rate/hist
```

Remove a trigger by writing it again with a leading `!`.

## Probing A Module

Use the module qualifier so that a same-named function elsewhere is not hit:

```sh
echo 'p:dbg/wr lab_char:demo_write count=$arg3:u64' >> kprobe_events
```

A probe for a module that is not loaded yet is accepted and armed when the module appears, which makes it possible to trace `probe()` of a module at load time.

## What Cannot Be Probed

| Cause | How to recognize it | Alternative |
| --- | --- | --- |
| function was inlined | absent from `/proc/kallsyms` | probe the caller, or `perf probe` on a line |
| compiler suffix | `demo_fn.isra.0`, `.constprop.0`, `.cold` | probe that exact name, arguments may differ |
| blacklisted | listed in `/sys/kernel/debug/kprobes/blacklist` | a neighboring function |
| function name is not unique | event creation fails | module qualifier or symbol plus offset |
| `CONFIG_KPROBES=n` | `kprobe_events` missing | debug kernel |

Check the name first:

```sh
sudo grep -w clk_set_rate /proc/kallsyms
```

## Kprobes In A Module

Tracefs is enough for almost everything. A module is useful when the handler needs logic:

```c
static int demo_pre(struct kprobe *p, struct pt_regs *regs)
{
    pr_info_ratelimited("%s: arg2=%lu\n", p->symbol_name,
                        regs_get_kernel_argument(regs, 1));
    return 0;
}

static struct kprobe demo_kp = {
    .symbol_name = "clk_set_rate",
    .pre_handler = demo_pre,
};

static int __init demo_init(void)
{
    return register_kprobe(&demo_kp);
}

static void __exit demo_exit(void)
{
    unregister_kprobe(&demo_kp);    /* waits for running handlers */
}
```

`regs_get_kernel_argument()` counts from zero and exists on x86-64, arm64, and several others. The handler must not sleep, take a lock that the probed code can hold, or call the probed function.

Only use this for observation. Older articles use kprobes to find `kallsyms_lookup_name()` and patch kernel internals; that bypasses the [export contract](../fundamentals/exported-symbols-and-module-dependencies.md#why-some-symbols-are-not-exported) and breaks with kernel hardening.

## Cost

An ftrace-based probe at a function entry costs well under a microsecond; a probe in the middle of a function uses a breakpoint and costs more. Return probes cost more than entry probes. That is harmless on a configuration path and can be fatal on a path that runs a million times per second.

## Runnable Example

`examples/linux-kernel/small-labs/17-kprobe-events` traces the character device of Lab 3, including its return value and the retry after a short write.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| `No such file or directory` writing `kprobe_events` | symbol missing or inlined | `/proc/kallsyms` |
| `Invalid argument` | syntax or fetch argument | `cat error_log` in tracefs |
| `Device or resource busy` on removal | event still enabled | `events/GROUP/enable` |
| arguments look wrong | optimized clone, wrong type | symbol suffix, signature in this kernel version |
| string shows `(fault)` | userspace page not resident, or kernel and user pointer mixed up | `string` or `ustring` |
| system slows down | probe on a hot path | filter does not help; the probe still fires |

## Practice Exercises

1. Run Lab 17 and explain each event of the 200-byte write.
2. Trace the return value of your driver's `probe()` while loading its module.
3. Add a `stacktrace` trigger to find every caller of `regulator_set_voltage()` during boot or resume.

## Debugging Checklist

- Is there a tracepoint already?
- Is the symbol present under exactly this name?
- Does the signature match the source of the running kernel?
- Was `error_log` read after a failure?
- Were probes disabled and removed afterwards?

## Related Topics

- [Ftrace And Tracepoints](ftrace-and-tracepoints.md)
- [Perf Overview](perf-overview.md)
- [Dynamic Debug](dynamic-debug.md)
- [Exported Symbols And Module Dependencies](../fundamentals/exported-symbols-and-module-dependencies.md)

## Official References

- [Kprobe-based event tracing](https://docs.kernel.org/trace/kprobetrace.html)
- [Kernel probes](https://docs.kernel.org/trace/kprobes.html)
- [Fprobe-based event tracing](https://docs.kernel.org/trace/fprobetrace.html)
