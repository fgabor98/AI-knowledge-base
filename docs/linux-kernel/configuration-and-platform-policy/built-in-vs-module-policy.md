---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Built-In Vs Module Policy

## What Problem Does This Solve?

Product teams need consistent rules for which drivers are built into the kernel and which ship as modules.

The `y` versus `m` decision affects boot success, update strategy, security policy, recovery, and support. It is not only a build-size question.

Examples:

- root filesystem driver is a module, but initramfs does not include it
- storage controller is a module, but module signing rejects it in secure boot
- optional USB accessory driver is built in, increasing attack surface permanently
- recovery image lacks the network driver needed for field diagnostics
- firmware needed by a modular driver is unavailable in early userspace

## Core Concepts

- early boot dependencies
- root filesystem dependencies
- initramfs
- optional hardware
- field updates
- module signing
- recovery images
- support diagnostics

## Mental Model

The policy follows boot dependency and update strategy. Root-critical and recovery-critical code usually belongs in the kernel image or initramfs.

```text
needed before initramfs?
  built in

needed to mount real rootfs?
  built in or module inside initramfs

optional after userspace starts?
  module is often acceptable

security policy forbids unsigned modules?
  module signing and key flow must exist

field update requires replacing driver alone?
  module may help, if ABI/signing/support policy allows it
```

## Meaning Of `y`, `m`, And `n`

| Value | Meaning | Product Implication |
| --- | --- | --- |
| `y` | built into the kernel image | available during early boot; updated with kernel |
| `m` | built as a loadable module | can load later; needs module storage, dependencies, and signing policy |
| `n` | not built | feature unavailable |

Kconfig dependencies may prevent a requested `m` or `y`. Review the final `.config`.

## Built-In Candidates

Usually built in:

- CPU/architecture core support
- interrupt controller
- timer/clocksource needed for boot
- pinctrl needed before driver probe
- early console driver when needed for bring-up
- storage bus for rootfs if no initramfs loads it
- root filesystem driver if no initramfs loads it
- initramfs decompressor support
- security infrastructure required before userspace
- watchdog driver if early recovery policy requires it

Embedded rootfs example:

```text
CONFIG_MMC=y
CONFIG_MMC_SDHCI=y
CONFIG_EXT4_FS=y
CONFIG_BLK_DEV_INITRD=y
```

The exact symbols depend on SoC, board, and storage path.

## Module Candidates

Usually reasonable as modules:

- optional USB devices
- field-service diagnostic drivers
- non-root filesystems
- optional sensors
- optional network adapters
- development-only drivers
- features that should be independently updateable

Module policy still needs:

- dependency loading
- firmware availability
- module signing
- version compatibility
- recovery plan when module load fails

## Initramfs Changes The Decision

If initramfs is used, root-critical drivers can be modules only if the initramfs contains:

- modules
- module dependencies
- firmware
- userspace tools to load them
- scripts or init system logic
- correct kernel command-line root discovery

Example policy:

```text
storage host controller: module in initramfs
root filesystem: built in
network driver: module in real rootfs
recovery network driver: module in rescue initramfs
```

Test by removing the module from the real rootfs and confirming the system still boots, or by booting with a minimal initramfs manifest.

## Firmware Availability

Modular and built-in drivers can both require firmware, but timing differs.

Questions:

- Does the driver request firmware during probe?
- Is firmware needed before rootfs is mounted?
- Is firmware built into the kernel, in initramfs, or on rootfs?
- What happens if firmware is missing?
- Is fallback loading disabled in production?

If a built-in driver needs firmware before rootfs, either include firmware in initramfs or use a kernel-supported built-in firmware mechanism where appropriate for the product.

## Security And Module Signing

Modules require a load policy.

Questions:

- Are unsigned modules permitted?
- Are invalid signatures rejected or only tainted?
- Who owns signing keys?
- How are field-service modules signed?
- Can the boot chain enforce module policy?
- Is module loading disabled after boot?

If module signing is strict and the field update process cannot sign modules, `m` may look flexible but fail operationally.

## Update Strategy

Built-in drivers:

- update with the kernel image
- simplify early boot
- reduce runtime module loading paths
- can increase kernel image size
- make field replacement less granular

Modules:

- can be loaded on demand
- can reduce base image footprint
- can support optional hardware
- require version compatibility with the running kernel
- require dependency and signing management
- may complicate recovery

Do not assume modules are easier to update. In many embedded products, the kernel and modules are released as one tested set.

## Recovery Image Policy

A recovery image often needs a different built-in/module set from production.

Recovery may require:

- storage driver for rescue media
- network driver for provisioning
- USB gadget or serial console support
- watchdog support
- filesystem repair tools
- firmware files
- signed modules if enforcement remains active

Document recovery separately:

```text
production boot path:
  eMMC -> ext4 rootfs

recovery boot path:
  initramfs -> USB Ethernet -> signed service module set
```

## Decision Matrix

| Question | If Yes | If No |
| --- | --- | --- |
| Needed before initramfs starts? | built in | continue |
| Needed to find or mount rootfs? | built in or initramfs module | continue |
| Needed for recovery when rootfs is broken? | built in or recovery initramfs | continue |
| Optional after userspace starts? | module candidate | built in candidate |
| Requires field replacement independent of kernel? | module candidate with signing plan | built in acceptable |
| Security policy forbids runtime module load? | built in or disabled | module possible |
| Needs firmware before rootfs? | built in plus firmware plan, or initramfs | module/rootfs possible |

## Testing Policy

Test the decision, not just the config:

```text
boot with normal image
boot with initramfs only
boot with real rootfs module removed
boot with unsigned module under secure policy
boot recovery image
probe optional hardware after userspace starts
run depmod/modprobe path
verify /proc/modules and dmesg
```

Useful commands:

```sh
cat /proc/modules
modinfo demo_driver.ko
modprobe -n -v demo_driver
dmesg | grep -i 'module'
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| kernel panic: cannot mount root | storage/fs driver missing early | `.config`, initramfs |
| module load rejected | signature/key/lockdown policy | dmesg and signing config |
| driver probes too late | built as module but dependency expected early | boot logs |
| firmware missing | not in initramfs/rootfs at request time | firmware path |
| recovery image cannot access storage | recovery config differs from production | recovery manifest |
| optional hardware never loads | missing module alias or depmod data | `modinfo`, `modules.alias` |

## Practice Exercises

### Exercise 1: Boot Dependency Map

For one board, list every driver needed for:

```text
console
boot storage
root filesystem
network recovery
watchdog
firmware loading
```

Mark each as `y`, `m in initramfs`, `m in rootfs`, or `n`.

### Exercise 2: Module Failure Test

Intentionally remove one root-critical module from initramfs or rootfs in a lab image. Confirm the failure mode and log evidence.

### Exercise 3: Signing Policy Test

Attempt to load:

```text
valid signed module
unsigned module
module signed by unknown key
module built for wrong kernel
```

Record expected and actual behavior.

## Debugging Checklist

- Confirm boot-critical storage, filesystem, and bus drivers.
- Check firmware availability.
- Check initramfs contents.
- Check secure boot and signing requirements.
- Check module dependencies and aliases.
- Check final `.config`, not only fragments.
- Check whether recovery images have the same critical drivers.
- Check dmesg for probe timing, firmware, and signature errors.

## Related Topics

- [Built-In Drivers Vs Loadable Modules](../fundamentals/built-in-vs-loadable-modules.md)
- [Initramfs Options](initramfs-options.md)
- [Module Signing And Hardening](module-signing-and-hardening.md)
- [Kernel Build And Install Overview](../source-build-and-tailoring/kernel-build-and-install-overview.md)

## Official References

- [Kconfig Language](https://docs.kernel.org/kbuild/kconfig-language.html)
- [Kernel module signing facility](https://docs.kernel.org/admin-guide/module-signing.html)
- [Using the initial RAM disk](https://docs.kernel.org/admin-guide/initrd.html)
