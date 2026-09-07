---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

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

## Investigate “the service stopped responding”

Start with an instance identity: executable build ID, boot ID, PID and start
time, configuration generation, and service restart counter. A reused PID or
new service instance can make several observations appear contradictory.

| Observation | Next discriminating observation |
| --- | --- |
| No process | Service result, exit status/signal, previous logs and core |
| High CPU, no completions | Sample stacks; distinguish useful work from a retry/spin loop |
| Low CPU, no completions | All thread stacks, waits, descriptors and queue age |
| One worker absent | Worker lifecycle and OOM events for the whole cgroup |
| Process ready, device missing | Discovery, driver binding, namespace and permission evidence |
| Failure only after update | Binary/library/configuration compatibility and release manifest |

For a hang, take several spaced observations. A thread in `futex` may be
waiting normally or deadlocked; a socket read may be waiting for a peer whose
request was never sent. Identify which state transition was expected next and
which event would enable it.

A useful incident note contains observation, hypothesis, test, result, and
remaining uncertainty. For example: “The response queue grows while the output
FD is never registered writable; restoring interest drains it.” That supports
an event-loop interest bug more directly than a general “network is slow”
description. Preserve the reproducer and add a regression covering that
transition.

## Related topics

- [Stage 14 overview](index.md)
- [Testing and verification](../testing-and-verification/index.md)
