---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Lifecycle, Update, And Hardware-In-The-Loop Tests

Long-lived failures appear only across restarts, updates, resource pressure, and physical disturbances. Build tests around events, not only steady-state success.

## Lifecycle scenarios

Test start, readiness, reload, dependency loss, reconnect, graceful stop, forced termination, watchdog expiry, crash loop limits, log rotation, full storage, clock changes, and repeated restart. Assert no orphaned process, descriptor, mount, lock, or stale socket remains.

## Update and power scenarios

Interrupt download, verification, installation, migration, boot selection, first boot, and confirmation. Power-cycle at each durable transition. Verify old-valid, new-valid, fallback, and safe-recovery outcomes; inspect that configuration and state remain compatible with the selected image.

## HIL discipline

Automate power, reset, network, thermal, sensor, and peripheral faults with explicit safety limits. Use controlled repetition and record board identity, firmware, image digest, stimulus timeline, serial/kernel logs, telemetry, and final state. Separate flaky infrastructure from product failures by validating the fixture itself and rerunning a minimized scenario.

Soak tests should have a resource budget and an end condition: monitor memory, descriptors, threads, CPU, storage, latency tails, and error counters. A test passes only when the artifact bundle makes the result independently reviewable.

## Define expected outcomes at interruption points

| Injection point | Required observation |
| --- | --- |
| SIGTERM with queued work | Admission stops; documented drain/cancel policy; bounded exit |
| SIGKILL during persistence | Startup selects a validated record and reports recovery |
| Device reset after command submission | Outstanding result becomes reconciled or unknown |
| Power loss before slot selection | Previously confirmed image remains bootable |
| Power loss during trial boot | Attempt budget and fallback policy remain effective |
| Rollback after migration | Selected binary can read the chosen state schema |
| Missing watchdog progress | Intended owner detects failure and preserves available evidence |

Process kill leaves the kernel and storage powered, whereas a power cut does
not. A warm reboot, cold boot, brownout and relay interruption also stress
different paths. Record the exact stimulus and which power rails/peripherals
were affected; a USB cable may unintentionally keep a board powered.

## Distinguish recovery from masking the failure

Do not let the test harness repair the target before collecting the fault.
Capture the selected slot, boot attempts, state generation, reset cause,
serial output and service result first. A test that automatically reflashes
on every failed boot can hide a broken rollback mechanism.

For a soak test, define duration, offered workload, lifecycle-event frequency,
and acceptable trends for memory, descriptors, queue age and error counts.
Use a baseline after warm-up and account for legitimate caches and log rotation.
Repeatedly retrying flaky tests until one passes loses failure-rate evidence:
report attempts and keep a reproducible minimized failure.

Hardware safety limits, emergency stop and fixture self-checks belong to the
test design. Validate the fixture can restore the known test state before
launching unattended interruption campaigns.

## Related topics

- [Stage 16 overview](index.md)
- [Evidence and failure classification](../diagnostics-debugging-and-performance/userspace-failure-taxonomy-and-evidence.md)
