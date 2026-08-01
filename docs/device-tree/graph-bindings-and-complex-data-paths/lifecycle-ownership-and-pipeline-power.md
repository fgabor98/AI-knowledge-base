---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Lifecycle, Ownership, And Pipeline Power

A graph becomes operational only after every required component is present and the subsystem assembles the chain. Correct lifecycle design handles arbitrary probe order, partial failure, stream transitions, suspend, hot-unplug, and teardown.

## Probe Order Is Not Data Direction

A source need not probe before its sink. An I²C sensor can register after a CSI receiver; a panel can register after a display controller; a codec can register after the CPU DAI. Frameworks use asynchronous notifiers, component matching, or deferred probe to assemble the final pipeline.

Do not encode data-flow order as initcall priority. If a component is permanently missing, trace:

1. both endpoint nodes in the live tree
2. reciprocal `remote-endpoint` values
3. ancestor `status` values
4. control-bus device creation and driver match
5. resource supplier registration
6. subsystem async/component logs
7. format/route constraints during completion

## Control Ownership Versus Stream Ownership

A bridge driver can own its registers and power resources while the DRM or media parent controls when the complete chain streams. A sensor driver owns sensor configuration, but the receiver may coordinate pipeline start. ASoC components own DAIs/widgets while the card coordinates a stream.

Exactly one layer should perform each transition. Duplicating reset, regulator, or GPIO control between a parent pipeline driver and component driver creates races. Missing coordination can send data before a sink is ready.

## Conceptual Start And Stop

A safe start often prepares from sink toward source so no producer sends into an unready receiver:

```text
power shared domain/resources
  -> configure sink/capture/output
  -> configure intermediate bridges/receivers
  -> configure source
  -> enable downstream stages
  -> start source last
```

Stop often halts the source first, drains the path, then disables downstream stages. This is a design pattern, not a universal sequence; DRM bridge callbacks, V4L2 stream control, and ASoC/DAPM implement subsystem-specific ordering.

## Failure Rollback

If a middle component fails to enable, unwind only completed steps and stop every producer. Review:

- clock and regulator reference counts
- reset/isolation state
- DMA and interrupt quiescence
- buffer ownership and fences
- bridge/panel prepare-enable balance
- async notifier/component cleanup

Repeated stream attempts should not leak power or leave stale routes. Fault injection at each stage is more revealing than probe-only testing.

## Runtime And System Suspend

Components can have independent runtime-PM state while one active stream requires the entire path. Device links and subsystem references help keep suppliers active. Still, graph edges alone do not establish every PM dependency.

For suspend, decide whether the pipeline stops, retains context, or supports wake. A camera sensor used for wake, an audio keyword detector, or display self-refresh needs an always-on subset distinct from the main data path. Model its supplies, clocks, IRQs, and firmware ownership explicitly.

Test system suspend with:

- pipeline idle
- pipeline configured but not streaming
- active stream where supported
- runtime-suspended components
- abort during suspend preparation
- wake and immediate restart

## Hotplug And Removal

Connectors, removable modules, and overlays can change availability. The subsystem must prevent new streams, stop existing users, unregister objects in safe order, and wait for outstanding work. Removing a graph node while a driver retains endpoint references can become a use-after-free or stale topology.

Dynamic DT changes are covered later in overlays, but graph drivers should already have sound bind/unbind lifetimes.

## Boot Firmware Handoff

Firmware may leave a display streaming, camera clock running, or DSP audio route active. Handoff requires a documented contract for memory, clocks, route registers, component power, and reset state. The OS cannot safely preserve a pipeline it cannot describe completely.

Compare cold boot, warm boot, kexec, and unbind/rebind. Different results reveal hidden inherited state.

## Authoritative References

- [Linux V4L2 async API](https://docs.kernel.org/driver-api/media/v4l2-async.html)
- [Linux DRM bridge helpers](https://docs.kernel.org/gpu/drm-kms-helpers.html)
- [Linux ASoC machine-driver documentation](https://docs.kernel.org/sound/soc/machine.html)
- [Linux device links](https://docs.kernel.org/driver-api/device_link.html)
- [Linux runtime power management](https://docs.kernel.org/power/runtime_pm.html)

## Continue

Proceed to [Graph Validation And Runtime Diagnosis](graph-validation-and-runtime-diagnosis.md).
