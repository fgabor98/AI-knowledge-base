---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Stage 10: Hardware-Facing Userspace And Kernel UAPI

Use documented kernel interfaces to control and observe hardware without confusing a
device node, sysfs attribute, `ioctl`, or memory mapping with the hardware itself.
The driver owns privileged access and translates a stable UAPI into bus transactions,
interrupt handling, power management, and device-specific behavior.

## The boundary

```text
userspace: open/read/write/ioctl/poll/mmap
        -> kernel UAPI validation and lifetime
        -> subsystem and driver
        -> bus/power/firmware
        -> hardware
```

Every UAPI needs version, permissions, blocking, cancellation, ownership, and
recovery rules. A successful call proves only what the UAPI promises.

## Learning materials

1. [Kernel Versus Userspace Boundary](kernel-versus-userspace-boundary.md)
2. [devfs, sysfs, udev, And Device Discovery](devfs-sysfs-udev-and-discovery.md)
3. [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)
4. [ioctl ABI And Compatibility](ioctl-abi-and-compatibility.md)
5. [poll, mmap, And Device Events](poll-mmap-and-device-events.md)
6. [GPIO, I2C, And SPI Userspace](gpio-i2c-and-spi-userspace.md)
7. [Serial, Input, Sensors, And hwmon](serial-input-sensors-and-hwmon.md)
8. [CAN, Watchdog, And Control Interfaces](can-watchdog-and-control-interfaces.md)
9. [Specialized UAPI: V4L2, ALSA, DRM, UIO, And VFIO](specialized-uapi-v4l2-alsa-drm-uio-vfio.md)

## UAPI review table

| Question | Evidence |
| --- | --- |
| Discovery | Device-tree/sysfs identity, uevent, stable subsystem name |
| Access | UID/GID, mode, ACL, capability, service sandbox |
| ABI | Headers, ioctl encoding, fixed widths, reserved fields, compat rules |
| I/O | Blocking, short result, readiness, interrupt, timeout |
| Ownership | FD, buffer, mapped range, queue, hardware claim |
| Recovery | Reset, reopen, reconfigure, unplug, firmware, power cycle |
| Version | Kernel/subsystem version, feature query, protocol generation |
| Evidence | UAPI result, kernel log, sysfs state, timing, device trace |

## Completion criteria

You can complete this stage when you can select a subsystem UAPI, inspect its identity
and permissions, perform bounded operations, and explain what remains unproven about
the physical device. You can also reject an unsafe design that depends on private
driver structure, `/dev/mem`, undocumented sysfs behavior, or raw physical addresses.

## Related topics

- [Stage 9: Userspace Networking](../userspace-networking/index.md)
- [Stage 11: Services, Init, And systemd](../services-init-and-systemd/index.md)
- [Device Tree](../../device-tree/index.md)
- [Linux Kernel Configuration](../../linux-kernel/configuration-and-platform-policy/index.md)

## References

- [Linux kernel userspace API](https://www.kernel.org/doc/html/latest/userspace-api/index.html)
- [Linux kernel driver API](https://www.kernel.org/doc/html/latest/driver-api/index.html)
- [Linux kernel ABI documentation](https://www.kernel.org/doc/html/latest/admin-guide/abi.html)
