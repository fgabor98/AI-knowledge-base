---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Kobjects And Sysfs Groups

## What Problem Does This Solve?

Kobjects provide the object model behind sysfs. Attribute groups organize related sysfs files and simplify creation and teardown.

## Core Concepts

- kobject
- kset overview
- sysfs attribute
- attribute group
- show callback
- store callback
- lifetime
- reference ownership

## Mental Model

Sysfs files are attached to kernel objects. The file lifetime must not outlive the object and state used by its callbacks.

## Practice Skeleton

- Create a small kobject-backed sysfs directory.
- Add a group of related attributes.
- Remove the group during teardown.
- Validate callback lifetime.

## Debugging Checklist

- Check object lifetime before exposing files.
- Validate all store input.
- Keep sysfs callbacks short.
- Prefer device attributes when the data belongs to a device.

## Related Topics

- [Sysfs Attributes](sysfs-attributes.md)
- [Pollable Sysfs Attributes](pollable-sysfs-attributes.md)
- [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md)
