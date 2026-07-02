---
status: draft
reviewed: false
domain: debugging
difficulty: beginner
last_reviewed: null
---

# Debugging And Diagnostics

Cross-cutting debugging and diagnostics topics for C, C++, Bash, Python, Linux userspace, kernel work, embedded systems, and product support.

This section should not duplicate the debugging checklists inside each technical topic. It should focus on reusable investigation methods, evidence capture, failure classification, and diagnostic workflows that apply across domains.

## Learning Path

Beginner:

1. failure classification
2. reproducing failures
3. reducing test cases
4. preserving first-failure evidence
5. reading compiler diagnostics
6. logging strategy
7. terminal and command-line evidence capture
8. basic GDB workflow
9. core dumps
10. `strace`
11. serial console workflow
12. lab notes and debug journals

Intermediate:

1. binary search and bisecting
2. hypothesis-driven debugging
3. timeout and hang classification
4. race-condition investigation workflow
5. `ltrace`
6. `perf`
7. ftrace overview
8. dynamic debug overview
9. `tcpdump`
10. logic analyzer workflow
11. hardware-in-the-loop testing
12. boot log parsing

Advanced:

1. crash dump workflow
2. persistent log collection
3. support bundle design
4. production diagnostics
5. automated flashing tests
6. board farm workflow
7. field failure triage
8. postmortems
9. observability requirements
10. remote diagnostics
11. reproducer automation
12. debug-data privacy and security boundaries

## Boundaries

- Use [Linux Kernel Debugging Basics](../linux-kernel/debugging/index.md) for kernel-specific tools and workflows.
- Use [Build Artifact Debugging](../build-systems/build-artifact-debugging.md) and build-system pages for build failures.
- Use [Embedded Productization](../embedded-productization/index.md) for product diagnostics, support bundles, and field operations.
- Use each language or subsystem page for local debugging checklists.

## Related Topics

- [Linux Userspace And System Programming](../linux-userspace-and-system-programming/index.md)
- [Linux Kernel Programming](../linux-kernel/index.md)
- [Embedded Linux](../embedded-linux/index.md)
- [Embedded Productization](../embedded-productization/index.md)
- [Build Systems](../build-systems/index.md)
