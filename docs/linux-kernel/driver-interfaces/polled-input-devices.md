---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Polled Input Devices

## What Problem Does This Solve?

Polled input drivers support devices that do not have a usable interrupt line or where periodic sampling is acceptable.

## Core Concepts

- polling interval
- input polling helper
- GPIO button polling
- debounce overview
- event reporting
- CPU cost
- latency tradeoff

## Mental Model

Polling is simple but costs periodic CPU wakeups and adds latency. Prefer interrupts when hardware supports them cleanly.

## Practice Skeleton

- Create a polled input button.
- Read a GPIO state.
- Report key press and release events.
- Test with `evtest`.

## Debugging Checklist

- Check polling interval.
- Check debounce behavior.
- Check active-low handling.
- Check module dependencies for polling helpers on older kernels.

## Related Topics

- [Input Subsystem](input-subsystem.md)
- [GPIO Consumer API](gpio-consumer-api.md)
- [Timers](../execution-and-concurrency/timers.md)
