---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# PID 1, Init, And Early Userspace

## What problem does this solve?

The kernel does not start an ordinary application directly. PID 1 establishes early
userspace, mounts, credentials, reaping, and service supervision. If it lacks signal
and child-reaping behavior, the system can fail even when individual programs are
correct.

## Boot sequence

```text
firmware -> kernel -> initramfs /init -> real root -> PID 1
        -> pseudo-filesystems -> devices/mounts -> services
```

Early userspace may load modules, unlock storage, select a root slot, mount `/proc`,
`/sys`, and `/dev`, and hand off to the final init. Files available in initramfs may
not exist after handoff.

## PID 1 obligations

PID 1 must reap orphaned children and establish signal/service policy. It should
propagate shutdown, stop descendants, preserve logs, and provide a bounded restart
policy. In a container, the container’s PID 1 has these local obligations even if the
host has another init.

## Service readiness

Starting a process is not readiness. A service should report readiness only after
required configuration, mounts, devices, threads, and listeners are valid. Dependents
must wait for that contract rather than sleep for an arbitrary time.

## Debugging

```sh
ps -p 1 -o pid,ppid,stat,cmd
cat /proc/1/status
cat /proc/1/mountinfo
dmesg | sed -n '1,100p'
```

Compare initramfs logs, kernel command line, final-root mount state, PID 1 logs, and
the service environment. A missing runtime path may be a handoff or mount-order bug.

## Common mistakes

- Treating PID 1 as an ordinary process.
- Forgetting child reaping in a minimal init/container.
- Starting services before mounts/devices/configuration are ready.
- Assuming initramfs files survive root handoff.
- Using arbitrary sleeps instead of readiness signals.

## Debugging checklist

- Record kernel/initramfs/PID 1 versions and command line.
- Identify root handoff, mounts, namespaces, service user, and child ownership.
- Test delayed storage/device, failed root selection, orphan children, and shutdown.
- Verify readiness and restart evidence survives a service crash.

## Related topics

- [Stage 11: Services, Init, And systemd](index.md)
- [Mounts, Initramfs, And Rootfs Layout](../linux-runtime-filesystem-and-rootfs/mounts-initramfs-and-rootfs-layout.md)
- [Exit, Waiting, And Zombies](../processes-and-program-lifetime/exit-waiting-and-zombies.md)

## References

- [`bootup(7)`](https://www.freedesktop.org/software/systemd/man/latest/bootup.html)
- [`pid_namespaces(7)`](https://man7.org/linux/man-pages/man7/pid_namespaces.7.html)
- [Linux early userspace](https://www.kernel.org/doc/html/latest/driver-api/early-userspace/early_userspace_support.html)
