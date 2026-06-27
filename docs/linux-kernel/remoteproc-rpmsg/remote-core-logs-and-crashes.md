---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Remote Core Logs And Crashes

## What Problem Does This Solve?

Remote-core failures need logs, crash state, and recovery policy that may not look like normal Linux kernel crashes.

## Core Concepts

- remoteproc crash state
- trace buffers
- firmware logs
- coredump overview
- watchdogs
- restart policy
- shared-memory evidence
- product diagnostics

## Mental Model

Remote-core debugging needs evidence from both sides of the boundary: Linux remoteproc state and firmware-side logs or trace buffers.

## Practice Skeleton

- Capture remoteproc state before and after a crash.
- Locate firmware logs.
- Test restart policy in a lab.
- Preserve crash evidence across reboot where possible.

## Debugging Checklist

- Check remoteproc crash messages.
- Check firmware trace buffers.
- Check shared-memory corruption.
- Check whether automatic restart hides the first failure.

## Related Topics

- [Remoteproc Framework](remoteproc-framework.md)
- [Watchdog Reset Diagnosis](../debugging/watchdog-reset-diagnosis.md)
- [Embedded Productization](../../embedded-productization/index.md)
