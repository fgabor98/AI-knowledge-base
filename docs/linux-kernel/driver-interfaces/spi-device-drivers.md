---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# SPI Device Drivers

## What Problem Does This Solve?

SPI device drivers bind to chip-select-addressed devices on an SPI controller and communicate through SPI messages and transfers.

The SPI controller handles electrical transfer mechanics. The device driver controls the peripheral protocol:

```text
chip select
clock polarity/phase
bits per word
transfer speed
command bytes
full-duplex or half-duplex transaction layout
```

## Core Concepts

- SPI controller
- SPI device
- `struct spi_device`
- `struct spi_driver`
- chip select
- SPI mode
- CPOL and CPHA
- bits per word
- maximum frequency
- `spi_message`
- `spi_transfer`
- `spi_sync()`
- `spi_write_then_read()`
- regmap over SPI
- `spidev`

## Mental Model

SPI is a controller-driven bus. A device is selected by chip select and exchanges bits according to a protocol defined by the peripheral.

```text
Device Tree node under SPI controller
-> spi_device
-> spi_driver probe
-> driver sends spi_message/spi_transfer
-> driver registers a subsystem interface
```

SPI has no universal register protocol. Read the device datasheet carefully.

## Device Tree Node

```dts
&spi1 {
    status = "okay";

    adc@0 {
        compatible = "example,spi-adc";
        reg = <0>;
        spi-max-frequency = <1000000>;
        spi-cpol;
        spi-cpha;
        vref-supply = <&vref_2v5>;
        interrupt-parent = <&gpio1>;
        interrupts = <12 IRQ_TYPE_EDGE_FALLING>;
    };
};
```

Important properties:

| Property | Meaning |
| --- | --- |
| `reg` | Chip select number. |
| `spi-max-frequency` | Maximum bus frequency for this device. |
| `spi-cpol` / `spi-cpha` | SPI mode bits. |
| `cs-gpios` on controller | Optional GPIO-backed chip selects. |

## Minimal SPI Driver

```c
#include <linux/module.h>
#include <linux/of.h>
#include <linux/spi/spi.h>

struct demo_spi {
    struct device *dev;
    struct spi_device *spi;
};

static int demo_probe(struct spi_device *spi)
{
    struct demo_spi *priv;

    priv = devm_kzalloc(&spi->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &spi->dev;
    priv->spi = spi;
    spi_set_drvdata(spi, priv);

    dev_info(&spi->dev, "SPI device probed, max speed %u Hz\n",
             spi->max_speed_hz);
    return 0;
}

static void demo_remove(struct spi_device *spi)
{
    struct demo_spi *priv = spi_get_drvdata(spi);

    dev_info(priv->dev, "removed\n");
}

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,spi-demo" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct spi_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .driver = {
        .name = "spi-demo",
        .of_match_table = demo_of_match,
    },
};
module_spi_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal SPI device driver");
```

## Simple Write-Then-Read

Many devices use command byte plus response:

```c
u8 cmd = DEMO_CMD_READ_ID;
u8 id;
int ret;

ret = spi_write_then_read(spi, &cmd, 1, &id, 1);
if (ret)
    return dev_err_probe(&spi->dev, ret, "failed to read id\n");
```

This is simple and common for short transactions.

## SPI Messages And Transfers

Use explicit messages for multi-part transactions:

```c
static int demo_read_reg(struct spi_device *spi, u8 reg, u8 *val)
{
    u8 tx = DEMO_READ_BIT | reg;
    struct spi_transfer xfers[] = {
        {
            .tx_buf = &tx,
            .len = 1,
        },
        {
            .rx_buf = val,
            .len = 1,
        },
    };

    return spi_sync_transfer(spi, xfers, ARRAY_SIZE(xfers));
}
```

Some devices require chip select to stay asserted across transfers in one message. The SPI core handles that according to the message and controller capabilities.

## Full-Duplex Transfers

SPI is often full-duplex electrically. A byte is received while a byte is transmitted.

Example:

```c
u8 tx[2] = { DEMO_READ_STATUS, 0 };
u8 rx[2];

ret = spi_write_then_read(spi, tx, 1, &rx[1], 1);
```

or:

```c
struct spi_transfer xfer = {
    .tx_buf = tx,
    .rx_buf = rx,
    .len = sizeof(tx),
};

ret = spi_sync_transfer(spi, &xfer, 1);
```

Check whether the first received byte is meaningful or just a dummy while the command is sent.

## Regmap Over SPI

For register-style SPI devices:

```c
static const struct regmap_config demo_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = DEMO_MAX_REG,
};

priv->regmap = devm_regmap_init_spi(spi, &demo_regmap_config);
if (IS_ERR(priv->regmap))
    return dev_err_probe(&spi->dev, PTR_ERR(priv->regmap),
                         "failed to init regmap\n");

ret = regmap_read(priv->regmap, DEMO_REG_ID, &id);
```

Regmap can handle common register read/write framing, caching, and update-bits operations.

## Mode And Speed

Check the device datasheet for:

- SPI mode 0/1/2/3
- maximum frequency
- bits per word
- chip-select setup/hold timing
- whether transfers are MSB-first or LSB-first
- dummy cycles

Device Tree usually supplies max frequency and mode bits. The driver can validate:

```c
if (spi->max_speed_hz > DEMO_MAX_SPI_HZ)
    dev_warn(&spi->dev, "speed above documented maximum\n");
```

Do not silently override board policy without a reason.

## Chip Select Issues

Common chip-select problems:

- wrong `reg` chip-select number
- active-high chip select not described correctly
- GPIO chip-select polarity mismatch
- controller-native and GPIO chip selects mixed incorrectly
- chip select toggles between transfers when device expects it held

Use a logic analyzer to confirm:

- clock phase/polarity
- chip select assertion
- command bytes
- dummy cycles
- response timing

## Interrupts In SPI Drivers

SPI transfers can sleep. Use threaded IRQs:

```c
static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_spi *priv = data;
    unsigned int status;

    regmap_read(priv->regmap, DEMO_STATUS, &status);
    demo_handle_status(priv, status);
    return IRQ_HANDLED;
}
```

Request:

```c
ret = devm_request_threaded_irq(&spi->dev, spi->irq,
                                NULL, demo_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(&spi->dev), priv);
```

## `spidev` And Product Drivers

`spidev` exposes raw SPI access to userspace. It is useful for experiments, but a product device usually needs a real driver when it has:

- interrupts
- power sequencing
- standard subsystem ABI
- concurrency requirements
- security restrictions
- suspend/resume behavior

Do not use generic `spidev` as a substitute for a real kernel driver unless the product explicitly accepts raw SPI as its ABI.

## Debugging SPI Devices

List devices:

```sh
ls /sys/bus/spi/devices
```

Inspect device:

```sh
cat /sys/bus/spi/devices/spi1.0/modalias
readlink /sys/bus/spi/devices/spi1.0/driver
```

Logs:

```sh
dmesg | grep -i spi
```

Runtime Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'spi-max-frequency|spi-cpol|spi-cpha|adc@' /tmp/running.dts
```

Electrical debug:

- logic analyzer
- scope on clock/chip-select
- verify pinmux
- verify power/reset

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| device never probes | node missing, wrong compatible, driver missing | DT, modinfo |
| all reads return `0xff` | MISO floating, CS wrong, power missing | logic analyzer |
| all reads return `0x00` | device held reset, mode wrong, wiring | reset, mode |
| wrong register data | dummy byte/protocol/endian mistake | datasheet |
| intermittent data | speed too high, signal integrity | lower speed, scope |
| sleeping warning in IRQ | SPI transfer in hard IRQ | threaded IRQ |

## Common Mistakes

- Confusing chip select number with bus number.
- Ignoring SPI mode.
- Forgetting dummy bytes or full-duplex behavior.
- Using `spidev` as final product ABI.
- Doing SPI transfers in hard IRQ context.
- Debugging protocol before checking pinmux, power, reset, and chip select.
- Not using regmap for register-style devices where it would simplify code.

## Practice Exercises

### Exercise 1: Minimal Probe

Add a DT node under an SPI controller and write a driver that logs `spi->chip_select` and `spi->max_speed_hz`.

### Exercise 2: Read An ID Register

Use `spi_write_then_read()` to send an ID command and read one byte. Confirm with a logic analyzer if possible.

### Exercise 3: Convert To Regmap

If the device has normal registers, replace manual read/write helpers with regmap.

## Debugging Checklist

- Is the SPI controller enabled?
- Is the device node under the correct controller?
- Is `reg` the correct chip select?
- Are mode, speed, and bits-per-word correct?
- Is pinctrl applied?
- Is power/reset sequencing complete?
- Does the compatible string match?
- Is the transaction shape correct on a logic analyzer?
- Are SPI operations only in sleepable context?

## Related Topics

- [Regmap](regmap.md)
- [Pinctrl](pinctrl.md)
- [Threaded Interrupts](threaded-interrupts.md)
- [GPIO Consumer API](gpio-consumer-api.md)
- [IIO Subsystem](iio-subsystem.md)

## Official References

- [Overview of Linux kernel SPI support](https://docs.kernel.org/spi/spi-summary.html)
- [SPI userspace API](https://docs.kernel.org/spi/spidev.html)
- [Regmap](https://docs.kernel.org/driver-api/regmap.html)
