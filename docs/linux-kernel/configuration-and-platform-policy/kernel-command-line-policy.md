---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Command Line Policy

## What Problem Does This Solve?

The kernel command line controls boot-time behavior, device initialization, root filesystem selection, logging, security, and diagnostics.

It is part of the product ABI between bootloader, kernel, initramfs, and userspace. A one-word change can alter which console is visible, whether rootfs mounts, how much evidence is logged, which LSM is active, or whether the system reboots after panic.

## Core Concepts

- `console=`
- `root=`
- `rootwait`
- `earlycon`
- log level
- init path
- panic behavior
- LSM parameters
- product-owned defaults

## Mental Model

The command line is part of the boot contract between bootloader, kernel, and product image. Treat it as versioned platform configuration.

```text
bootloader environment
+ boot script
+ device tree chosen stdout path
+ kernel built-in command line options
+ distribution defaults
-> /proc/cmdline
-> runtime boot behavior
```

Review `/proc/cmdline`, not only the bootloader script you think is active.

## Capture The Effective Command Line

Runtime truth:

```sh
cat /proc/cmdline
```

Boot evidence:

```sh
dmesg | grep -i 'Kernel command line'
```

Archive both for releases. Some boot chains append arguments from several places, so the final string can differ from the source file you edited.

## Ownership Model

Assign ownership by category:

| Category | Examples | Owner |
| --- | --- | --- |
| console/logging | `console=`, `earlycon`, `loglevel=` | BSP/platform and support |
| rootfs | `root=`, `rootwait`, `rootfstype=` | platform/storage owner |
| init | `init=`, `rdinit=` | distribution/product owner |
| panic/recovery | `panic=`, `oops=panic`, watchdog-related policy | reliability owner |
| security | `security=`, `lsm=`, lockdown-related args | security owner |
| debug | `ignore_loglevel`, subsystem debug args | development/support |
| hardware quirks | driver-specific parameters | BSP owner |

Do not let debug arguments become permanent because nobody owns them.

## Console And Early Logging

Common arguments:

```text
console=ttyS0,115200n8
console=tty0
earlycon
loglevel=7
ignore_loglevel
printk.time=1
```

Rules:

- use the correct serial device name for the platform
- keep baud rate explicit for serial consoles
- use `earlycon` for early boot bring-up, not automatically in production
- decide whether production should expose a shell or only logs
- make duplicate `console=` entries intentional

Multiple console arguments can be valid, but they must be understood. The visible login console and kernel log console behavior can differ depending on ordering and userspace.

## Root Filesystem Arguments

Common arguments:

```text
root=/dev/mmcblk0p2
root=PARTUUID=12345678-02
root=UUID=...
rootwait
rootfstype=ext4
ro
rw
```

Prefer stable identifiers such as `PARTUUID=` or filesystem UUID when device enumeration order can vary.

Use `rootwait` when the root block device can appear asynchronously, such as MMC, USB, or some storage stacks.

Rootfs policy must match built-in/module/initramfs policy:

```text
root device driver built in?
root filesystem built in?
initramfs loads storage modules?
firmware available before root mount?
```

## Init And Initramfs Arguments

Common arguments:

```text
init=/sbin/init
rdinit=/init
```

`init=` chooses the init process after the real root filesystem is mounted. `rdinit=` is used for initramfs early userspace.

For debugging:

```text
init=/bin/sh
```

This is useful in a lab and usually unacceptable in production.

## Panic And Recovery Arguments

Common examples:

```text
panic=10
oops=panic
panic_on_warn=1
softlockup_panic=1
nmi_watchdog=1
```

Policy questions:

- Should the device reboot automatically after panic?
- How long should it wait so logs can flush?
- Should warnings panic in production, or only in CI?
- Does the watchdog reset faster than panic timeout?
- Is reset reason preserved after reboot?

Use panic-on-warning carefully. It is useful in CI and some safety profiles, but a noisy warning path can turn into a reboot loop.

## Security Arguments

Common areas:

```text
security=selinux
lsm=landlock,lockdown,yama,integrity,apparmor,bpf
selinux=1
apparmor=1
audit=1
```

Exact options depend on kernel configuration and distribution policy.

Rules:

- do not select an LSM on the command line without enabling its kernel config
- verify active LSMs at runtime
- archive command-line security arguments
- test userspace policy loading
- treat disabling arguments as release blockers unless explicitly approved

Runtime check:

```sh
cat /sys/kernel/security/lsm
```

## Debug Arguments

Useful during bring-up:

```text
ignore_loglevel
initcall_debug
dyndbg="file drivers/foo/* +p"
module_blacklist=demo
```

Product rule:

```text
debug arguments require owner, purpose, expiration, and production status
```

If a debug argument remains for months, either promote it to documented policy or remove it.

## Built-In Command Line

Some platforms use kernel configuration options to provide or force a built-in command line.

Questions:

- Does the bootloader pass a command line?
- Does the kernel append to it?
- Does the kernel override it?
- Does Device Tree provide bootargs?
- Which source wins?

Review:

```sh
grep '^CONFIG_CMDLINE' build/.config
cat /proc/cmdline
```

Be cautious with forced built-in command lines. They can make bootloader changes appear ineffective.

## Duplicate And Conflicting Arguments

Examples:

```text
console=ttyS0,115200 console=ttyS1,115200
root=/dev/mmcblk0p2 root=PARTUUID=...
quiet loglevel=7 ignore_loglevel
security=selinux lsm=...
```

Some duplicates are valid. Some are accidental. The policy is to document them and test the effective result.

## Development Versus Production Examples

Bring-up:

```text
console=ttyS0,115200n8 earlycon ignore_loglevel printk.time=1 rootwait root=PARTUUID=...
```

Production:

```text
console=ttyS0,115200n8 rootwait root=PARTUUID=... ro panic=10
```

Diagnostic/service:

```text
console=ttyS0,115200n8 rootwait root=PARTUUID=... loglevel=7 systemd.log_level=debug
```

Do not copy these blindly. They illustrate profile separation.

## Review Workflow

For every release:

```text
capture /proc/cmdline
classify each argument
assign owner
compare with previous release
compare with expected profile
boot with minimal required command line in lab
verify logs, rootfs, init, security, watchdog
```

Keep a command-line manifest:

```text
argument: rootwait
owner: platform storage
reason: eMMC enumeration can be asynchronous
production: yes
debug-only: no
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no console output | wrong `console=` or missing driver | `/proc/cmdline`, DT stdout-path |
| root mount panic | wrong `root=`, missing `rootwait`, missing driver | boot log |
| initramfs shell instead of product boot | `rdinit=` or initramfs failure | early userspace logs |
| production logs too verbose | debug args left in command line | command-line diff |
| LSM not active | config missing or wrong `security=`/`lsm=` | `/sys/kernel/security/lsm` |
| bootloader edit has no effect | built-in command line overrides | `CONFIG_CMDLINE*` |

## Practice Exercises

### Exercise 1: Runtime Audit

Capture:

```sh
cat /proc/cmdline
dmesg | grep -i 'Kernel command line'
```

For every argument, record:

```text
owner
purpose
debug or production
expected source
runtime effect
```

### Exercise 2: Minimal Boot

Remove every nonessential argument in a lab boot. Add arguments back only when a failure proves they are needed.

### Exercise 3: Security Check

Change only the security-related command-line arguments in a lab image and verify active LSMs and policy loading.

## Debugging Checklist

- Check `/proc/cmdline`.
- Check bootloader environment or boot script.
- Check duplicate or conflicting arguments.
- Verify console and rootfs arguments first.
- Check built-in `CONFIG_CMDLINE*` behavior.
- Check Device Tree bootargs where applicable.
- Compare development, service, and production variants.
- Archive command line with release artifacts.

## Related Topics

- [Embedded Linux](../../embedded-linux/index.md)
- [Debug Vs Production Configs](debug-vs-production-configs.md)
- [Initramfs Options](initramfs-options.md)
- [Watchdog Options](watchdog-options.md)

## Official References

- [The kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [Using the initial RAM disk](https://docs.kernel.org/admin-guide/initrd.html)
- [Linux Security Module Usage](https://docs.kernel.org/admin-guide/LSM/index.html)
