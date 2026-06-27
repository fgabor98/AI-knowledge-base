---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Device Classes, Uevents, And udev

## What Problem Does This Solve?

Device classes and uevents let kernel devices appear as stable userspace nodes and metadata through `/dev` and sysfs.

## Core Concepts

- device class
- device node
- major and minor numbers
- uevent
- modalias
- `udev`
- `udevadm info`
- device permissions
- module autoloading

## Mental Model

The kernel creates device identity and events. Userspace policy decides names, permissions, and additional symlinks.

## Practice Skeleton

- Create a class device for a character driver.
- Inspect its sysfs path.
- Inspect the matching `/dev` node.
- Use `udevadm info` to connect kernel identity to userspace metadata.

## Debugging Checklist

- Check major and minor numbers.
- Check uevent contents.
- Check udev rules and permissions.
- Check modalias when module autoloading fails.

## Related Topics

- [Character Device Basics](character-device-basics.md)
- [Platform Devices And Platform Drivers](platform-devices-and-drivers.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)
