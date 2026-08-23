---
status: draft
reviewed: false
domain: c
difficulty: advanced
last_reviewed: null
---

# Capstone: Embedded Linux Service

Build and deploy a production-style C service for an embedded Linux target. The service
should interact with a real or simulated device, run under a service manager, survive
short I/O and device faults, expose bounded diagnostics, and be packaged from a
reproducible cross-build. The project exercises the user/kernel, build/sysroot,
configuration/deployment, and operations boundaries.

## Project Brief

Choose a service such as:

- sensor acquisition and telemetry publisher;
- hardware health monitor;
- GPIO/LED/relay control daemon;
- local diagnostic/firmware-update agent;
- serial/CAN protocol gateway;
- DMA-backed data collector with a kernel or UIO/VFIO interface.

The service shall:

- start only after its required device/configuration dependencies are ready;
- validate configuration and device responses;
- use file descriptors and OS APIs with complete error handling;
- handle signals, cancellation, restart, and device removal/reset;
- emit structured, rate-limited diagnostics with a build ID;
- expose health/readiness behavior appropriate to the product;
- run under least privilege with a controlled filesystem/network surface;
- install, upgrade, rollback, and uninstall cleanly.

Keep device-specific code behind an adapter so most policy can be host-tested with fake
file descriptors, transports, and clocks.

## Target And Cross-Compilation Contract

Record:

- target architecture, ABI, endianness, and CPU baseline;
- compiler/binutils, libc, kernel headers, SDK, and sysroot versions;
- C dialect, warning/sanitizer/static-analysis configuration;
- CPU feature, hardening, PIE/RELRO/stack-protector, and optimization options;
- kernel version/configuration and Device Tree binding;
- root filesystem layout, service manager, users/groups, capabilities, and MAC policy;
- packaging format, update mechanism, signing, and rollback support.

A sysroot is a target header/library/runtime view, not merely a directory of headers.
Verify that compiler, linker, pkg-config, CMake/meson, and dependency resolution use
the target sysroot and do not accidentally link host libraries. Inspect the final ELF
interpreter, `DT_NEEDED` dependencies, symbol versions, RPATH/RUNPATH, and architecture.

## Architecture

```text
service manager -> process lifecycle/signal layer
                         |
configuration -> policy/state machine -> device adapter
                         |                    |
                    metrics/logging      fd/ioctl/mmap/sysfs
                                              |
                                      kernel/driver/Device Tree
```

The policy layer should not know whether the device is a real file descriptor, a
socket, a replay file, or a fake. The adapter maps OS errors and device status into a
small domain error set. Keep `errno` and raw ioctl details at the edge.

## Configuration

Define a versioned configuration schema with:

- required/optional fields and defaults;
- units, ranges, enum values, and cross-field constraints;
- file ownership/permissions and secret handling;
- startup versus runtime-reload behavior;
- migration and rollback rules;
- invalid configuration action: refuse start, safe default, or degraded mode.

Parse into a validated internal structure rather than reading configuration values
throughout the service. Avoid reloading a configuration while another thread uses the
old object; build a complete immutable snapshot and publish it under a defined
synchronization/lifetime protocol.

Do not treat environment variables, command-line options, Device Tree properties, or
sysfs values as trusted merely because the service runs as a system user. Validate
length, encoding, numeric range, path, permissions, and capability.

## Device Access

Choose the smallest appropriate interface:

- existing kernel subsystem and normal device API;
- `read`/`write`/`poll` for stream-like devices;
- `ioctl` for versioned device-specific operations;
- `mmap` only through a driver-defined, validated mapping;
- netlink/sysfs/configfs or a control daemon for configuration;
- a dedicated kernel driver when interrupts, DMA, power, security, or ownership require it.

For every device operation, handle:

- `EINTR`, short I/O, timeout, `EAGAIN`, disconnect, reset, and `ENODEV`;
- device generation/version changes;
- stale mappings and descriptor lifetime;
- partial command effects and retry idempotence;
- endian/width/layout of ioctl structures;
- blocking behavior and shutdown cancellation.

Do not use `/dev/mem` as a production driver architecture. Do not cast a physical
address in userspace. Do not assume a `volatile` mapping makes a transaction atomic or
orders it with DMA.

## Event Loop And Threading

Select a model deliberately:

- one event loop with `poll`/`epoll`, timers, signals, and control fds;
- worker threads for blocking or CPU-heavy operations;
- a bounded queue between I/O and policy;
- a process split when fault isolation or privilege separation matters.

Define descriptor ownership and shutdown order. A common safe sequence is stop accepting
new work, signal cancellation, wake blocked waits, drain/fail queued work, join workers,
close descriptors, unmap memory, and only then exit. Avoid closing a descriptor from one
thread while another may be blocked on or about to reuse its integer value without a
protocol.

Use monotonic deadlines, not wall-clock time, for timeouts. Treat `EINTR` separately
from timeout and permanent failure. Avoid unbounded retry loops and log storms.

## Service Manager Integration

Define the service lifecycle for the target manager. For a systemd-style target, decide:

- `Type`, readiness notification, and startup ordering;
- `After`/`Requires` dependencies on device, filesystem, network, and time;
- restart policy and backoff;
- watchdog/heartbeat behavior;
- signal/shutdown timeout;
- user/group, capabilities, namespaces, filesystem protections, and device access;
- resource limits, CPU affinity, memory, and I/O policy;
- log destination and rate limiting.

A minimal unit concept may look like:

```ini
[Unit]
Description=Telemetry service
After=dev-sensor0.device
Requires=dev-sensor0.device

[Service]
ExecStart=/usr/libexec/telemetryd --config /etc/telemetryd/config.toml
Restart=on-failure
RestartSec=2
User=telemetry
NoNewPrivileges=yes

[Install]
WantedBy=multi-user.target
```

The exact unit must match the target, device naming, readiness behavior, and security
policy. Test boot ordering, missing device, delayed device, repeated crash, clean stop,
upgrade, and rollback.

## Logging And Diagnostics

Use structured events with timestamp, severity, service version/build ID, device
instance/generation, operation/request ID, state, errno/domain error, and bounded
context. Separate operator action from developer trace. Protect secrets and avoid
logging raw untrusted payloads without encoding/size limits.

Implement rate limiting and counters for repeated failures. A health endpoint or status
file should distinguish alive, ready, degraded, recovering, and failed. Make the
diagnostic state machine independent of the logging transport so logging failure cannot
prevent recovery.

## Crash Diagnostics

Enable a deployment-appropriate core-dump/backtrace policy and preserve symbols outside
the target. Record build ID, target image, configuration version, kernel/Device Tree
version, and recent bounded events. Use sanitizers and debug builds on a representative
host; use production-like optimization for reproducing timing/ABI issues.

For a crash report, capture:

- signal and fault address;
- thread and stack information;
- loaded object/build IDs;
- open device/configuration state when safe;
- last operations and request IDs;
- restart count and watchdog state.

Do not make the service depend on a writable root filesystem or a live network to leave
the minimum evidence needed for diagnosis.

## Security And Least Privilege

Minimize:

- users/groups and Linux capabilities;
- writable directories and device nodes;
- accepted control sockets/ports;
- parsed configuration and protocol surface;
- syscall, namespace, and filesystem access;
- lifetime of elevated privileges.

Validate all device/configuration/network input. Use privilege separation or a helper
when a small operation needs elevated access. Review `ioctl`/`mmap` usage, path handling,
temporary files, environment, signal behavior, and dynamic loading. A service that runs
as root because device access was inconvenient is not finished.

## Packaging And Deployment

Package:

- executable and target shared-library dependencies;
- service unit/init scripts and tmpfiles/udev rules if needed;
- configuration schema/defaults and migration tools;
- users/groups/capabilities and filesystem permissions;
- version/build ID, SBOM/provenance, license data, and signature;
- debug symbols and source mapping in the artifact store;
- health checks, rollback, and post-install validation.

Test installation into a clean target root filesystem and a container/chroot/sysroot
where representative. Verify no host path, developer account, build-time secret, or
undeclared library is required.

## Test Plan

### Host tests

- configuration parsing and migration;
- state machine and retry/backoff;
- short I/O, `EINTR`, `EAGAIN`, timeout, EOF, and device error mapping;
- malformed ioctl/device responses;
- queue full, cancellation, and worker shutdown;
- health/readiness/logging behavior;
- sanitizer, static analysis, and fuzz tests for parsers/configuration.

### Target tests

- clean boot/start/stop/restart and readiness ordering;
- missing/delayed/reset device;
- kernel/driver version and Device Tree compatibility;
- permissions/capability/SELinux/AppArmor policy;
- long-run resource leak and descriptor/thread count;
- CPU/memory/I/O/latency/power under load;
- crash capture, automatic restart, backoff, and terminal failure;
- upgrade, downgrade/rollback, interrupted deployment, and configuration migration.

## Milestones

1. Target/sysroot/deployment contract and service requirements.
2. Host-testable policy/state machine with fake device.
3. Cross-build and real device adapter.
4. Service manager, configuration, diagnostics, and security policy.
5. Fault/restart/upgrade/rollback tests.
6. Performance/resource/security report and release package.

## Assessment Rubric

- **Build integrity:** correct target ABI/sysroot, dependencies, hardening, and
  reproducibility.
- **API/device safety:** complete I/O/error/ABI/mapping/lifetime handling.
- **Lifecycle:** clean startup, readiness, shutdown, restart, and upgrade behavior.
- **Operations:** useful bounded logs, health state, crash evidence, and recovery.
- **Security:** least privilege, input validation, controlled filesystem/device/network
  access, and no unnecessary root dependency.
- **Testing:** host, target, fault, soak, packaging, and rollback evidence.
- **Maintainability:** adapter/policy separation, clear contracts, and deployable docs.

## Common Mistakes

- Building with host headers/libraries while believing the result is target-tested.
- Opening `/dev/mem` or casting addresses instead of using a driver contract.
- Treating `ioctl` structures as private C structs without ABI/version validation.
- Restarting on every failure without backoff, health state, or crash-loop policy.
- Closing descriptors or replacing configuration while another thread still uses them.
- Logging unbounded/untrusted data or making recovery depend on logging success.
- Shipping a service as root because permissions and device access were not designed.
- Testing the executable but not installation, boot ordering, rollback, or clean roots.

## Related Topics

- [Professional Practice And Capstones overview](./index.md)
- [Embedded Linux](../platform-specific-c/embedded-linux.md)
- [C Interoperability](../advanced-c/c-interoperability.md)
- [Protocols And Serialization](../advanced-c/protocols-and-serialization.md)
- [Compilation, Linking, And ABI](../compilation-linking-and-abi/index.md)
- [Security](../correctness-quality-and-security/security.md)

## References

- [Linux userspace API documentation](https://docs.kernel.org/userspace-api/index.html)
- [Linux driver API documentation](https://docs.kernel.org/driver-api/index.html)
- [systemd service units](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html)
- [systemd security settings](https://www.freedesktop.org/software/systemd/man/latest/systemd.exec.html)
- [GNU libc manual](https://sourceware.org/glibc/manual/)
- [CMake cross-compiling documentation](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)
- The target kernel configuration, Device Tree binding, root filesystem/package policy,
  service manager, security policy, and deployment/rollback specification
