# Userspace Failure Taxonomy And Evidence

## Classify the first broken boundary

- **Input/protocol:** malformed bytes, invalid state transition, or peer disconnect.
- **Application:** assertion, race, lifetime bug, deadlock, or incorrect error handling.
- **ABI/loader:** wrong architecture, missing symbol version, interpreter, library, or startup environment.
- **Deployment:** wrong permissions, paths, user, unit, package, configuration, or working directory.
- **Kernel/UAPI:** unsupported ioctl, changed errno, resource limit, namespace, or policy denial.
- **Device/hardware:** absent device, firmware fault, bus error, thermal event, power loss, or marginal signal.

The visible error is often downstream. A service reporting “device unavailable” may have failed to load a library, while a timeout may be a deadlock rather than a slow device.

## Evidence bundle

Capture the command line and environment policy, executable checksum/build ID, package versions, kernel release, architecture, UID/GID/capabilities, limits, mounts, cgroup, relevant logs, exit status or signal, and a monotonic timeline. Redact secrets deliberately rather than deleting all context.

Make evidence actionable: include the exact command to reproduce, expected versus observed behavior, frequency, first known bad version, and whether tracing changes the result. Store artifacts with retention and access controls appropriate to their contents.

## Hypothesis discipline

Write “If X is the cause, Y should be observable.” Then collect Y. This prevents a large log dump from becoming a substitute for reasoning and makes the eventual fix reviewable.
