---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# IIO Triggers And Buffers

## What Problem Does This Solve?

IIO triggers and buffers support sampled data capture where userspace needs streams instead of one-off sysfs reads.

Sysfs attributes are good for slow state reads:

```text
cat in_voltage0_raw
```

Buffers are for repeated samples:

```text
enable channels
select trigger
enable buffer
read packed samples from /dev/iio:deviceX
```

## Core Concepts

- trigger
- triggered buffer
- poll function
- scan elements
- active scan mask
- timestamp
- buffer enable
- sample layout
- hardware FIFO
- data-ready interrupt
- userspace buffered reads
- `iio_push_to_buffers_with_timestamp()`

## Mental Model

Triggers decide when a sample is captured. Buffers define how samples are delivered to userspace.

```text
trigger event
-> driver reads selected channels
-> driver packs scan data
-> driver pushes sample + timestamp
-> userspace reads from IIO character device
```

The sample layout is ABI. Userspace must know which scan elements are enabled and how they are stored.

## When To Use Buffers

Use buffered IIO when:

- samples repeat over time
- timing matters
- userspace needs many samples
- device has data-ready IRQ
- device has hardware FIFO
- sysfs reads are too slow or imprecise

Do not use buffers for a single occasional configuration value.

## Channel Scan Definition

Example channel:

```c
{
    .type = IIO_VOLTAGE,
    .indexed = 1,
    .channel = 0,
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
    .scan_index = 0,
    .scan_type = {
        .sign = 'u',
        .realbits = 12,
        .storagebits = 16,
        .endianness = IIO_CPU,
    },
},
IIO_CHAN_SOFT_TIMESTAMP(1),
```

Important fields:

| Field | Meaning |
| --- | --- |
| `scan_index` | Position in scan layout. |
| `realbits` | Meaningful bits. |
| `storagebits` | Storage size in buffer. |
| `shift` | Bit shift if data is aligned differently. |
| `endianness` | Byte order in buffer. |

## Triggered Buffer Setup

High-level shape:

```c
ret = devm_iio_triggered_buffer_setup(dev, indio_dev,
                                      NULL,
                                      demo_trigger_handler,
                                      NULL);
if (ret)
    return ret;
```

Trigger handler:

```c
static irqreturn_t demo_trigger_handler(int irq, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct demo_iio *priv = iio_priv(indio_dev);
    __le16 data[2];
    int ret;

    ret = demo_read_sample(priv, data);
    if (!ret)
        iio_push_to_buffers_with_timestamp(indio_dev, data,
                                           iio_get_time_ns(indio_dev));

    iio_trigger_notify_done(indio_dev->trig);
    return IRQ_HANDLED;
}
```

Actual data layout must match active scan elements.

## Active Scan Mask

Userspace enables channels:

```sh
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage0_en
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_timestamp_en
```

Driver can inspect:

```c
if (test_bit(0, indio_dev->active_scan_mask))
    read_channel_0();
```

Use active scan masks to avoid reading channels userspace did not enable, unless hardware forces grouped reads.

## Userspace Buffer Flow

Typical userspace setup:

```sh
cd /sys/bus/iio/devices/iio:device0
echo 1 > scan_elements/in_voltage0_en
echo 1 > scan_elements/in_timestamp_en
echo trigger0 > trigger/current_trigger
echo 16 > buffer/length
echo 1 > buffer/enable
```

Read:

```sh
dd if=/dev/iio:device0 bs=32 count=10 2>/dev/null | hexdump -C
```

Disable:

```sh
echo 0 > buffer/enable
```

The exact device node and trigger name depend on system setup.

## Data-Ready IRQ As Trigger Source

Many sensors have a data-ready interrupt.

Flow:

```text
data ready IRQ
-> threaded IRQ or trigger poll
-> IIO trigger handler reads sample
-> push to buffer
```

Do not perform I2C/SPI reads in hard IRQ context. Use threaded IRQ or IIO triggered buffer helpers.

## Hardware FIFO

If hardware has a FIFO:

- read all available samples in bounded chunks
- preserve sample order
- handle overrun flags
- push samples with correct timestamps or document timing
- avoid blocking indefinitely in trigger handler

Hardware FIFO support is more complex than single-sample triggered capture. Start with direct mode and simple triggered buffer before adding FIFO logic.

## Timestamping

Use IIO timestamp helpers:

```c
iio_get_time_ns(indio_dev)
```

or appropriate hardware timestamps if supported.

Timestamp must correspond to the sample time as closely as practical.

## Buffer Enable And Disable

Drivers may need to configure hardware when buffers are enabled:

- set sampling frequency
- enable data-ready interrupt
- enable FIFO
- start conversion
- stop conversion when disabled

Use IIO buffer setup callbacks where appropriate. Do not keep hardware streaming after the buffer is disabled unless the driver design requires it.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| no `/dev/iio:deviceX` | buffer support not registered | triggered buffer setup |
| enable fails | no trigger or scan elements invalid | sysfs errors |
| sample layout wrong | scan_type mismatch | scan_elements files |
| timestamp missing | timestamp channel not enabled/pushed | scan mask |
| overrun | userspace too slow or FIFO handling wrong | buffer length, logs |
| I2C sleep warning | read in hard IRQ | threaded trigger path |

## Common Mistakes

- Using sysfs reads for high-rate data.
- Pushing data that does not match enabled scan elements.
- Forgetting timestamp channel layout.
- Ignoring active scan mask.
- Doing unbounded FIFO drains in a handler.
- Not disabling hardware when buffer is disabled.
- Assuming userspace knows sample layout without reading scan elements.

## Practice Exercises

### Exercise 1: Enable A Buffer

For an existing IIO device, inspect `scan_elements`, enable one channel and timestamp, then read from `/dev/iio:deviceX`.

### Exercise 2: Add A Timestamp

Add `IIO_CHAN_SOFT_TIMESTAMP()` to a dummy driver and verify timestamp bytes appear in the buffer layout.

### Exercise 3: Compare Sysfs And Buffer Reads

Read one channel through sysfs and then through a buffer. Explain the differences in timing and format.

## Debugging Checklist

- Does the driver register buffer support?
- Are scan elements correct?
- Are active channels handled correctly?
- Is a trigger selected?
- Does the handler push data with the expected layout?
- Is timestamp handling correct?
- Does hardware stop when buffer is disabled?
- Is userspace reading with the right sample size?

## Related Topics

- [IIO Subsystem](iio-subsystem.md)
- [IIO Channels And Sysfs](iio-channels-and-sysfs.md)
- [IRQ Handling](irq-handling.md)
- [Threaded Interrupts](threaded-interrupts.md)

## Official References

- [Industrial I/O](https://docs.kernel.org/driver-api/iio/index.html)
- [IIO buffers](https://docs.kernel.org/driver-api/iio/buffers.html)
