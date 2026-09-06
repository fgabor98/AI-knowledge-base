# Lifecycle, Update, And Hardware-In-The-Loop Tests

Long-lived failures appear only across restarts, updates, resource pressure, and physical disturbances. Build tests around events, not only steady-state success.

## Lifecycle scenarios

Test start, readiness, reload, dependency loss, reconnect, graceful stop, forced termination, watchdog expiry, crash loop limits, log rotation, full storage, clock changes, and repeated restart. Assert no orphaned process, descriptor, mount, lock, or stale socket remains.

## Update and power scenarios

Interrupt download, verification, installation, migration, boot selection, first boot, and confirmation. Power-cycle at each durable transition. Verify old-valid, new-valid, fallback, and safe-recovery outcomes; inspect that configuration and state remain compatible with the selected image.

## HIL discipline

Automate power, reset, network, thermal, sensor, and peripheral faults with explicit safety limits. Use controlled repetition and record board identity, firmware, image digest, stimulus timeline, serial/kernel logs, telemetry, and final state. Separate flaky infrastructure from product failures by validating the fixture itself and rerunning a minimized scenario.

Soak tests should have a resource budget and an end condition: monitor memory, descriptors, threads, CPU, storage, latency tails, and error counters. A test passes only when the artifact bundle makes the result independently reviewable.
