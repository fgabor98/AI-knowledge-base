---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# Modaliases, Module Metadata, And Autoloading

Matching answers whether a registered driver supports a device. Autoloading answers how userspace discovers and loads the module containing that driver. Device aliases connect the two, but they are not themselves proof of a successful match or probe.

## The Autoload Pipeline

```text
DT-backed device is registered
        ↓ kernel emits uevent with MODALIAS
userspace device manager receives event
        ↓ kmod searches module alias database
matching .ko is loaded
        ↓ module registers driver
bus retries matching
        ↓ probe runs if a match exists
```

Systems can vary: modules may be preloaded, built in, omitted from an image, blocked by policy, or loaded by an initramfs before the main userspace device manager starts.

## Device-Side Modalias

A DT-backed platform device commonly exposes:

```sh
cat /sys/bus/platform/devices/DEVICE/modalias
cat /sys/bus/platform/devices/DEVICE/uevent
```

OF modalias strings encode node identity and compatible values in a kernel-defined alias format such as `of:N...T...C...`. Do not construct the value by hand; read sysfs or the uevent.

The modalias exists only after a Linux device is registered. A raw node in `/sys/firmware/devicetree/base` has properties but is not enough to trigger bus-device autoloading.

## Driver-Side Alias Metadata

```c
static const struct of_device_id demo_of_match[] = {
        { .compatible = "example,match-lab" },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

During the kernel build, `modpost` reads supported device tables and emits alias metadata into the module. Inspect it with:

```sh
modinfo -F alias demo.ko
```

After module installation, `depmod` builds alias indexes under `/lib/modules/$(uname -r)/`, including `modules.alias` and binary indexes used by kmod. `modprobe` resolves aliases to module names:

```sh
modprobe --resolve-alias "$(cat /sys/bus/platform/devices/DEVICE/modalias)"
```

Run diagnostic queries without loading anything first. Loading a module changes kernel state and should happen only on an appropriate development target.

## Missing `MODULE_DEVICE_TABLE`

If `.of_match_table` is correct but `MODULE_DEVICE_TABLE` is missing:

- an already built-in driver can still match
- a manually loaded module can register and match
- automatic module discovery from the OF modalias may fail
- `modinfo -F alias` will not show the expected OF alias

This is why “`modprobe demo` fixes it” strongly suggests an autoload/packaging path problem rather than a DT match failure.

## Built-In Drivers

A built-in driver has no load event and may not appear in `lsmod`. Check kernel configuration, boot logs, registered drivers, and the device's `driver` symlink instead.

Kbuild records built-in module information in files such as `modules.builtin` and `modules.builtin.modinfo` so userspace tools can understand that a driver is already part of the kernel. A failed `modprobe` message alone does not prove the driver is absent.

## Packaging And Policy Failures

Correct source can still fail in a product image because:

- the Kconfig symbol is disabled
- the driver is modular but the `.ko` is missing from the root filesystem or initramfs
- module dependencies or alias indexes were not regenerated
- the module was built for another kernel release or ABI
- signature, lockdown, or allowlist policy rejects it
- required firmware is absent after the module loads
- userspace coldplug does not replay events for devices created before it started

Inspect `dmesg`, `modinfo`, the image manifest, module signatures, and the exact `/lib/modules/$(uname -r)` tree.

## Alias Collisions

More than one module can advertise a matching alias. Kbuild records module order so kmod can resolve aliases deterministically, but deterministic does not mean architecturally correct. Compatible contracts should have one owning driver unless a deliberate framework defines otherwise.

Use:

```sh
modprobe --resolve-alias MODALIAS
grep -F 'COMPATIBLE_FRAGMENT' /lib/modules/$(uname -r)/modules.alias
```

If several modules match, fix overlapping ID tables rather than relying on install order or blacklisting as the permanent design.

## Manual Bind And `driver_override`

Sysfs can expose `bind`, `unbind`, and `driver_override`. These interfaces change live driver ownership and can crash, hang, or damage hardware if used incorrectly. They are diagnostic tools for controlled development systems, not a substitute for correct ID tables.

Forced binding does not prove compatibility. It only bypasses normal selection; `probe()` can still fail or, worse, succeed and program incompatible hardware.

## Diagnostic Checklist

1. Does the Linux device expose a modalias?
2. Does `modinfo -F alias` show a compatible module alias?
3. Does alias resolution return exactly the intended module?
4. Is the driver built in or modular?
5. Is the correct module packaged for the running kernel?
6. Did security policy permit loading?
7. Did module initialization register the driver successfully?
8. Did the bus then match and invoke `probe()`?

## Authoritative References

- [Linux DeviceTree modalias and uevent APIs](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux Kbuild output files](https://docs.kernel.org/kbuild/kbuild.html)
- [Linux external-module build process](https://docs.kernel.org/kbuild/modules.html)
- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)

## Next Step

Continue with [Binding-Driven Probe Contracts](binding-driven-probe-contracts.md).
