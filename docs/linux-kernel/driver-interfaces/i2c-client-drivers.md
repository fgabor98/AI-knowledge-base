---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# I2C Client Drivers

## What Problem Does This Solve?

I2C client drivers bind to devices on an I2C bus and communicate through the kernel I2C subsystem.

The I2C controller driver moves bytes on the bus. The client driver owns one device's protocol:

```text
I2C adapter/controller
  implements bus transfers

I2C client device
  has a 7-bit or 10-bit address

I2C client driver
  knows the device register protocol and subsystem ABI
```

## Core Concepts

- I2C adapter
- I2C client
- `struct i2c_client`
- `struct i2c_driver`
- `probe()`
- Device Tree matching
- `i2c_device_id`
- SMBus helpers
- raw I2C transfers
- regmap over I2C
- repeated start
- 7-bit address
- bus speed
- clock stretching
- `i2c-dev`

## Mental Model

The bus controller driver handles electrical transfer mechanics. The client driver should not know controller-specific details.

```text
Device Tree node under an I2C bus
-> i2c_client
-> i2c_driver match
-> probe(client)
-> client driver reads/writes device registers
-> client driver registers with IIO/input/hwmon/etc.
```

## Device Tree Node

```dts
&i2c2 {
    status = "okay";

    temperature-sensor@48 {
        compatible = "example,tmp102";
        reg = <0x48>;
        vdd-supply = <&vdd_3v3>;
        interrupt-parent = <&gpio1>;
        interrupts = <7 IRQ_TYPE_LEVEL_LOW>;
    };
};
```

The `reg` value is the I2C address, usually 7-bit. Do not include the read/write bit.

## Minimal I2C Driver

```c
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>

struct demo_i2c {
    struct device *dev;
    struct i2c_client *client;
};

static int demo_probe(struct i2c_client *client)
{
    struct demo_i2c *priv;

    priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = &client->dev;
    priv->client = client;
    i2c_set_clientdata(client, priv);

    dev_info(&client->dev, "I2C device at 0x%02x probed\n", client->addr);
    return 0;
}

static void demo_remove(struct i2c_client *client)
{
    struct demo_i2c *priv = i2c_get_clientdata(client);

    dev_info(priv->dev, "removed\n");
}

static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-i2c" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct i2c_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .driver = {
        .name = "demo-i2c",
        .of_match_table = demo_of_match,
    },
};
module_i2c_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal I2C client driver");
```

## Reading And Writing Registers

SMBus byte register helpers:

```c
ret = i2c_smbus_read_byte_data(client, DEMO_REG_ID);
if (ret < 0)
    return dev_err_probe(&client->dev, ret, "failed to read ID\n");

id = ret;
```

Write:

```c
ret = i2c_smbus_write_byte_data(client, DEMO_REG_CONFIG, config);
if (ret < 0)
    return ret;
```

These helpers are convenient for devices compatible with SMBus-style operations. Not every I2C device supports every SMBus transaction type.

## Raw I2C Transfers

Use `i2c_transfer()` when the protocol needs explicit messages:

```c
static int demo_read_block(struct i2c_client *client, u8 reg,
                           void *buf, size_t len)
{
    struct i2c_msg msgs[2] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = 1,
            .buf = &reg,
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = len,
            .buf = buf,
        },
    };
    int ret;

    ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
    if (ret < 0)
        return ret;
    if (ret != ARRAY_SIZE(msgs))
        return -EIO;

    return 0;
}
```

This produces a write of the register address followed by a read, often with a repeated start depending on adapter capability.

## Regmap Over I2C

For register-oriented devices, prefer regmap:

```c
static const struct regmap_config demo_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = DEMO_MAX_REG,
};

priv->regmap = devm_regmap_init_i2c(client, &demo_regmap_config);
if (IS_ERR(priv->regmap))
    return dev_err_probe(&client->dev, PTR_ERR(priv->regmap),
                         "failed to init regmap\n");

ret = regmap_read(priv->regmap, DEMO_REG_ID, &id);
```

Regmap gives consistent access, caching, bit updates, debugfs integration, and cleaner error handling.

## Matching Tables

Device Tree:

```c
static const struct of_device_id demo_of_match[] = {
    { .compatible = "example,demo-i2c" },
    { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Legacy/non-DT I2C ID table:

```c
static const struct i2c_device_id demo_id[] = {
    { "demo-i2c" },
    { }
};
MODULE_DEVICE_TABLE(i2c, demo_id);
```

Driver:

```c
static struct i2c_driver demo_driver = {
    .probe = demo_probe,
    .remove = demo_remove,
    .id_table = demo_id,
    .driver = {
        .name = "demo-i2c",
        .of_match_table = demo_of_match,
    },
};
```

Use the tables expected by your platform and kernel version.

## Interrupts In I2C Drivers

I2C transfers can sleep. If an I2C device has an interrupt, use a threaded IRQ:

```c
static irqreturn_t demo_irq_thread(int irq, void *data)
{
    struct demo_i2c *priv = data;
    unsigned int status;

    regmap_read(priv->regmap, DEMO_STATUS, &status);
    demo_handle_status(priv, status);
    return IRQ_HANDLED;
}

ret = devm_request_threaded_irq(&client->dev, client->irq,
                                NULL, demo_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(&client->dev), priv);
```

Many I2C clients expose `client->irq` if firmware described an interrupt.

## Power And Resources

I2C client drivers often need:

- regulators
- reset GPIOs
- IRQ GPIOs
- clocks for unusual devices
- pinctrl on the controller or board

Example:

```c
priv->vdd = devm_regulator_get(&client->dev, "vdd");
if (IS_ERR(priv->vdd))
    return dev_err_probe(&client->dev, PTR_ERR(priv->vdd),
                         "failed to get vdd\n");
```

Bring-up sequence usually follows the datasheet:

```text
enable supply
deassert reset
wait startup delay
read chip ID
configure device
register subsystem interface
```

## Userspace I2C Is Not A Driver

Tools such as `i2cget`, `i2cset`, and `i2cdetect` are useful during bring-up, but they do not replace a kernel driver when the device needs:

- interrupts
- power management
- standard subsystem ABI
- safe multi-process ownership
- suspend/resume handling
- register caching

Use `i2cdetect` cautiously. Some devices react badly to probing.

## Debugging I2C Clients

List buses/devices:

```sh
ls /sys/bus/i2c/devices
i2cdetect -l
```

Check a specific device:

```sh
readlink /sys/bus/i2c/devices/2-0048/driver
cat /sys/bus/i2c/devices/2-0048/name
```

Check logs:

```sh
dmesg | grep -i -E 'i2c|demo|defer'
```

Check Device Tree:

```sh
dtc -I fs -O dts /proc/device-tree > /tmp/running.dts
rg 'temperature-sensor|tmp102|i2c' /tmp/running.dts
```

Check bus electrically:

- pull-ups present
- correct voltage
- SCL/SDA pinmux
- bus speed supported
- address straps correct
- reset released

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| device never appears | node missing/disabled or bus disabled | runtime DT, `/sys/bus/i2c` |
| probe never runs | compatible mismatch or driver not loaded | `modinfo`, `dmesg` |
| read returns `-ENXIO` | no ACK at address | address, power, reset, wiring |
| read returns `-ETIMEDOUT` | bus stuck or controller issue | SCL/SDA, pinmux |
| intermittent errors | weak pull-ups, speed too high, power noise | scope, bus speed |
| IRQ handler warning | I2C transfer in hard IRQ | threaded IRQ |
| wrong data | endian/register protocol mistake | datasheet, regmap config |

## Common Mistakes

- Including the read/write bit in the I2C address.
- Using `i2cdetect` as proof a device is safe.
- Doing I2C transfers in hard IRQ context.
- Ignoring regulator/reset sequencing.
- Inventing a character device for sensor data instead of using IIO/hwmon/input where appropriate.
- Not checking whether SMBus helpers match the device protocol.
- Forgetting `MODULE_DEVICE_TABLE()` for autoloading.

## Practice Exercises

### Exercise 1: Minimal Probe

Add an I2C node and a driver that logs the address from `client->addr`.

### Exercise 2: Read Chip ID

Read one ID register using SMBus helpers or regmap. Return `-ENODEV` if the ID is not expected.

### Exercise 3: Convert To Regmap

Replace direct SMBus register reads with `devm_regmap_init_i2c()` and `regmap_read()`.

## Debugging Checklist

- Is the I2C bus enabled?
- Is the device node under the correct bus?
- Is the 7-bit address correct?
- Does the device have power and reset released?
- Does the compatible string match the driver?
- Does the driver use the correct transaction type?
- Are I2C operations done in sleepable context?
- Is the right subsystem ABI used?

## Related Topics

- [Regmap](regmap.md)
- [Device Tree Matching From Drivers](../fundamentals/device-tree-matching.md)
- [Threaded Interrupts](threaded-interrupts.md)
- [Regulators](regulators.md)
- [GPIO Consumer API](gpio-consumer-api.md)
- [IIO Subsystem](iio-subsystem.md)

## Official References

- [Implementing I2C device drivers](https://docs.kernel.org/i2c/writing-clients.html)
- [I2C device interface](https://docs.kernel.org/i2c/dev-interface.html)
- [Regmap](https://docs.kernel.org/driver-api/regmap.html)
