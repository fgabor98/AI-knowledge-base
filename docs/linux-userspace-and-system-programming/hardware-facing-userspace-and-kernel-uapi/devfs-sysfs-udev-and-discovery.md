---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# devfs, sysfs, udev, And Device Discovery

## What problem does this solve?

The presence of `/dev/foo` is not a complete device-discovery proof. `devtmpfs`
creates kernel device nodes; udev/mdev or a static policy names and permissions them;
sysfs represents the kernel device model. Discovery must identify the actual device,
driver, capabilities, and readiness.

## Discovery path

```text
hardware/device-tree description
  -> driver probe and sysfs device model
  -> devtmpfs node / uevent
  -> udev/mdev policy and stable name
  -> service opens and queries capabilities
```

A service should prefer a stable subsystem identity or a product-managed symlink over
`/dev/ttyUSB0`-style enumeration order. Verify serial number, sysfs path, compatible
attribute, or another identity before using the device.

## sysfs and uevents

Sysfs paths and attributes are kernel UAPI. Read units, valid values, permissions,
and lifetime from subsystem documentation. Uevents notify that topology/state may
have changed; they do not replace reading authoritative state. A remove event can
race with an open FD, so the service needs disconnect/recovery behavior.

```sh
find /sys/class -maxdepth 2 -type l | head
udevadm info --query=all --path=/sys/class/net/lo 2>/dev/null || true
udevadm monitor --kernel --udev 2>/dev/null || true
```

## Device nodes and permissions

Inspect type, major/minor, owner, mode, ACL, and namespace. A static `/dev` image may
be correct for a fixed appliance; dynamic devtmpfs plus udev suits hotplug systems.
Do not create nodes manually as a runtime workaround without knowing the driver and
security policy.

## Startup and hotplug

Open only after required device readiness. If a device can disappear, handle
`ENODEV`, HUP, reset, and stale configuration. Do not rely on one udev event to prove
firmware load, calibration, or power readiness; query the subsystem.

## Common mistakes

- Hard-coding unstable enumeration names.
- Treating a directory or node as proof of driver binding.
- Parsing sysfs without units/range/version validation.
- Letting udev rules grant broad group/device access.
- Ignoring hotplug and namespace differences.

## Debugging checklist

- Capture sysfs path, compatible/serial identity, driver link, node, and permissions.
- Check uevents, kernel logs, device-tree status, firmware, and power state.
- Reproduce delayed probe, remove/add, reset, and permission changes.
- Compare service namespace with the host namespace.
- Query capabilities after opening and before enabling hardware.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [Pseudo-filesystems And Device Nodes](../linux-runtime-filesystem-and-rootfs/pseudo-filesystems-and-device-nodes.md)
- [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)
- [Linux Kernel Configuration](../../linux-kernel/configuration-and-platform-policy/index.md)

## References

- [`udev(7)`](https://man7.org/linux/man-pages/man7/udev.7.html)
- [Linux kernel sysfs documentation](https://www.kernel.org/doc/html/latest/filesystems/sysfs.html)
- [Linux kernel ABI documentation](https://www.kernel.org/doc/html/latest/admin-guide/abi.html)
