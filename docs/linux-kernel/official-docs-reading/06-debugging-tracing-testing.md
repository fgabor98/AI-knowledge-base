---
status: active
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# 6. Debugging, Tracing, And Testing

Official sections: [Development tools](https://docs.kernel.org/dev-tools/index.html),
[Testing](https://docs.kernel.org/dev-tools/testing-overview.html),
[Tracing](https://docs.kernel.org/trace/index.html), and
[Fault injection](https://docs.kernel.org/fault-injection/index.html)

## Debugging Fundamentals

- [ ] **P0** [Debugging advice for kernel developers](https://docs.kernel.org/process/debugging/index.html)
- [ ] **P0** [Bug hunting](https://docs.kernel.org/admin-guide/bug-hunting.html)
- [ ] **P0** [Dynamic debug HOWTO](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)
- [ ] **P0** [DebugFS](https://docs.kernel.org/filesystems/debugfs.html)
- [ ] **P0** [Sysfs](https://docs.kernel.org/filesystems/sysfs.html)
- [ ] **P0** [Magic SysRq](https://docs.kernel.org/admin-guide/sysrq.html)
- [ ] **P1** [KGDB and KDB](https://docs.kernel.org/dev-tools/kgdb.html)
- [ ] **P1** [Ramoops persistent logs](https://docs.kernel.org/admin-guide/ramoops.html)
- [ ] **P1** [Kdump](https://docs.kernel.org/admin-guide/kdump/kdump.html)

## Ftrace, Events, And Probes

- [ ] **P0** [Linux tracing technologies guide](https://docs.kernel.org/trace/index.html)
- [ ] **P0** [ftrace](https://docs.kernel.org/trace/ftrace.html)
- [ ] **P0** [Tracepoint API](https://docs.kernel.org/core-api/tracepoint.html)
- [ ] **P0** [Trace events](https://docs.kernel.org/trace/events.html)
- [ ] **P0** [Event tracing](https://docs.kernel.org/trace/events.html#using-event-tracing)
- [ ] **P0** [Kprobe event tracing](https://docs.kernel.org/trace/kprobetrace.html)
- [ ] **P1** [Function error injection](https://docs.kernel.org/fault-injection/fault-injection.html)
- [ ] **P1** [Histogram triggers](https://docs.kernel.org/trace/histogram.html)
- [ ] **P1** [Boot-time tracing](https://docs.kernel.org/trace/boottime-trace.html)
- [ ] **P1** Read the `trace-cmd` man pages supplied with the exact userspace tool version used in the lab.
- [ ] **P1** [Kernel probes](https://docs.kernel.org/trace/kprobes.html)

## Sanitizers And Dynamic Checkers

- [ ] **P0** [KASAN](https://docs.kernel.org/dev-tools/kasan.html)
- [ ] **P0** [KCSAN](https://docs.kernel.org/dev-tools/kcsan.html)
- [ ] **P0** [Lockdep design](https://docs.kernel.org/locking/lockdep-design.html)
- [ ] **P0** [Kernel memory leak detector](https://docs.kernel.org/dev-tools/kmemleak.html)
- [ ] **P1** [UBSAN](https://docs.kernel.org/dev-tools/ubsan.html)
- [ ] **P1** [KFENCE](https://docs.kernel.org/dev-tools/kfence.html)
- [ ] **P1** [KMSAN](https://docs.kernel.org/dev-tools/kmsan.html)

## Tests And Fault Injection

- [ ] **P0** [Kernel testing guide](https://docs.kernel.org/dev-tools/testing-overview.html)
- [ ] **P0** [KUnit](https://docs.kernel.org/dev-tools/kunit/index.html)
- [ ] **P0** [Kselftest](https://docs.kernel.org/dev-tools/kselftest.html)
- [ ] **P0** [Fault injection](https://docs.kernel.org/fault-injection/index.html)
- [ ] **P0** [Fault-injection capabilities](https://docs.kernel.org/fault-injection/fault-injection.html)
- [ ] **P1** [Error-inject framework](https://docs.kernel.org/fault-injection/notifier-error-inject.html)
- [ ] **P1** [Device fault injection](https://docs.kernel.org/fault-injection/fault-injection.html)

## Embedded Driver Labs

- [ ] Prove a successful match and probe using dmesg, sysfs, and trace events.
- [ ] Diagnose an intentionally wrong `compatible` string.
- [ ] Diagnose a missing clock, regulator, GPIO, IRQ, and firmware file.
- [ ] Observe and explain one deferred-probe sequence.
- [ ] Trace hard IRQ entry, threaded handler, workqueue activity, and userspace notification.
- [ ] Inject an allocation or I/O failure and verify cleanup.
- [ ] Test repeated bind/unbind or module load/unload where hardware permits it.
- [ ] Capture an oops over serial and resolve symbols against the exact `vmlinux`.
- [ ] Persist logs across a watchdog reset or kernel panic.
- [ ] Compare debug and production kernel configurations for observability and overhead.
