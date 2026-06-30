---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Debug Vs Production Configs

## What Problem Does This Solve?

Debug kernels expose failures earlier and with more evidence, while production kernels prioritize boot time, footprint, attack surface, and deterministic behavior.

Driver and kernel bugs often disappear or become silent under a production config:

- a race becomes a rare data corruption instead of a KCSAN report
- a lock inversion becomes an occasional hang instead of a lockdep warning
- a use-after-free becomes a random crash instead of a KASAN report
- a sleep-in-atomic bug becomes timing-dependent instead of a direct warning

Debug configs are for finding bugs. Production configs are for shipping policy. They should be related but not identical.

## Core Concepts

- debug symbols
- lockdep
- KASAN
- UBSAN
- dynamic debug
- ftrace
- debugfs
- production hardening
- footprint tradeoffs

## Mental Model

Use debug configs to find classes of bugs that production configs may hide. Promote only deliberate, documented options into production.

```text
debug config:
  higher overhead
  better evidence
  stronger internal checking
  acceptable diagnostic exposure in lab

production config:
  lower overhead
  smaller attack surface
  stable boot/runtime behavior
  controlled diagnostic interfaces
```

Do not tune a production kernel by randomly removing options until it boots. Build from a policy.

## Profile Categories

| Profile | Purpose | Typical Characteristics |
| --- | --- | --- |
| bring-up | first board boot and driver development | verbose logs, debugfs, tracing, permissive boot |
| race/lifetime debug | concurrency and memory bug finding | lockdep, KASAN/KCSAN, debug objects |
| performance debug | latency and throughput analysis | tracing/perf with limited sanitizers |
| field diagnostic | support image or temporary service build | selected diagnostics, controlled access |
| production | shipped product | hardening, minimal exposure, tested watchdog/recovery |

One debug config cannot optimize for everything. KASAN, KCSAN, lockdep, tracing, and perf can interact with timing and overhead. Keep purpose-specific fragments.

## Debug Options Worth Knowing

| Area | Example Options | Use |
| --- | --- | --- |
| symbols | `CONFIG_DEBUG_INFO`, `CONFIG_DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT` | symbolized crashes and source-level debugging |
| module/debug metadata | `CONFIG_KALLSYMS`, `CONFIG_KALLSYMS_ALL` | better stack traces and symbol lookup |
| lock bugs | `CONFIG_LOCKDEP`, `CONFIG_PROVE_LOCKING` | lock ordering and IRQ-lock issues |
| sleepability | `CONFIG_DEBUG_ATOMIC_SLEEP` | sleeping in atomic context |
| memory lifetime | `CONFIG_KASAN`, `CONFIG_KFENCE` | use-after-free and bounds bugs |
| data races | `CONFIG_KCSAN` | unsynchronized access reports |
| undefined behavior | `CONFIG_UBSAN` | integer and C undefined behavior checks |
| object lifecycle | `CONFIG_DEBUG_OBJECTS`, timer/work debug options | active object misuse |
| tracing | `CONFIG_FTRACE`, `CONFIG_FUNCTION_TRACER`, `CONFIG_TRACEPOINTS` | runtime tracing |
| dynamic logs | `CONFIG_DYNAMIC_DEBUG` | selective driver logging |
| debug filesystem | `CONFIG_DEBUG_FS` | subsystem-specific diagnostic files |

Exact symbol names and dependencies can vary by kernel version. Always check the final `.config`.

## Example Debug Fragment

```text
CONFIG_KALLSYMS=y
CONFIG_KALLSYMS_ALL=y
CONFIG_DEBUG_INFO=y
CONFIG_DYNAMIC_DEBUG=y
CONFIG_DEBUG_FS=y
CONFIG_FTRACE=y
CONFIG_FUNCTION_TRACER=y
CONFIG_TRACEPOINTS=y
CONFIG_LOCKDEP=y
CONFIG_PROVE_LOCKING=y
CONFIG_DEBUG_ATOMIC_SLEEP=y
CONFIG_KASAN=y
```

This is not a universal fragment. It is a starting point for a lab kernel. Some options may be unavailable or too expensive on small targets.

## Example Production Fragment

```text
# CONFIG_DEBUG_FS is not set
# CONFIG_KASAN is not set
# CONFIG_KCSAN is not set
# CONFIG_PROVE_LOCKING is not set
CONFIG_STACKPROTECTOR=y
CONFIG_STACKPROTECTOR_STRONG=y
CONFIG_HARDENED_USERCOPY=y
CONFIG_STRICT_KERNEL_RWX=y
CONFIG_STRICT_MODULE_RWX=y
```

Production may still keep selected evidence features:

- useful symbol tables for support, depending on exposure policy
- persistent logs
- panic/oops behavior
- watchdog reset reason capture
- limited tracepoints if the product requires observability

The rule is not "disable all diagnostics." The rule is "ship only intentional diagnostics."

## Debugfs Policy

`debugfs` is valuable for development and dangerous as an accidental production interface.

Questions:

- Is debugfs mounted in production?
- Which services can access it?
- Do drivers expose sensitive registers, buffers, keys, or physical addresses?
- Does a field diagnostic image enable it temporarily?
- Is there a documented support procedure?

Possible policies:

| Product Policy | Meaning |
| --- | --- |
| lab only | enabled only in debug builds |
| service only | enabled in controlled diagnostic images |
| production disabled | not built or not mounted |
| production mounted read-only | unusual; requires strong justification |

Do not use debugfs as a stable userspace ABI.

## Debug Symbols And Artifacts

A production image can avoid shipping debug symbols while still archiving them for crash analysis.

Archive:

```text
vmlinux
System.map
modules with debug info
final .config
module symbol/version files
exact source revision
```

Strip or split shipped artifacts according to product policy, but keep the unstripped artifacts in the release archive.

## Runtime Debug Switches

Some debug features are compiled in but activated at runtime:

- dynamic debug controls
- ftrace instances
- trace events
- boot command-line log level
- sysctl panic behavior
- module parameters

Document which runtime switches are allowed in production. A compiled feature can still be a support risk if it is exposed without access control.

## Performance And Timing Impact

Debug configs can change behavior:

- KASAN increases memory use and slows code
- KCSAN changes timing to detect races
- lockdep adds lock tracking overhead
- tracing can affect latency
- debug logging can hide timing bugs or create new ones

When a bug appears only in production, rerun with incremental debug options. When a bug appears only in debug, check timing sensitivity before dismissing it.

## Comparison Workflow

For every profile:

```text
build final .config
archive final .config
measure kernel image size
measure boot time
run driver smoke tests
run stress tests
capture dmesg
compare config diff against baseline
```

Useful checks:

```sh
grep '^CONFIG_KASAN' build/.config
grep '^CONFIG_DEBUG_FS' build/.config
grep '^CONFIG_MODULE_SIG' build/.config
```

Use structured CI checks for required and forbidden symbols.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| debug option missing | unmet dependency or wrong fragment order | final `.config` |
| production image exposes debugfs | debug fragment included by mistake | mount table and config |
| crash cannot be symbolized | debug artifacts not archived | release artifacts |
| debug build too slow to boot | heavy sanitizer or tracing profile | profile purpose |
| bug disappears under debug | timing changed | reduce options incrementally |
| field support cannot inspect issue | production removed all evidence paths | diagnostic image plan |

## Practice Exercises

### Exercise 1: Build Two Profiles

Create:

```text
debug.cfg
production.cfg
```

Build both, archive final `.config`, and compare:

```text
kernel image size
boot time
enabled debug features
enabled hardening features
mounted diagnostic filesystems
```

### Exercise 2: Driver Bug Matrix

Run the same driver test on:

```text
production
lockdep debug
KASAN debug
KCSAN debug
tracing debug
```

Record which profile exposes the failure.

### Exercise 3: Production Exposure Audit

List every debug interface available in a production boot and decide whether it is intentional.

## Debugging Checklist

- Confirm requested debug options survive dependency resolution.
- Check runtime overhead.
- Keep debugfs policy explicit.
- Do not ship accidental diagnostic exposure.
- Archive unstripped debug artifacts for every release.
- Separate debug and production fragments.
- Run the same functional tests on both profiles.
- Document runtime debug switches that field support may use.

## Related Topics

- [Config Review Workflow](config-review-workflow.md)
- [Configuration Fragments And Auditing](../../build-systems/advanced/linux-kernel/configuration-fragments-and-auditing.md)
- [Kernel Debugging Basics](../debugging/index.md)
- [Oops, Panic, And Crash Logs](../debugging/oops-panic-crash-logs.md)

## Official References

- [Kconfig Language](https://docs.kernel.org/kbuild/kconfig-language.html)
- [Kernel hacking guides](https://docs.kernel.org/kernel-hacking/index.html)
