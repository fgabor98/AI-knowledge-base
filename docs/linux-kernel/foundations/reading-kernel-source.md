---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Reading Kernel Source

## What Problem Does This Solve?

Kernel source is large. Beginners need a method for reading it without trying to understand the whole tree at once.

## Core Concepts

- source tree layout
- `drivers/`
- `include/linux/`
- `Documentation/`
- `git grep`
- `rg`
- call chains
- struct-first reading
- registration paths
- subsystem examples

## Mental Model

Start from one concrete object: a driver, a struct, a callback table, or a log message. Follow ownership and registration before reading implementation details.

## Practice Skeleton

- Pick one simple platform driver.
- Locate its Kconfig and Makefile entry.
- Find its `of_match_table`.
- Follow registration into `probe`.
- Map driver state from allocation to cleanup.

## Debugging Checklist

- Search for the exact log message first.
- Read structs before helper functions.
- Follow callback registration sites.
- Compare a target driver with a known-good driver in the same subsystem.

## Related Topics

- [Kernel Source Acquisition](../source-build-and-tailoring/kernel-source-acquisition.md)
- [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [Kernel Documentation Reading Guide For Beginners](kernel-documentation-reading-guide-for-beginners.md)
