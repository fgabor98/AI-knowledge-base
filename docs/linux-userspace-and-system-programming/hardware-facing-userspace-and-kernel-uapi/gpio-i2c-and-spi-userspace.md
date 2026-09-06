---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# GPIO, I2C, And SPI Userspace

## What problem does this solve?

Direct bus access from userspace can be appropriate for controlled board functions,
but it competes with kernel drivers and must respect ownership, electrical timing,
permissions, and transaction semantics. Prefer standard subsystem UAPIs and existing
drivers over raw bus poking.

## GPIO

Use the modern GPIO character-device UAPI where supported. Request lines with a
consumer label, direction, initial value, edge behavior, and bias/drive policy. A
line request is an ownership claim; handle `EBUSY`, hotplug, and release. Avoid old
global GPIO numbers and sysfs GPIO for new designs.

## I2C

`/dev/i2c-*` exposes adapter transactions through ioctl. Check adapter functionality,
target address, combined-message semantics, retries, timeouts, and whether a kernel
driver already owns the client. A successful write may only mean bytes reached the
adapter; device register semantics and commit behavior are separate.

## SPI

`/dev/spidev*` exposes controlled transfers with mode, bits-per-word, speed, and
transfer framing. Chip select, delays, half-duplex, and maximum transfer size are
device/platform-specific. Validate mode and speed at every open/configuration and
serialize access to shared devices.

## Common mistakes

- Using global GPIO numbers or assuming line identity across boards.
- Claiming an I2C/SPI device already owned by a kernel driver.
- Treating bus ACK or ioctl success as device-operation success.
- Ignoring bus speed, mode, address, transfer length, and timeout limits.
- Running raw bus tools with broad privileges in production.

## Debugging checklist

- Record device-tree identity, adapter/bus, address/CS, mode, speed, and owner.
- Check permissions, driver binding, clock/power, and electrical wiring.
- Capture transactions with kernel tracing or bus analyzer where safe.
- Test NACK, arbitration loss, timeout, stuck bus, reset, and unplug.
- Verify retries do not repeat non-idempotent register operations.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)
- [Device Tree](../../device-tree/index.md)
- [GPIO, I2C, And SPI in embedded systems](../../c/embedded-c-and-hardware/peripheral-drivers.md)

## References

- [GPIO character device userspace API](https://www.kernel.org/doc/html/latest/userspace-api/gpio/chardev.html)
- [`i2c-dev(4)`](https://man7.org/linux/man-pages/man4/i2c-dev.4.html)
- [`spidev(4)`](https://www.kernel.org/doc/html/latest/spi/spidev.html)
