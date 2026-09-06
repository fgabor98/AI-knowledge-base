---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Serial, Input, Sensors, And hwmon

## What problem does this solve?

Standard subsystems expose common hardware through stable patterns. Choosing the
subsystem UAPI instead of a private register protocol improves portability, but
userspace still must validate units, timestamps, freshness, permissions, and device
lifetime.

## Input and sensors

Input event devices provide typed events with codes and timestamps; they are not
ordinary text streams. IIO can expose channels, scale, raw values, triggered buffers,
and timestamps. hwmon commonly exposes sensor values and limits as sysfs attributes
with documented units and update behavior.

Read the subsystem documentation and query available channels/features. Do not assume
`/sys/class/hwmon/hwmon0` identifies the same sensor after boot or board changes.

## Data validity

Validate event type/code, payload size, sign, scale, unit, range, timestamp domain,
sequence, and freshness. Distinguish a valid zero from missing data and a stale
cached value. Sensor conversion/calibration policy belongs in an explicit layer.

## Common mistakes

- Hard-coding hwmon index or channel order.
- Treating sysfs sensor text as a durable configuration API.
- Ignoring scale, units, timestamp, and freshness.
- Parsing input events as text or assuming one read equals one event.
- Continuing after device removal or stale sysfs topology.

## Debugging checklist

- Record subsystem, sysfs path, device identity, channel, unit, scale, and timestamp.
- Inspect driver binding, permissions, buffer mode, and event rate.
- Test stale data, overflow, invalid range, unplug/reset, and channel changes.
- Compare raw, scaled, calibrated, and product-level values.
- Correlate userspace samples with kernel and external measurement evidence.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [Serial Protocols, Timeouts, And Testing](../terminals-ttys-and-serial-userspace/serial-protocols-timeouts-and-testing.md)
- [devfs, sysfs, udev, And Device Discovery](devfs-sysfs-udev-and-discovery.md)
- [IIO userspace interface](https://www.kernel.org/doc/html/latest/iio/index.html)

## References

- [Linux input event interface](https://www.kernel.org/doc/html/latest/input/input.html)
- [Linux IIO userspace interface](https://www.kernel.org/doc/html/latest/iio/iio_devbuf.html)
- [Linux hwmon sysfs interface](https://www.kernel.org/doc/html/latest/hwmon/sysfs-interface.html)
