---
status: draft
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# Suspend And Resume

## What Problem Does This Solve?

System suspend and resume coordinate device state transitions across the whole platform.

## Core Concepts

- system sleep
- suspend callbacks
- resume callbacks
- noirq phases
- device ordering
- wakeup sources
- freezer overview
- restore state

## Mental Model

Suspend is a coordinated system transition. A driver must quiesce hardware, preserve or restore state, and leave wakeup behavior intentional.

## Practice Skeleton

- Add basic suspend and resume callbacks.
- Save and restore one hardware register.
- Test repeated suspend cycles.
- Test suspend failure rollback.

## Debugging Checklist

- Check callback order.
- Check active users before suspend.
- Check wakeup configuration.
- Check whether runtime PM state interacts with system sleep.

## Related Topics

- [Runtime PM](runtime-pm.md)
- [Wake Sources](wake-sources.md)
- [Suspend And Resume Debugging](suspend-resume-debugging.md)
