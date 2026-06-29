---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IIO Channels And Sysfs

## What Problem Does This Solve?

IIO channel definitions create standardized sysfs attributes for sensor and converter data.

Instead of manually creating:

```text
/sys/.../adc0
/sys/.../adc0_scale
```

the driver describes channels and the IIO core generates standard files:

```text
in_voltage0_raw
in_voltage_scale
```

## Core Concepts

- `struct iio_chan_spec`
- channel type
- indexed channels
- modified channels
- channel number
- `read_raw()`
- `write_raw()`
- raw attribute
- scale attribute
- offset attribute
- sampling frequency
- info masks
- `IIO_VAL_*` return formats
- generated sysfs names

## Mental Model

IIO sysfs files are generated from channel descriptions and callbacks.

```text
iio_chan_spec says:
  voltage channel 0 has RAW
  all voltage channels share SCALE

IIO core creates:
  in_voltage0_raw
  in_voltage_scale

userspace reads:
  driver read_raw() callback
```

The driver describes what each channel means.

## Channel Specification

Voltage channel:

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

Generated files:

```text
in_voltage0_raw
in_voltage_scale
```

If each channel has its own scale:

```c
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
                      BIT(IIO_CHAN_INFO_SCALE),
```

Generated:

```text
in_voltage0_raw
in_voltage0_scale
```

## Multiple Channels

```c
static const struct iio_chan_spec demo_channels[] = {
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 0,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
    {
        .type = IIO_VOLTAGE,
        .indexed = 1,
        .channel = 1,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
    },
};
```

Files:

```text
in_voltage0_raw
in_voltage1_raw
in_voltage_scale
```

## Modified Channels

For axis-based sensors:

```c
{
    .type = IIO_ACCEL,
    .modified = 1,
    .channel2 = IIO_MOD_X,
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),
},
```

Generated:

```text
in_accel_x_raw
in_accel_scale
```

Use modified channels for semantic axes or components, not arbitrary labels.

## `read_raw()`

```c
static int demo_read_raw(struct iio_dev *indio_dev,
                         struct iio_chan_spec const *chan,
                         int *val, int *val2, long mask)
{
    struct demo_iio *priv = iio_priv(indio_dev);
    int ret;
    unsigned int regval;

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        ret = demo_read_channel(priv, chan->channel, &regval);
        if (ret)
            return ret;

        *val = regval;
        return IIO_VAL_INT;

    case IIO_CHAN_INFO_SCALE:
        *val = 0;
        *val2 = 805; /* 0.000805 */
        return IIO_VAL_INT_PLUS_MICRO;

    default:
        return -EINVAL;
    }
}
```

Return format tells the IIO core how to print `val` and `val2`.

## Common `IIO_VAL_*` Formats

| Return | Meaning |
| --- | --- |
| `IIO_VAL_INT` | `*val` is an integer. |
| `IIO_VAL_INT_PLUS_MICRO` | `*val + *val2 / 1000000`. |
| `IIO_VAL_INT_PLUS_NANO` | `*val + *val2 / 1000000000`. |
| `IIO_VAL_FRACTIONAL` | `*val / *val2`. |
| `IIO_VAL_FRACTIONAL_LOG2` | `*val / 2^*val2`. |

Use the format that represents the device scaling accurately.

## Scale Examples

12-bit ADC with 3.3 V reference:

```text
scale = 3300 mV / 4096 = 0.805664 mV per count
```

IIO voltage scale is commonly in volts unless the channel ABI says otherwise. A driver may return:

```c
*val = 0;
*val2 = 805664;
return IIO_VAL_INT_PLUS_NANO;
```

Check the IIO ABI for the exact expected unit for each channel type.

## Offset

Temperature example:

```c
case IIO_CHAN_INFO_OFFSET:
    *val = -273150;
    return IIO_VAL_INT;
```

Use offset only when the ABI and conversion formula require it.

## Writable Attributes

Some values can be writable:

```c
static int demo_write_raw(struct iio_dev *indio_dev,
                          struct iio_chan_spec const *chan,
                          int val, int val2, long mask)
{
    switch (mask) {
    case IIO_CHAN_INFO_SAMP_FREQ:
        return demo_set_sample_frequency(iio_priv(indio_dev), val);
    default:
        return -EINVAL;
    }
}
```

Add to info:

```c
static const struct iio_info demo_info = {
    .read_raw = demo_read_raw,
    .write_raw = demo_write_raw,
};
```

Writable attributes must validate values and update hardware safely.

## Available Attributes

Drivers can expose available values for settings such as sampling frequency:

```text
sampling_frequency_available
```

Use IIO mechanisms where appropriate instead of custom attributes.

## Debugging Generated Attributes

List:

```sh
ls /sys/bus/iio/devices/iio:device0
```

Read:

```sh
cat /sys/bus/iio/devices/iio:device0/in_voltage0_raw
cat /sys/bus/iio/devices/iio:device0/in_voltage_scale
```

Inspect all:

```sh
for f in /sys/bus/iio/devices/iio:device0/in_*; do
    echo "$f=$(cat "$f" 2>/dev/null)"
done
```

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| expected file missing | info mask wrong | channel spec |
| file name unexpected | channel type/index/modified wrong | channel fields |
| scale value wrong | unit/format error | `IIO_VAL_*`, ABI |
| read returns error | transport or channel selection bug | dmesg, regmap |
| all channels same value | ignored `chan->channel` | read_raw implementation |
| writable attr accepts bad value | missing validation | write_raw |

## Common Mistakes

- Creating custom sysfs files instead of IIO channel attributes.
- Returning processed values from `_raw`.
- Getting scale units wrong.
- Ignoring `chan->channel`.
- Using separate scale where shared scale is correct, or vice versa.
- Forgetting `input_sync` equivalent is not relevant here; IIO handles sysfs reads through callbacks.
- Making high-rate data available only through sysfs reads.

## Practice Exercises

### Exercise 1: Four Voltage Channels

Define four indexed voltage channels with raw values and shared scale. Confirm generated filenames.

### Exercise 2: Per-Channel Scale

Move scale from shared-by-type to separate and observe sysfs filename changes.

### Exercise 3: Validate `read_raw()`

Inject different dummy values per channel and prove userspace sees the right values.

## Debugging Checklist

- Does each channel have the correct type?
- Are indexed/modified fields correct?
- Are info masks generating the intended files?
- Does `read_raw()` handle every mask correctly?
- Does `read_raw()` use `chan->channel` or `channel2` correctly?
- Are units and `IIO_VAL_*` formats correct?
- Does userspace need buffered capture instead of sysfs reads?

## Related Topics

- [IIO Subsystem](iio-subsystem.md)
- [IIO Triggers And Buffers](iio-triggers-and-buffers.md)
- [Sysfs Attributes](../fundamentals/sysfs-attributes.md)
- [Device Classes, Uevents, And udev](../fundamentals/device-classes-uevents-and-udev.md)

## Official References

- [Industrial I/O](https://docs.kernel.org/driver-api/iio/index.html)
- [IIO core elements](https://docs.kernel.org/driver-api/iio/core.html)
