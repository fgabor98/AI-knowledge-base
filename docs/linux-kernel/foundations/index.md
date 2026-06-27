---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel Foundations For Driver Developers

This track is the pre-intermediate runway for learning Linux kernel internals and driver development. It is for readers who know C and Linux basics, but are not yet comfortable reading kernel code, loading modules, interpreting probe failures, or debugging kernel runtime behavior.

## Learning Materials

1. [Kernel Mental Model](kernel-mental-model.md)
2. [Kernel C Survival Guide](kernel-c-survival-guide.md)
3. [Reading Kernel Source](reading-kernel-source.md)
4. [Kernel Development Lab Setup](kernel-development-lab-setup.md)
5. [Driver Development Workflow](driver-development-workflow.md)
6. [Debugging Ladder](debugging-ladder.md)
7. [Failure Taxonomy](failure-taxonomy.md)
8. [Execution Context Primer](execution-context-primer.md)
9. [Device Model Primer](device-model-primer.md)
10. [Small Lab Progression](small-lab-progression.md)
11. [Kernel Documentation Reading Guide For Beginners](kernel-documentation-reading-guide-for-beginners.md)

## Mental Model

Kernel development becomes manageable when the learner separates four questions:

```text
what code is running?
-> what object owns the state?
-> what context is it running in?
-> what evidence can prove the failure mode?
```

This track teaches those questions before the learner is expected to understand subsystem-specific APIs.

## Completion Criteria

- Explain kernel space vs user space and why drivers live in the kernel.
- Build, load, inspect, and unload a trivial module in a safe lab.
- Follow a simple driver from registration to probe to userspace-visible state.
- Classify failures as build, load, probe, resource, runtime, crash, hang, or race problems.
- Use logs, sysfs, debugfs, and basic tracing as an ordered debugging ladder.

## Related Topics

- [Kernel Source, Build, And Tailoring](../source-build-and-tailoring/index.md)
- [Linux Device Driver Fundamentals](../fundamentals/index.md)
- [Kernel Debugging Basics](../debugging/index.md)
- [Device Tree](../../device-tree/index.md)
