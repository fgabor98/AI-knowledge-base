---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Linux Runtime Overlays, Devices, Notifiers, And Lifetime

Applying an overlay to Linux's live tree changes more than data. Reconfiguration notifications can create devices, drivers can probe, userspace can open interfaces, DMA and IRQs can start, and other kernel code can cache pointers. Removal must unwind the entire causal chain before overlay-owned memory is freed.

## In-Kernel API Shape

The core workflow is conceptually:

```c
int overlay_id;
int ret;

ret = of_overlay_fdt_apply(overlay_fdt, overlay_size,
                           &overlay_id, base_node);
if (ret)
        /* Follow the kernel-version-specific partial-apply cleanup contract. */

/* ... use the applied configuration ... */

ret = of_overlay_remove(&overlay_id);
```

Exact prototypes and supported callers vary by kernel version. Use the headers and API documentation for the target tree. The returned overlay changeset ID is a runtime removal handle, not a stable overlay identity.

`of_overlay_remove_all()` removes overlays in a permitted order. It does not prove that every driver or subsystem is dynamically removable.

## Overlay Notifier Phases

The overlay notifier actions include:

```text
OF_OVERLAY_PRE_APPLY
OF_OVERLAY_POST_APPLY
OF_OVERLAY_PRE_REMOVE
OF_OVERLAY_POST_REMOVE
```

Callbacks can validate or coordinate around a specific overlay transition. The upstream lifetime rule is strict: pointers to overlay nodes or their content must not persist beyond the corresponding `OF_OVERLAY_POST_REMOVE` callback. The overlay memory is freed after post-remove notification even when a post-remove notifier reports an error.

Therefore a post-remove error cannot extend pointer lifetime or “veto” the memory free.

## Dynamic Reconfiguration Notifiers Differ

The generic OF changeset/reconfiguration notifiers in `drivers/of/dynamic.c` are distinct from overlay notifiers. They are not allowed to retain pointers to overlay nodes or properties. Overlay code cannot protect callers that cache such pointers and later dereference them after removal.

Document which notifier chain a consumer uses. Similar-looking callbacks have different permissible lifetimes.

## Node Refcounts Are Not A Removal Contract

`of_node_get()` and `of_node_put()` manage references for normal node use, but retaining a pointer to overlay-owned content across overlay removal is not made safe merely by incrementing a refcount. Follow overlay documentation's lifetime boundary.

Copy stable scalar/string data into owned memory when appropriate, and release all overlay-derived state during teardown. Never cache direct property-value pointers indefinitely.

## Device Population

When an applied node is active and belongs to a populated bus, Linux can register a device and bind a driver. Removal can trigger device unregister/unbind. The exact path depends on bus and subsystem support.

An overlay node appearing in `/sys/firmware/devicetree/base` does not prove:

- the correct device object was created
- a driver matched
- probe completed rather than deferred
- all dependent suppliers exist
- userspace sees a functioning interface
- removal is supported

Trace tree, device model, driver logs, and subsystem state separately.

## Teardown State Machine

For a dynamically removable device, require an explicit sequence:

```text
ACTIVE
  -> reject new opens/requests
QUIESCING
  -> stop hardware and protocol ingress
  -> cancel timers and delayed work
  -> flush workqueues/tasklets/threaded IRQ work
  -> terminate or synchronize DMA
  -> disable and synchronize IRQs
  -> unregister child devices and subsystem objects
  -> release device links, PM, clocks, resets, regulators, GPIOs
  -> wait for users/refcounts under subsystem rules
DETACHED
  -> release copied DT-derived data and node references
OVERLAY_REVERTED
  -> overlay memory may be freed
```

The exact order is driver- and subsystem-specific. Avoid powering down before asynchronous access is drained.

## Hidden Lifetime Holders

Audit:

- open character/block/network/input/media device handles
- sysfs/configfs/debugfs files and callbacks
- interrupts, threaded handlers, tasklets, timers, workqueues
- DMA descriptors and device/IOMMU mappings
- runtime PM references and device links
- clock/regulator/reset/PHY consumers
- graph endpoint caches and component frameworks
- notifier registrations
- firmware callbacks and remote processors
- child devices created by MFD, I2C, SPI, PCI, platform, or auxiliary buses
- users of `of_find_*` that retain node/property pointers

One unsupported consumer makes the whole runtime-removal claim unsafe.

## Probe Failure Is Not Clean Rollback

An overlay can apply successfully while a newly created device fails probe. The tree mutation and device registration may remain even though the desired function is absent. Product code must define whether to:

- keep the overlay and expose a diagnosed failed device
- remove the overlay after all partial device state is unwound
- enter a recovery mode
- reboot into a known pre-composed configuration

Do not assume a failed driver probe automatically reverts the overlay.

## Apply Error Handling

Current kernel API documentation warns that some apply error paths can leave a partially applied changeset, particularly around post-apply notifiers. The caller must retain/use the returned overlay ID and follow the documented removal cleanup for its kernel version.

Design error injection for:

- resolution failure before changeset creation
- pre-apply notifier rejection
- changeset application error
- post-apply notifier error
- device creation/probe failure
- pre-remove notifier rejection
- revert failure
- post-remove notifier error

Record the resulting tree and device state for each.

## Removal Dependencies

The core prevents removing an overlay that another overlay is stacked on top of. This protects recorded structural dependencies. It does not model arbitrary userspace, electrical, or cross-subsystem dependencies.

Remove dependents first, quiesce all device effects, then remove the provider overlay. `of_overlay_remove_all()` supplies stack order, not driver teardown correctness.

## When To Declare Boot-Only

Classify an overlay as apply-only until removal has explicit evidence. Boot-only/pre-boot application is preferable when:

- drivers lack hot-unplug support
- the hardware is not physically hot-removable
- shared power/pin resources affect permanent devices
- remote firmware or DMA ownership persists
- userspace cannot be coordinated
- a subsystem caches DT pointers or topology
- failure recovery requires reboot anyway

A runtime application interface does not create runtime-removable hardware.

## Runtime Evidence

For an overlay advertised as removable, test repeated cycles under load:

- apply, probe, use, unbind/remove, and reapply
- concurrent user open/close and removal rejection
- IRQ and DMA stress during quiesce
- suspend/resume before and after apply
- removal during deferred probe or failed probe
- dependent overlay removal order
- memory debugging/KASAN/KCSAN/lockdep as appropriate
- module unload/reload where relevant
- resource inventories before and after each cycle

“One successful `rmdir`” is not lifecycle evidence.

## Authoritative References

- [Linux Devicetree Overlay Notes](https://docs.kernel.org/devicetree/overlay-notes.html)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux Devicetree Changesets](https://docs.kernel.org/devicetree/changesets.html)
- [Linux Driver Binding documentation](https://docs.kernel.org/driver-api/driver-model/binding.html)

## Continue

Proceed to [Validation, Security, And Product Architecture](validation-security-and-product-architecture.md).
