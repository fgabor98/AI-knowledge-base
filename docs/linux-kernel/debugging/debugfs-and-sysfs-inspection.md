---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Debugfs And Sysfs Inspection

## What Problem Does This Solve?

Runtime filesystems expose kernel and device state for inspection and controlled diagnostics.

## Core Concepts

- sysfs
- debugfs
- configfs overview
- device hierarchy
- driver bind and unbind
- subsystem debug files
- ABI stability
- production exposure

## Mental Model

Sysfs is a stable-ish user ABI surface when documented. Debugfs is diagnostic and should not be required for normal product operation.

## Practice Skeleton

- Locate a device in sysfs.
- Inspect driver bind state.
- Inspect subsystem debugfs files.
- Document which files are safe for diagnostics.

## Debugging Checklist

- Confirm filesystems are mounted.
- Distinguish stable ABI from debug-only state.
- Avoid scripting product behavior around debugfs.
- Check permissions and security policy.

## Related Topics

- [Sysfs Attributes](../fundamentals/sysfs-attributes.md)
- [Dynamic Debug](dynamic-debug.md)
- [Module Signing And Hardening](../configuration-and-platform-policy/module-signing-and-hardening.md)
