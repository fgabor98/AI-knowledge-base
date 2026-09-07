---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Stage 17: Design And Architecture Patterns

Good userspace architecture makes ownership, failure, timing, and contracts visible. It does not begin with a favorite framework; it begins with the workload, trust boundaries, lifecycle, and recovery requirements.

## Design sequence

1. State the externally observable requirements and unacceptable failures.
2. Identify processes, privileges, resources, devices, and persistence owners.
3. Choose synchronous or event-driven control flow per boundary.
4. Model states and failure transitions before implementing retries.
5. Define IPC/UAPI/configuration contracts with versioning and observability.
6. Test normal, degraded, restart, update, and power-loss paths.

Prefer simple local structure until concurrency, isolation, independent restart, or resource ownership justifies another process or event loop. Every abstraction should buy a measurable property: testability, containment, throughput, latency, or maintainability.

## Completion checklist

- [ ] Every resource has one clear owner and a shutdown policy.
- [ ] Process boundaries are justified by isolation or lifecycle needs.
- [ ] Backpressure, cancellation, retry, and timeout behavior are explicit.
- [ ] Hardware recovery is modeled as states and events, not scattered flags.
- [ ] Contracts include errors, versioning, limits, and diagnostic context.

## Capstone design review

Combine a device owner, local control protocol, configuration store and service
unit. Draw descriptor/buffer ownership and the device state machine. Walk one
request through startup, success, overload, timeout, disconnect, restart and
update rollback.

Bring the protocol contract, resource/deadline budgets, privilege map and
test evidence to review. Every externally visible outcome must have an owner,
a defined meaning and enough evidence to diagnose it after failure. Reuse the
earlier chapters' concrete mechanisms and record product-specific assumptions
instead of hiding them in generic “retry” or “cleanup” code.

## Learning materials

1. [Utility, Daemon, And Process Boundaries](utility-daemon-and-process-boundaries.md)
2. [Synchronous Versus Event-Driven Architecture](synchronous-versus-event-driven-architecture.md)
3. [Hardware-Service State Machines And Recovery](hardware-service-state-machines-and-recovery.md)
4. [Design Review And Userspace Contracts](design-review-and-userspace-contracts.md)
