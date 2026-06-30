---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kernel Configuration And Platform Policy

This track covers product-level choices that shape kernel behavior beyond individual driver code.

It assumes you already know:

- [Kernel Source, Build, And Tailoring](../source-build-and-tailoring/index.md)
- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Kernel Execution And Concurrency](../execution-and-concurrency/index.md)

## What Problem Does This Solve?

Kernel configuration is not just a build detail. It decides what the product can boot, debug, secure, recover, update, and support.

Examples:

- a storage driver built as a module, but no initramfs loads it before rootfs mount
- a debug config shipped with debugfs and aggressive tracing enabled
- a production image missing the watchdog driver
- a secure-boot product that cannot load field service modules because signing policy was not planned
- a command line that silently disables an LSM or changes console logging
- a container runtime that expects namespaces or cgroup controllers that the kernel did not enable
- a release built from fragments whose final `.config` differs from the requested policy

This chapter gives a product-policy view of kernel configuration. Build mechanics live in the build-system chapter; this track focuses on what to decide, how to review it, and how to avoid accidental product behavior.

## Learning Materials

1. [Debug Vs Production Configs](debug-vs-production-configs.md)
2. [Built-In Vs Module Policy](built-in-vs-module-policy.md)
3. [Kernel Command Line Policy](kernel-command-line-policy.md)
4. [Watchdog Options](watchdog-options.md)
5. [Module Signing And Hardening](module-signing-and-hardening.md)
6. [Namespaces, Cgroups, And LSM Overview](namespaces-cgroups-lsm.md)
7. [Initramfs Options](initramfs-options.md)
8. [Config Review Workflow](config-review-workflow.md)

## Mental Model

Kernel configuration is product policy encoded as technical defaults. Debug, security, boot, module, and recovery choices must be intentional and reviewable.

```text
Kconfig symbols
+ fragments
+ defconfig
+ command line
+ initramfs contents
+ bootloader policy
+ userspace policy
-> observable product behavior
```

Review the final behavior, not only the requested options.

## Policy Layers

| Layer | Examples | Review Question |
| --- | --- | --- |
| SoC/BSP | CPU, interrupt controller, pinctrl, clocks, PMIC | Does the platform boot and enumerate hardware? |
| Board | storage, network PHY, USB, display, regulators | Does this physical board work? |
| Product | filesystems, protocols, namespaces, watchdog | Does the product feature set work? |
| Debug | debugfs, tracing, sanitizers, verbose logs | Is this build meant for diagnosis? |
| Security | module signing, LSMs, hardening, lockdown | Is the attack surface acceptable? |
| Recovery | initramfs, shell, rootfs fallback, watchdog behavior | Can the device recover in the field? |
| Release | archived `.config`, fragments, command line, artifacts | Can this build be reproduced and audited? |

## Configuration Inputs Versus Effective Result

Fragments and defconfigs are inputs. The final `.config` is the effective result for one build.

```text
vendor defconfig
+ board fragment
+ product fragment
+ debug or production fragment
-> Kconfig dependency resolution
-> final .config
```

Kconfig dependencies can change requested values. A requested `CONFIG_FOO=y` can still disappear if dependencies are unmet, the symbol is renamed, the architecture does not support it, or a later fragment overrides it.

## Product Profiles

Most teams need at least three profiles:

| Profile | Purpose |
| --- | --- |
| bring-up/debug | maximize evidence, tolerate overhead |
| diagnostic/service | field-safe diagnostics with bounded exposure |
| production | minimize attack surface and accidental behavior |

Do not let one mutable `.config` serve every purpose. Keep profiles explicit.

Example profile split:

```text
base_defconfig
board.cfg
product.cfg
debug.cfg
production.cfg
service.cfg
```

The build must make it clear which profile was used.

## Review Artifacts

Each release should archive:

- kernel source identity
- base defconfig
- fragment list and order
- final `.config`
- kernel command line
- initramfs manifest
- module list and signing policy
- selected LSM policy
- watchdog policy
- boot logs from a known-good boot
- config diff against previous release

This makes later debugging possible when a failure report says "same hardware, new image."

## Completion Criteria

You are ready to move on when you can:

- separate development, diagnostic, and production kernel configurations
- explain which drivers must be built in and which can be modules
- identify drivers that must live in initramfs if they are modular
- review a kernel command line as product policy
- define watchdog ownership across bootloader, kernel, and userspace
- explain module signing enforcement and exception handling
- identify namespace, cgroup, and LSM dependencies of a userspace stack
- review initramfs contents against boot and recovery requirements
- audit final resolved `.config`, not only requested fragments
- archive the config artifacts needed to reproduce a release

## Common Mistakes

- Treating `.config` as an implementation detail that does not need review.
- Reviewing fragments but not the final resolved config.
- Mixing debug and production settings in one fragment.
- Building rootfs-critical drivers as modules without initramfs support.
- Shipping debug command-line options by accident.
- Enabling a security feature without planning the userspace policy.
- Enabling module signing without planning key ownership and field updates.
- Assuming the bootloader command line is stable without auditing it.
- Treating watchdog behavior as a driver-only decision.

## Related Topics

- [Linux Kernel Build System](../../build-systems/advanced/linux-kernel/index.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Embedded Productization](../../embedded-productization/index.md)
- [Configuration Fragments And Auditing](../../build-systems/advanced/linux-kernel/configuration-fragments-and-auditing.md)

## Official References

- [Kconfig Language](https://docs.kernel.org/kbuild/kconfig-language.html)
- [The kernel command-line parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [Using the initial RAM disk](https://docs.kernel.org/admin-guide/initrd.html)
- [Kernel module signing facility](https://docs.kernel.org/admin-guide/module-signing.html)
