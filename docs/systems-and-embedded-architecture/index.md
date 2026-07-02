---
status: draft
reviewed: false
domain: systems-architecture
difficulty: intermediate
last_reviewed: null
---

# Systems And Embedded Architecture

Systems and embedded architecture topics focused on how hardware, firmware, the kernel, userspace, networking, updates, diagnostics, and product constraints fit together.

This section is the system-design layer above individual programming languages, Linux subsystems, and build tools.

## Learning Path

Foundations:

1. CPU architecture basics
2. memory hierarchy
3. caches and cache lines
4. MMU and virtual memory concepts
5. interrupts at system level
6. DMA at system level
7. buses: I2C, SPI, UART, CAN, PCIe, USB, and Ethernet
8. clocks, resets, regulators, and power rails
9. boot chain overview
10. firmware, kernel, and userspace responsibility split
11. latency, throughput, and jitter
12. observability and diagnostics as design constraints

Embedded Linux Architecture:

1. bootloader to kernel handoff
2. Device Tree as hardware contract
3. kernel driver vs userspace service decision
4. service architecture on embedded Linux
5. IPC design
6. logging and persistent evidence
7. read-only root filesystem strategy
8. persistent state partitioning
9. update and rollback architecture
10. recovery and rescue paths
11. watchdog strategy
12. hardware/software partitioning

Advanced System Design:

1. real-time constraints and scheduling boundaries
2. fault containment
3. safety-oriented failure modes
4. secure boot and trust boundaries
5. remote firmware and heterogeneous SoC partitioning
6. power-management architecture
7. networked device architecture
8. manufacturing and provisioning architecture
9. field diagnostics and support bundles
10. product compatibility contracts
11. release architecture
12. tradeoff analysis across cost, power, performance, reliability, and maintainability

## Design Questions

- What must run before Linux?
- What belongs in the kernel, userspace, firmware, or hardware?
- What happens when power is lost during an update?
- What evidence survives a crash or watchdog reset?
- Which component owns a device, memory region, interrupt, or protocol?
- What are the latency and throughput requirements?
- What is the rollback path?
- How is version compatibility enforced?
- What is the smallest recoverable system image?
- Which failures should restart a service, reboot Linux, or put hardware in a safe state?

## Related Topics

- [Linux Userspace And System Programming](../linux-userspace-and-system-programming/index.md)
- [Linux Kernel Programming](../linux-kernel/index.md)
- [Embedded Linux](../embedded-linux/index.md)
- [Build Systems](../build-systems/index.md)
- [Embedded Productization](../embedded-productization/index.md)
- [Device Tree](../device-tree/index.md)
