---
status: active
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# 8. Architecture, Userspace ABI, Administration, And Optional Areas

This checklist completes the official top-level documentation map. Read P0/P1
items for embedded product work; use P2 areas when a project actually needs
them.

Knowledge-guide companion: [Stage 8](knowledge-guide-companion.md#stage-8-architecture-userspace-abi-administration-and-policy)

## ARM And ARM64

- [ ] **P0** [ARM64 architecture documentation](https://docs.kernel.org/arch/arm64/index.html)
- [ ] **P0** [ARM64 booting requirements](https://docs.kernel.org/arch/arm64/booting.html)
- [ ] **P0** [ARM64 memory layout](https://docs.kernel.org/arch/arm64/memory.html)
- [ ] **P1** [ARM architecture documentation](https://docs.kernel.org/arch/arm/index.html)
- [ ] **P1** [Booting ARM Linux](https://docs.kernel.org/arch/arm/booting.html)
- [ ] **P1** [ARM porting guide](https://docs.kernel.org/arch/arm/porting.html)
- [ ] **P1** Read architecture-specific cache, DMA, barriers, and interrupt material relevant to the selected TI SoC.

## Userspace ABI And Driver Interfaces

- [ ] **P0** [Userspace API](https://docs.kernel.org/userspace-api/index.html)
- [ ] **P0** [Linux ABI description](https://docs.kernel.org/admin-guide/abi.html)
- [ ] **P0** [Stable ABI files](https://docs.kernel.org/admin-guide/abi-stable.html)
- [ ] **P0** [Testing ABI files](https://docs.kernel.org/admin-guide/abi-testing.html)
- [ ] **P0** [Sysfs rules](https://docs.kernel.org/admin-guide/sysfs-rules.html)
- [ ] **P0** [ioctl-based interfaces](https://docs.kernel.org/driver-api/ioctl.html)
- [ ] **P0** [(How to avoid) botching up ioctls](https://docs.kernel.org/process/botching-up-ioctls.html)
- [ ] **P1** [Userspace I/O HOWTO](https://docs.kernel.org/driver-api/uio-howto.html)
- [ ] **P1** [Generic Netlink](https://docs.kernel.org/userspace-api/netlink/intro.html)

## Administration And Product Operation

- [ ] **P0** [Kernel parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [ ] **P0** [Module parameters](https://docs.kernel.org/admin-guide/kernel-parameters.html)
- [ ] **P0** [Serial console](https://docs.kernel.org/admin-guide/serial-console.html)
- [ ] **P0** [Kernel taints](https://docs.kernel.org/admin-guide/tainted-kernels.html)
- [ ] **P0** [Security hardening self-protection](https://docs.kernel.org/security/self-protection.html)
- [ ] **P1** [Linux Security Modules](https://docs.kernel.org/admin-guide/LSM/index.html)
- [ ] **P1** [Namespaces](https://docs.kernel.org/admin-guide/namespaces/index.html)
- [ ] **P1** [Control groups](https://docs.kernel.org/admin-guide/cgroup-v2.html)
- [ ] **P1** [Kernel module signing](https://docs.kernel.org/admin-guide/module-signing.html)

## Optional Top-Level Areas

- [ ] **P2** [PCI](https://docs.kernel.org/PCI/index.html) when the board uses PCIe endpoints or root complexes.
- [ ] **P2** [USB](https://docs.kernel.org/driver-api/usb/index.html) when implementing USB host, gadget, or function drivers.
- [ ] **P2** [DRM](https://docs.kernel.org/gpu/index.html) for display projects.
- [ ] **P2** [Media](https://docs.kernel.org/driver-api/media/index.html) for camera/video projects.
- [ ] **P2** [ALSA](https://docs.kernel.org/sound/index.html) for audio projects.
- [ ] **P2** [Networking](https://docs.kernel.org/networking/index.html) beyond CAN and PHY work.
- [ ] **P2** [Filesystems](https://docs.kernel.org/filesystems/index.html) for filesystem or storage-stack work.
- [ ] **P2** [Memory management](https://docs.kernel.org/mm/index.html) beyond driver allocation and DMA needs.
- [ ] **P2** [Scheduler](https://docs.kernel.org/scheduler/index.html) for latency and real-time investigations.
- [ ] **P2** [Security](https://docs.kernel.org/security/index.html) for production hardening.
- [ ] **P2** [Livepatching](https://docs.kernel.org/livepatch/index.html).
- [ ] **P2** [Rust](https://docs.kernel.org/rust/index.html).
- [ ] **P2** [Unsorted documentation](https://docs.kernel.org/staging/index.html) only when linked by relevant code or maintained documentation.

## Final Synthesis

- [ ] Build a source-and-documentation map for one complete project driver stack.
- [ ] Document differences between upstream and the active TI vendor kernel.
- [ ] Identify every userspace ABI the product depends on and its stability status.
- [ ] Produce a debug kernel configuration and a production configuration with explicit rationale.
- [ ] Revisit all unchecked P1 items and promote project-relevant ones to P0.
- [ ] Record which official top-level areas are intentionally deferred and why.
