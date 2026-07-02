---
status: draft
reviewed: false
domain: embedded-productization
difficulty: intermediate
last_reviewed: null
---

# Embedded Productization

Embedded productization covers the work that turns a booting embedded Linux board into a reproducible, testable, releasable, updateable, and diagnosable product.

This section is intentionally cross-cutting. It sits above build systems, embedded Linux bring-up, debugging, security, manufacturing, and field operations.

## Scope

- embedded DevOps and release engineering
- reproducible product builds
- artifact versioning and release manifests
- hardware-in-the-loop testing
- factory flashing and provisioning
- OTA and field update systems
- A/B boot and rollback design
- secure boot and verified boot
- image layout, partitioning, and storage strategy
- production diagnostics and observability
- watchdogs, reliability, and failure recovery
- device identity, certificates, and calibration data
- SoC vendor BSP release management

## Learning Path

Start with the product lifecycle:

1. Build a complete image from pinned sources.
2. Boot it on real hardware and capture serial logs.
3. Generate a release manifest that identifies every artifact.
4. Flash the image through the same path used by production.
5. Run hardware smoke tests automatically.
6. Produce an update bundle, not only a full factory image.
7. Test interrupted update and rollback behavior.
8. Add secure boot or verified boot constraints.
9. Add field diagnostics, persistent logs, and version reporting.
10. Document the exact release, factory, and recovery workflows.

## Topic Groups

### Release Engineering

- product versioning
- artifact naming
- build provenance
- source manifests
- release manifests
- debug symbols
- SDK release artifacts
- SBOM and license artifacts
- factory image vs OTA image
- release notes and compatibility notes

### Embedded DevOps

- CI for bootloader, kernel, rootfs, and full images
- CI build containers or pinned build hosts
- Yocto downloads and sstate cache management
- artifact retention
- image size regression checks
- license and CVE checks
- reproducible release builds
- hardware CI and board farm integration

### Hardware-In-The-Loop Testing

- serial console automation
- relay-controlled power cycling
- SD/eMMC flashing automation
- network boot
- boot log parsing
- smoke tests
- peripheral tests
- update and reboot cycle tests
- result capture and artifact linking

### Factory Provisioning

- factory flashing stations
- golden images
- serial numbers
- MAC address programming
- certificates and keys
- calibration data
- production test logs
- device identity
- recovery image handling

### Updates And Recovery

- RAUC
- SWUpdate
- Mender
- OSTree-style update flows
- A/B partitioning
- bootloader environment integration
- signed update bundles
- rollback rules
- interrupted update recovery
- runtime recovery workflow
- rescue shell policy
- compatibility metadata
- update status reporting
- bootloader/user-space update contract

### Security And Chain Of Trust

- ROM boot trust anchor
- SPL and U-Boot signing
- FIT image signing
- verified boot
- root filesystem integrity
- kernel module signing
- key management
- secure manufacturing implications
- filesystem permissions
- Linux capabilities
- users and groups
- SSH hardening
- secrets handling
- TPM overview
- TEE overview
- attack surface reduction

### Diagnostics And Reliability

- persistent logs
- crash dumps
- watchdogs
- boot counters
- health checks
- update status reporting
- remote log retrieval
- read-only root filesystems
- filesystem corruption prevention
- brownout and power-loss behavior
- version reporting on device
- support bundle generation
- remote diagnostics
- field health checks
- artifact provenance on device

### Manufacturing And Board Test

- factory flashing workflow
- hardware test images
- MAC address provisioning
- serial number provisioning
- EEPROM programming
- certificate provisioning
- calibration data
- production test logs
- fixture-driven tests
- separation of manufacturing and production images

## Related Topics

- [Embedded Linux](../embedded-linux/index.md)
- [Build Systems](../build-systems/index.md)
- [Debugging And Diagnostics](../debugging/index.md)
- [Linux Kernel Programming](../linux-kernel/index.md)
- [Networking](../networking/index.md)
- [Topic Map](../topic-map.md)
