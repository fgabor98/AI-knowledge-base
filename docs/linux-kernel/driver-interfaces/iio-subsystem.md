---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IIO Subsystem

## What Problem Does This Solve?

The Industrial I/O subsystem supports sensors, ADCs, DACs, IMUs, light sensors, pressure sensors, temperature sensors, and other data-acquisition devices.

Use IIO when a device exposes measured or generated physical quantities:

- voltage
- current
- temperature
- acceleration
- angular velocity
- magnetic field
- pressure
- light
- proximity
- raw ADC counts

IIO gives userspace a standard ABI instead of a private character device or custom sysfs layout.

## Core Concepts

- IIO device
- `struct iio_dev`
- private driver state
- IIO channel
- channel type
- channel attributes
- `read_raw()`
- `write_raw()`
- raw values
- scale
- offset
- sampling frequency
- events
- triggers
- buffers
- scan elements
- sysfs ABI

## Mental Model

IIO separates device data channels from board details and userspace presentation.

```text
driver describes channels
-> IIO core creates standard sysfs attributes
-> driver implements callbacks for raw/scale/offset
-> userspace reads standard in_* files or buffered samples
```

The driver should describe what each channel means, not invent a new file layout.

## When To Use IIO

Use IIO for:

- ADCs
- DACs
- temperature sensors
- accelerometers
- gyroscopes
- magnetometers
- pressure sensors
- light/proximity sensors
- industrial measurement devices
- sampled data streams

Do not use IIO for:

- buttons and keys: use input
- networking: use netdev
- audio streams: use ALSA/ASoC
- simple board control GPIOs: use GPIO/LED/regulator/reset as appropriate
- debug-only internal data: use debugfs or tracing

## Minimal IIO Device Shape

```c
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/module.h>
#include <linux/platform_device.h>

struct demo_iio {
    struct device *dev;
    int raw_value;
};

static int demo_read_raw(struct iio_dev *indio_dev,
                         struct iio_chan_spec const *chan,
                         int *val, int *val2, long mask)
{
    struct demo_iio *priv = iio_priv(indio_dev);

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        *val = priv->raw_value;
        return IIO_VAL_INT;
    case IIO_CHAN_INFO_SCALE:
        *val = 0;
        *val2 = 1000;
        return IIO_VAL_INT_PLUS_MICRO;
    default:
        return -EINVAL;
    }
}

static const struct iio_info demo_info = {
    .read_raw = demo_read_raw,
};
```

Channels:

```c
static const struct iio_chan_spec demo_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 0,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
};
```

Probe:

```c
static int demo_probe(struct platform_device *pdev)
{
    struct iio_dev *indio_dev;
    struct demo_iio *priv;

    indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*priv));
    if (!indio_dev)
        return -ENOMEM;

    priv = iio_priv(indio_dev);
    priv->dev = &pdev->dev;
    priv->raw_value = 1234;

    indio_dev->name = "demo-iio";
    indio_dev->modes = INDIO_DIRECT_MODE;
    indio_dev->info = &demo_info;
    indio_dev->channels = demo_channels;
    indio_dev->num_channels = ARRAY_SIZE(demo_channels);

    return devm_iio_device_register(&pdev->dev, indio_dev);
}
```

## Generated Sysfs ABI

The channel above can create files like:

```text
/sys/bus/iio/devices/iio:deviceX/in_voltage0_raw
/sys/bus/iio/devices/iio:deviceX/in_voltage_scale
```

Userspace:

```sh
cat /sys/bus/iio/devices/iio:device0/name
cat /sys/bus/iio/devices/iio:device0/in_voltage0_raw
cat /sys/bus/iio/devices/iio:device0/in_voltage_scale
```

Scale and offset let userspace convert raw values into physical units according to IIO ABI rules.

## IIO Driver Over I2C Or SPI

Most real IIO devices are I2C or SPI clients:

```text
i2c_driver or spi_driver
-> regmap for register access
-> iio_device_alloc
-> channels and iio_info callbacks
-> iio_device_register
```

Example flow:

```c
priv->regmap = devm_regmap_init_i2c(client, &config);
indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*priv));
```

IIO is the userspace-facing subsystem. I2C/SPI is just the transport.

## Raw, Scale, And Offset

Raw:

```text
in_voltage0_raw
```

Scale:

```text
in_voltage_scale
```

Offset:

```text
in_temp_offset
```

Userspace conversion often follows:

```text
physical = (raw + offset) * scale
```

Check IIO documentation for exact channel semantics and units.

## Direct Mode Versus Buffered Mode

Direct mode:

```text
userspace reads sysfs attribute
driver performs one measurement/read
```

Buffered mode:

```text
trigger fires repeatedly
driver pushes samples into IIO buffer
userspace reads sample stream
```

Use direct mode for slow, occasional reads. Use buffers for repeated samples where timing and layout matter.

## Events

IIO events represent threshold crossings, data-ready signals, or device-specific conditions through the IIO event ABI.

Use events when userspace needs notification about conditions, not a continuous data stream.

Examples:

- threshold crossed
- rising/falling event
- data ready
- overtemperature

## Debugging IIO Devices

List devices:

```sh
ls /sys/bus/iio/devices
```

Inspect:

```sh
for d in /sys/bus/iio/devices/iio:device*; do
    echo "$d: $(cat "$d/name" 2>/dev/null)"
done
```

Read attributes:

```sh
cat /sys/bus/iio/devices/iio:device0/in_voltage0_raw
```

Check logs:

```sh
dmesg | grep -i iio
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no iio device appears | driver did not register IIO device | probe logs |
| attributes missing | channel masks wrong | channel specs |
| raw read fails | transport/register error | regmap/I2C/SPI |
| units wrong | scale/offset semantics wrong | IIO ABI docs |
| buffered data wrong | scan layout mismatch | scan elements |
| userspace cannot identify device | `indio_dev->name` missing/wrong | sysfs name |

## Common Mistakes

- Creating custom sysfs files for sensor data instead of IIO channels.
- Reporting physical units directly in `_raw` attributes.
- Getting scale units wrong.
- Reusing channel numbers inconsistently.
- Registering IIO before hardware is initialized.
- Ignoring buffered mode when userspace needs streams.
- Implementing transport logic in userspace instead of a proper IIO driver.

## Practice Exercises

### Exercise 1: Dummy Voltage Channel

Register one IIO voltage channel with raw and scale attributes. Read generated sysfs files.

### Exercise 2: Add A Second Channel

Add channel 1 and confirm generated names are `in_voltage0_raw` and `in_voltage1_raw`.

### Exercise 3: Connect To I2C/SPI

Replace the dummy raw value with a real register read through regmap.

## Debugging Checklist

- Does probe allocate and register an `iio_dev`?
- Is `indio_dev->name` set?
- Are channel types and numbers correct?
- Are `info_mask_*` fields correct?
- Does `read_raw()` return the right `IIO_VAL_*` format?
- Are scale and offset documented and unit-correct?
- Does userspace need direct reads, events, or buffers?

## Related Topics

- [IIO Channels And Sysfs](iio-channels-and-sysfs.md)
- [IIO Triggers And Buffers](iio-triggers-and-buffers.md)
- [I2C Client Drivers](i2c-client-drivers.md)
- [SPI Device Drivers](spi-device-drivers.md)
- [Regmap](regmap.md)

## Official References

- [Industrial I/O](https://docs.kernel.org/driver-api/iio/index.html)
- [IIO core elements](https://docs.kernel.org/driver-api/iio/core.html)
