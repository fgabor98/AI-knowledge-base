---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Controlled Bind/Unbind And Runtime Forensics

Manual bind/unbind can distinguish one-time boot ordering from repeatable probe failure, but it actively tears down and reinitializes a device. It can disconnect storage, console, network access, power, clocks, child devices, or DMA. Evidence collection is read-only; bind/unbind requires a lab safety case.

## Do Not Experiment On Critical Devices

Never casually unbind:

- root filesystem or boot storage controllers
- active console/serial transport
- network interface carrying your remote session
- watchdog or safety controller
- interrupt, clock, reset, pinctrl, regulator, PMIC, or power-domain providers
- IOMMU for active DMA devices
- parent bus with live children
- remoteproc/firmware owner with shared memory in use
- display/input needed for recovery

Use local console, recovery boot, isolated hardware, and a tested power-cycle path.

## Precondition Review

Before unbind:

- driver implements safe remove/unbind for this device
- subsystem supports hot removal/reprobe
- all userspace clients are stopped
- no mounted filesystem, network route, open device, or active stream depends on it
- child devices can be removed safely
- DMA, IRQ, work, timers, and firmware operations quiesce in remove
- suppliers remain stable
- runtime overlay nodes will not disappear unexpectedly
- you know exact bus device and driver names

If any item is unknown, stop and inspect source/subsystem documentation.

## Snapshot Before Action

```bash
dev=/sys/bus/platform/devices/48000000.device
driver=$(basename "$(readlink -f "$dev/driver")")

printf 'device=%s driver=%s\n' "$(basename "$dev")" "$driver"
readlink -f "$dev/of_node"
cat "$dev/modalias"
cat "$dev/uevent"
```

Also capture:

- full kernel log with monotonic timestamps
- deferred list
- supplier/consumer links
- subsystem state and active users
- interrupts/DMA/resource summaries relevant to the device
- driver/module versions

## Unbind And Bind

For a bus exposing these controls:

```bash
device_name=$(basename "$dev")
driver_dir="/sys/bus/platform/drivers/$driver"

printf '%s\n' "$device_name" | sudo tee "$driver_dir/unbind"
test ! -e "$dev/driver"

printf '%s\n' "$device_name" | sudo tee "$driver_dir/bind"
test -L "$dev/driver"
```

Check command status and logs after each transition. Do not chain unbind and bind blindly; inspect whether teardown completed.

Exact controls vary by bus and driver. Absence of `bind`/`unbind` is not an invitation to create them or unload a provider module.

## Interpretation

| Result | Possible meaning |
|---|---|
| unbind refused | driver/bus policy, active dependency, wrong device identifier |
| unbind hangs | teardown deadlock, active I/O, child/dependency issue |
| unbind succeeds; bind succeeds | initial ordering/transient issue possible; compare logs/state |
| bind fails consistently | repeatable match/probe/resource/hardware failure |
| bind succeeds only after supplier action | dependency/readiness issue |
| driver link returns but function fails | probe success is not functional correctness |
| crash after unbind | stale async callback, user, reference, or teardown bug |

One successful rebind does not prove repeated lifecycle safety.

## Manual Bind Does Not Override Matching By Default

Writing a device name to a driver's `bind` file asks the bus/driver core to attach that driver, but ordinary matching and policy still apply according to the bus. If matching is intentionally bypassed through `driver_override`, that is a separate state change.

Never use forced matching to conclude the DT compatible is correct.

## `driver_override` Experiment

If the bus supports it, upstream semantics are:

- writing a driver name restricts matching to that driver
- it bypasses normal bus-specific ID matching when evaluated
- it does not automatically unbind
- it does not automatically load or bind the requested driver
- clearing it restores normal matching

Record original value and restore it. Use only when the candidate driver is known safe for the hardware and the purpose is to isolate match metadata—not to operate unsupported hardware.

## Module Reload Is Broader

`modprobe -r` affects every device bound to the module and may fail due to references. It can also remove shared provider functionality. Prefer per-device unbind when supported and scoped.

Built-in drivers cannot be unloaded. Rebooting with altered command line/configuration may be the only safe retry for early/core devices.

## Build A Timeline

Use monotonic logs:

```bash
journalctl -k -b 0 -o short-monotonic > before.log
# Perform one controlled transition.
journalctl -k -b 0 -o short-monotonic > after.log
```

Record wall-clock/operator event separately:

```text
T+123.400 snapshot complete
T+130.100 unbind requested
T+130.230 driver link removed
T+135.000 bind requested
T+135.420 probe success/driver link restored
```

Avoid `dmesg -T` as the sole correlation source; human timestamps can be reconstructed imperfectly.

## Verify Teardown

After unbind, check:

- driver symlink removed
- class/subsystem interface removed or inactive
- interrupts no longer increasing
- DMA/work/timers stopped according to trace/source
- child devices removed as expected
- supplier links/PM references released
- no kernel warnings, refcount leaks, or use-after-free
- userspace observes removal cleanly

After bind, verify the same functional tests used at boot, not only the symlink.

## Field Forensics Bundle

```text
boot ID, kernel release/build/config identity
raw boot FDT and live-tree captures
target raw/decoded DT properties
device/of_node/subsystem/modalias/uevent/driver state
module and alias metadata
deferred list and supplier/consumer links
full kernel log
subsystem-specific state
controlled transition timeline and statuses
hardware result
```

Redact secrets, MACs/serials where policy requires, and `/chosen` seed content.

## Stop Conditions

Stop and power-cycle/recover if:

- unbind affects console/rootfs/remote access unexpectedly
- interrupts/DMA continue after teardown
- kernel reports lockup, refcount, RCU, slab, or use-after-free warnings
- a supplier is removed while consumers remain
- live tree changes during capture unexpectedly
- bind attempt drives unsafe power/clock/pin state
- evidence no longer corresponds to the original boot

## Authoritative References

- [Linux driver-core binding model](https://docs.kernel.org/driver-api/driver-model/binding.html)
- [Linux driver model: probe and remove](https://docs.kernel.org/driver-api/driver-model/driver.html)
- [Linux sysfs rules](https://docs.kernel.org/filesystems/sysfs.html)
- [Linux Devicetree overlay notes](https://docs.kernel.org/devicetree/overlay-notes.html)

## Continue

Proceed to the [Runtime Device Tree And Probe Forensics Lab](runtime-device-tree-and-probe-forensics-lab.md).
