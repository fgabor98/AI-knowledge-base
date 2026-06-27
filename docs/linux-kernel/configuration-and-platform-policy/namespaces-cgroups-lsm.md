---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Namespaces, Cgroups, And LSM Overview

## What Problem Does This Solve?

Namespaces, cgroups, and Linux Security Modules provide isolation, resource control, and security policy enforcement.

## Core Concepts

- namespaces
- cgroups
- capabilities
- SELinux
- AppArmor
- Landlock overview
- device access control
- container runtime dependencies

## Mental Model

These features are kernel mechanisms used by userspace policy. Enable only what the product or platform actually needs, then validate the userspace stack against it.

## Practice Skeleton

- Identify required namespaces for the target userspace.
- Identify required cgroup controllers.
- Check active LSMs.
- Validate device access restrictions for services.

## Debugging Checklist

- Check kernel config support.
- Check mount points and active controllers.
- Check audit logs or denial logs.
- Distinguish kernel support from userspace policy configuration.

## Related Topics

- [Kernel Command Line Policy](kernel-command-line-policy.md)
- [Embedded Linux](../../embedded-linux/index.md)
- [Embedded Productization](../../embedded-productization/index.md)
