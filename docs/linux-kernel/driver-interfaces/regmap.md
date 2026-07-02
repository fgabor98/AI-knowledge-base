---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# Regmap

## What Problem Does This Solve?

Regmap centralizes register access for devices connected through buses such as I2C, SPI, MMIO, or custom transports.

Instead of open-coding register read/write helpers in each driver, regmap lets the driver describe the register layout once and use a consistent API:

```text
regmap_read()
regmap_write()
regmap_update_bits()
regmap_bulk_read()
regmap_fields
```

It can also provide caching, locking, access validation, debugfs inspection, and bus-independent code.

## Core Concepts

- `struct regmap`
- `struct regmap_config`
- register width
- value width
- register stride
- max register
- readable/writeable/volatile/precious registers
- caching
- endianness
- `regmap_read()`
- `regmap_write()`
- `regmap_update_bits()`
- bulk reads and writes
- regmap fields
- regmap over I2C
- regmap over SPI
- regmap over MMIO

## Mental Model

Regmap is a register access layer between the driver and the transport.

```text
driver
  wants register 0x10 bit 3 set

regmap
  knows register/value widths, cache, volatility, access rules

transport
  I2C, SPI, MMIO, or custom bus operation
```

The same driver logic becomes easier to read and easier to adapt across bus variants.

## Basic Configuration

8-bit registers, 8-bit values:

```c
static const struct regmap_config demo_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = DEMO_MAX_REG,
};
```

16-bit registers, 32-bit values:

```c
static const struct regmap_config demo_regmap_config = {
    .reg_bits = 16,
    .val_bits = 32,
    .reg_stride = 4,
    .max_register = 0x100,
};
```

Use values that match the device protocol, not the CPU register type.

## I2C Regmap

```c
priv->regmap = devm_regmap_init_i2c(client, &demo_regmap_config);
if (IS_ERR(priv->regmap))
    return dev_err_probe(&client->dev, PTR_ERR(priv->regmap),
                         "failed to init regmap\n");
```

Read:

```c
ret = regmap_read(priv->regmap, DEMO_REG_ID, &id);
if (ret)
    return ret;
```

Write:

```c
ret = regmap_write(priv->regmap, DEMO_REG_CONFIG, config);
```

## SPI Regmap

```c
priv->regmap = devm_regmap_init_spi(spi, &demo_regmap_config);
if (IS_ERR(priv->regmap))
    return dev_err_probe(&spi->dev, PTR_ERR(priv->regmap),
                         "failed to init regmap\n");
```

Some SPI devices need custom read/write flag bits, padding, or formatting. Regmap supports many common formats, but check the device protocol.

## MMIO Regmap

For memory-mapped register blocks:

```c
void __iomem *base;

base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(base))
    return PTR_ERR(base);

priv->regmap = devm_regmap_init_mmio(&pdev->dev, base,
                                     &demo_regmap_config);
if (IS_ERR(priv->regmap))
    return PTR_ERR(priv->regmap);
```

MMIO regmap can be useful when:

- the same IP block appears on different transports
- register field helpers reduce boilerplate
- debugfs visibility is valuable
- access tables improve safety

For simple high-performance MMIO paths, direct `readl()`/`writel()` may still be clearer.

## Bit Updates

Instead of:

```c
ret = regmap_read(map, REG_CTRL, &val);
if (ret)
    return ret;
val |= CTRL_ENABLE;
ret = regmap_write(map, REG_CTRL, val);
```

use:

```c
ret = regmap_update_bits(map, REG_CTRL, CTRL_ENABLE, CTRL_ENABLE);
```

Clear:

```c
ret = regmap_update_bits(map, REG_CTRL, CTRL_ENABLE, 0);
```

This centralizes locking and cache handling.

## Bulk Access

Read multiple bytes/values:

```c
ret = regmap_bulk_read(map, DEMO_DATA0, buf, len);
```

Write:

```c
ret = regmap_bulk_write(map, DEMO_DATA0, buf, len);
```

Use bulk operations when the hardware supports efficient sequential access. Be careful with FIFO registers: repeated reads from one FIFO address are not the same as sequential register reads.

## Volatile And Precious Registers

Volatile registers change without explicit writes:

- status registers
- data registers
- counters
- interrupt status
- hardware-updated measurement values

Precious registers should not be read casually because reading may clear state or consume data:

- FIFO pop registers
- clear-on-read status
- data consume registers

Example:

```c
static bool demo_volatile_reg(struct device *dev, unsigned int reg)
{
    switch (reg) {
    case DEMO_STATUS:
    case DEMO_DATA:
        return true;
    default:
        return false;
    }
}

static bool demo_precious_reg(struct device *dev, unsigned int reg)
{
    return reg == DEMO_FIFO;
}

static const struct regmap_config demo_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = DEMO_MAX_REG,
    .volatile_reg = demo_volatile_reg,
    .precious_reg = demo_precious_reg,
};
```

Do not cache volatile registers incorrectly.

## Access Tables

Restrict readable/writeable ranges:

```c
static const struct regmap_range demo_readable_ranges[] = {
    regmap_reg_range(0x00, 0x1f),
    regmap_reg_range(0x40, 0x4f),
};

static const struct regmap_access_table demo_readable_table = {
    .yes_ranges = demo_readable_ranges,
    .n_yes_ranges = ARRAY_SIZE(demo_readable_ranges),
};

static const struct regmap_config demo_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .rd_table = &demo_readable_table,
};
```

This catches driver bugs and makes debugfs safer.

## Caching

Regmap caches can reduce bus traffic and preserve state across suspend/resume.

Example:

```c
.cache_type = REGCACHE_RBTREE,
```

Common cache considerations:

- mark volatile registers
- sync cache after reset or resume
- bypass cache for diagnostic paths only when needed
- do not cache registers that hardware changes

Suspend/resume pattern:

```c
regcache_cache_only(map, true);
regcache_mark_dirty(map);
```

Resume:

```c
regcache_cache_only(map, false);
ret = regcache_sync(map);
```

Use patterns appropriate for your device and subsystem.

## Regmap Fields

For bitfields:

```c
static const struct reg_field enable_field =
    REG_FIELD(DEMO_CTRL, 0, 0);

priv->enable = devm_regmap_field_alloc(dev, map, enable_field);
if (IS_ERR(priv->enable))
    return PTR_ERR(priv->enable);

ret = regmap_field_write(priv->enable, 1);
```

This avoids scattering masks and shifts throughout the driver.

## Endianness And Formatting

Regmap can handle register and value endianness for buses where byte order matters.

Check:

- datasheet register address width
- value width
- wire order
- CPU MMIO endianness
- SPI command format
- whether multi-byte values are big-endian or little-endian

Wrong endianness often produces values that look shifted, byte-swapped, or plausible but wrong.

## Debugfs

If enabled, regmap exposes useful debugfs entries:

```sh
find /sys/kernel/debug/regmap -maxdepth 2 -type f
```

You may see:

```text
registers
access
range
```

Do not rely on debugfs as a production ABI. It is for debugging.

## When Not To Use Regmap

Regmap may be unnecessary when:

- the device has only one or two simple operations
- the protocol is not register-like
- performance-critical MMIO paths need direct access
- register access has unusual side effects not easy to model
- an existing subsystem helper already abstracts the device

Use regmap when it reduces real complexity.

## Common Failure Modes

| Symptom | Likely Cause | First Checks |
| --- | --- | --- |
| all reads fail | wrong bus/probe/power | transport and probe logs |
| values byte-swapped | endian/config mismatch | `reg_bits`, `val_bits`, endianness |
| status never changes | volatile register cached | `volatile_reg` |
| FIFO data lost | precious register read by debug/cache path | `precious_reg` |
| writes hit wrong register | register stride or address width wrong | config, datasheet |
| resume loses config | cache sync missing or reset occurred | regcache flow |

## Common Mistakes

- Treating every bus transaction as a register access when the protocol is not register-like.
- Forgetting volatile registers.
- Caching interrupt status or data registers.
- Using `regmap_update_bits()` on write-one-to-clear status registers.
- Getting register/value widths wrong.
- Ignoring endian requirements.
- Relying on debugfs in product code.

## Practice Exercises

### Exercise 1: Convert Read/Write Helpers

Replace hand-written I2C register reads with `regmap_read()` and `regmap_write()`.

### Exercise 2: Add Volatile Status

Mark a status register volatile and compare debug behavior before and after.

### Exercise 3: Use A Regmap Field

Replace one mask/shift pair with `regmap_field_read()` and `regmap_field_write()`.

## Debugging Checklist

- Do `reg_bits`, `val_bits`, and `reg_stride` match the datasheet?
- Are volatile and precious registers marked?
- Are readable/writeable ranges correct?
- Is caching appropriate?
- Is endian handling correct?
- Are bulk operations valid for the register layout?
- Does debugfs show expected registers?
- Are errors returned and logged with context?

## Related Topics

- [I2C Client Drivers](i2c-client-drivers.md)
- [SPI Device Drivers](spi-device-drivers.md)
- [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)
- [Debugfs And Sysfs Inspection](../debugging/debugfs-and-sysfs-inspection.md)

## Official References

- [Regmap](https://docs.kernel.org/driver-api/regmap.html)
- [Implementing I2C device drivers](https://docs.kernel.org/i2c/writing-clients.html)
- [Overview of Linux kernel SPI support](https://docs.kernel.org/spi/spi-summary.html)
