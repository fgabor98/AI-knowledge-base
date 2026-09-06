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
