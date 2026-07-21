---
status: active
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# 7. Power Management And Heterogeneous SoCs

## Device Power Management

- [ ] **P0** [CPU and device power management](https://docs.kernel.org/driver-api/pm/index.html)
- [ ] **P0** [Device power-management basics](https://docs.kernel.org/driver-api/pm/devices.html)
- [ ] **P0** [Runtime PM](https://docs.kernel.org/power/runtime_pm.html)
- [ ] **P0** [System sleep states](https://docs.kernel.org/admin-guide/pm/sleep-states.html)
- [ ] **P0** [System suspend and device interrupts](https://docs.kernel.org/power/suspend-and-interrupts.html)
- [ ] **P0** [Device wake-up model](https://docs.kernel.org/driver-api/pm/devices.html#device-wakeup)
- [ ] **P0** Study generic power domains through the [device power-management guide](https://docs.kernel.org/driver-api/pm/index.html), `include/linux/pm_domain.h`, and `drivers/pmdomain/`.
- [ ] **P1** [Device frequency scaling](https://docs.kernel.org/driver-api/devfreq.html)
- [ ] **P1** [CPU idle](https://docs.kernel.org/admin-guide/pm/cpuidle.html)
- [ ] **P1** [CPU performance scaling](https://docs.kernel.org/admin-guide/pm/cpufreq.html)
- [ ] **P1** [Suspend testing](https://docs.kernel.org/power/basic-pm-debugging.html)
- [ ] **P1** [Power-management quality of service](https://docs.kernel.org/power/pm_qos_interface.html)

## Firmware, Remoteproc, RPMsg, And Virtio

- [ ] **P0** [Remote processor framework](https://docs.kernel.org/staging/remoteproc.html)
- [ ] **P0** [Remote processor messaging framework](https://docs.kernel.org/staging/rpmsg.html)
- [ ] **P0** [Virtio](https://docs.kernel.org/driver-api/virtio/virtio.html)
- [ ] **P0** [Firmware request API](https://docs.kernel.org/driver-api/firmware/request_firmware.html)
- [ ] **P0** Read remoteproc/RPMsg/PRU/R5/M4 bindings used by the target SoC.
- [ ] **P0** Read the corresponding upstream TI remoteproc, mailbox, interrupt-router, and RPMsg drivers.
- [ ] **P1** [Common mailbox framework](https://docs.kernel.org/driver-api/mailbox.html)
- [ ] **P1** Read `Documentation/devicetree/bindings/reserved-memory/` in the selected kernel tree and review the [official binding collection](https://docs.kernel.org/devicetree/bindings/).

## TI Heterogeneous-SoC Exercises

- [ ] Draw the ownership map for Linux, PRU, R5, and M4 cores.
- [ ] Record firmware filenames, loading path, remoteproc state transitions, and recovery behavior.
- [ ] Map every carveout, vring, resource table, and shared-memory region.
- [ ] Trace mailbox/interrupt routing between host and remote core.
- [ ] Trace RPMsg channel announcement, endpoint creation, message flow, and teardown.
- [ ] Verify cache coherency and memory attributes for shared memory.
- [ ] Test remote-core crash reporting and restart behavior.
- [ ] Test Linux suspend/resume while remote cores and RPMsg clients are active.
- [ ] Compare upstream behavior with the TI SDK/vendor kernel implementation.

## Power-Management Exercises

- [ ] Draw supplier/consumer ordering for clocks, regulators, resets, power domains, and devices.
- [ ] Verify runtime-PM usage counts and autosuspend behavior from sysfs and tracepoints.
- [ ] Test every system sleep state supported by the board.
- [ ] Prove configured wake sources and distinguish wake-capable from wake-enabled.
- [ ] Test failure rollback during suspend and resume.
- [ ] Capture the first failing device rather than only the final suspend error.
