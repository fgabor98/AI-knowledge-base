---
status: draft
reviewed: false
domain: device-tree
difficulty: intermediate
last_reviewed: null
---

# From Live Device Tree Node To Linux Device

Linux unflattens DT nodes into `struct device_node` objects. Separate population code then creates Linux devices on specific buses. The live tree can therefore contain a valid, available node with no corresponding device object.

## Start With The Node

```bash
dtroot=/sys/firmware/devicetree/base
node="$dtroot/soc/device@48000000"

test -d "$node"
tr '\0' '\n' <"$node/compatible"

if [[ -e "$node/status" ]]; then
        tr -d '\0' <"$node/status"
        printf '\n'
else
        printf '%s\n' 'status absent'
fi
```

Confirm:

- exact path and parent bus
- ordered compatible list
- effective availability
- address and parent cell context
- all required provider properties

Do not begin by guessing a sysfs device name from the unit address.

## Find Devices Through `of_node`

Devices associated with an OF node commonly expose an `of_node` symlink. Match resolved paths:

```bash
target=$(readlink -f "$node")

find /sys/devices -type l -name of_node -print0 2>/dev/null |
while IFS= read -r -d '' link; do
        if [[ $(readlink -f "$link") == "$target" ]]; then
                dirname "$link"
        fi
done
```

This proves association between a Linux device and the live node. It avoids assumptions about platform-device naming.

Depending on kernel/subsystem, related firmware-node links may have different names or layering. Prefer the documented `of_node` relationship where present and record the resolved target.

## Identify The Bus

Given a device directory:

```bash
dev=/sys/devices/platform/soc/48000000.device

readlink -f "$dev/subsystem"
readlink -f "$dev/driver" 2>/dev/null
cat "$dev/modalias" 2>/dev/null
cat "$dev/uevent" 2>/dev/null
```

The `subsystem` symlink identifies the bus/class association used for matching and lifecycle. A node beneath an SoC bus often becomes a platform device, but bus controller children can become I2C, SPI, MDIO, auxiliary, or other device types.

## Population Is Hierarchical

Common paths:

```text
live SoC/simple-bus node
  -> platform population creates controller device
  -> controller driver probes
  -> controller enumerates DT child devices on its bus
  -> child drivers match/probe
```

If an I2C controller never binds, its child sensor node can remain visible in DT while no I2C client device appears. Fix the parent/controller stage before debugging the child driver.

For SPI/I2C children, inspect:

```bash
ls -l /sys/bus/i2c/devices
ls -l /sys/bus/spi/devices
```

Device names encode bus numbering and addresses/chip selects according to subsystem conventions, not necessarily the full DT path.

## Availability Is Necessary, Not Sufficient

OF availability normally accepts absent `status`, `okay`, or `ok`. A device still may not be created because:

- parent bus/node is unavailable
- parent device was not populated
- controller driver has not probed
- subsystem intentionally handles child nodes internally
- node represents data/topology, not an independently registered device
- firmware/secure world owns the function
- architecture/platform code excludes it
- a runtime overlay transition is incomplete

Not every node should have a one-to-one sysfs device.

## Nodes That Are Not Devices

Examples include:

- `/aliases` and `/chosen`
- pin configuration groups
- OPP tables
- graph `ports`/`endpoint` nodes
- fixed partitions interpreted by a parent
- CPU topology containers
- reserved-memory regions
- provider substructures consumed by another driver

Do not report “missing platform device” for a node whose binding defines no standalone device.

## Parent-First Trace

For a missing device:

1. identify node and binding
2. identify parent node/bus
3. find parent Linux device through `of_node`
4. verify parent driver link and logs
5. confirm the parent subsystem enumerates this child class
6. confirm child `status`, address, and compatible
7. search the correct bus device list

This avoids blaming a child compatible when the controller never registered its bus.

## Platform Device Names

Platform device names can include translated addresses, node names, or platform-specific naming. Treat them as Linux device-model identifiers, not DT ABI. Always prove with `of_node`:

```bash
readlink -f /sys/bus/platform/devices/DEVICE/of_node
```

Hard-coding a platform device name into long-lived diagnostics can break when hierarchy or naming changes while the hardware binding remains valid.

## One Node, Multiple Objects

Complex subsystems can create:

- one parent device plus child devices
- class devices in `/sys/class`
- auxiliary/component objects
- MFD children
- network, DRM, media, input, hwmon, IIO, or sound interfaces

The OF node association proves origin; follow symlinks and subsystem topology to the functional interface. A class device may point back through a physical device chain rather than expose its own direct `of_node`.

## Negative Evidence

If no `of_node` symlink points to the live node, record:

- node exists and exact availability
- parent device/bus state
- searched buses and time of snapshot
- boot/probe logs
- whether the node should instantiate independently under its binding

“`find` returned nothing” is useful only with scope and timing. Asynchronous or deferred probe can change device state after capture.

## Node-To-Device Record

```text
live node path:
compatible/status:
binding-defined entity type:
parent live node:
parent Linux device/driver:
child Linux device physical path:
bus/subsystem:
resolved of_node:
device creation time/log evidence:
```

## Authoritative References

- [Linux sysfs device ABI](https://docs.kernel.org/admin-guide/abi-stable.html)
- [Linux Devicetree Kernel API](https://docs.kernel.org/devicetree/kernel-api.html)
- [Linux Devicetree usage model](https://docs.kernel.org/devicetree/usage-model.html)
- [Linux driver-core model](https://docs.kernel.org/driver-api/driver-model/)

## Continue

Proceed to [Matching, Modaliases, Modules, And Bound Drivers](matching-modaliases-modules-and-bound-drivers.md).
