---
status: active
reviewed: false
domain: linux-kernel
difficulty: advanced
last_reviewed: null
---

# 5. Embedded Driver Subsystems

Official parent: [Driver implementer's API guide](https://docs.kernel.org/driver-api/index.html)

Knowledge-guide companion: [Stage 5](knowledge-guide-companion.md#stage-5-embedded-driver-subsystems)

Read the P0 sections in the order below. For each subsystem, study its consumer
API before its controller/provider API unless the project implements a provider.

## Interrupts, GPIO, Pin Control, And Regmap

- [ ] **P0** [Generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html)
- [ ] **P0** [GPIO subsystem](https://docs.kernel.org/driver-api/gpio/index.html)
- [ ] **P0** [GPIO descriptor consumer interface](https://docs.kernel.org/driver-api/gpio/consumer.html)
- [ ] **P0** [GPIO controller drivers](https://docs.kernel.org/driver-api/gpio/driver.html)
- [ ] **P0** [GPIO mappings](https://docs.kernel.org/driver-api/gpio/board.html)
- [ ] **P0** [Pinctrl subsystem](https://docs.kernel.org/driver-api/pin-control.html)
- [ ] **P0** Read the regmap kernel-doc from the exact source tree, starting at `include/linux/regmap.h`, and use the [Driver API index](https://docs.kernel.org/driver-api/index.html) to check whether the selected kernel version renders it separately.
- [ ] **P1** [GPIO mockup for testing](https://docs.kernel.org/admin-guide/gpio/gpio-mockup.html)
- [ ] **P1** [GPIO simulator](https://docs.kernel.org/admin-guide/gpio/gpio-sim.html)

## I2C, SPI, UART, And TTY

- [ ] **P0** [I2C and SMBus subsystem](https://docs.kernel.org/i2c/index.html)
- [ ] **P0** [Writing I2C client drivers](https://docs.kernel.org/i2c/writing-clients.html)
- [ ] **P0** [Instantiating I2C devices](https://docs.kernel.org/i2c/instantiating-devices.html)
- [ ] **P0** [I2C functionality](https://docs.kernel.org/i2c/functionality.html)
- [ ] **P0** [I2C fault codes](https://docs.kernel.org/i2c/fault-codes.html)
- [ ] **P0** [SPI subsystem](https://docs.kernel.org/spi/index.html)
- [ ] **P0** [Serial devices](https://docs.kernel.org/driver-api/serial/index.html)
- [ ] **P0** [TTY subsystem](https://docs.kernel.org/driver-api/tty/index.html)
- [ ] **P1** [TTY driver API](https://docs.kernel.org/driver-api/tty/tty_driver.html)
- [ ] **P1** [Serial console](https://docs.kernel.org/admin-guide/serial-console.html)

## Clocks, Resets, Regulators, Power Domains, And PWM

- [ ] **P0** [Common clock framework](https://docs.kernel.org/driver-api/clk.html)
- [ ] **P0** [Reset controller API](https://docs.kernel.org/driver-api/reset.html)
- [ ] **P0** [Regulator consumer API](https://docs.kernel.org/power/regulator/consumer.html)
- [ ] **P0** [Regulator driver API](https://docs.kernel.org/power/regulator/regulator.html)
- [ ] **P0** Study generic PM domains through the [device power-management guide](https://docs.kernel.org/driver-api/pm/index.html), `include/linux/pm_domain.h`, and `drivers/pmdomain/` in the selected source tree.
- [ ] **P0** [PWM interface](https://docs.kernel.org/driver-api/pwm.html)
- [ ] **P1** [Power sequencing API](https://docs.kernel.org/driver-api/pwrseq.html)

## IIO, Input, CAN, And Networking

- [ ] **P0** [Industrial I/O](https://docs.kernel.org/driver-api/iio/index.html)
- [ ] **P0** [IIO core elements](https://docs.kernel.org/driver-api/iio/core.html)
- [ ] **P0** [IIO buffers](https://docs.kernel.org/driver-api/iio/buffers.html)
- [ ] **P0** [IIO triggers](https://docs.kernel.org/driver-api/iio/triggers.html)
- [ ] **P0** [Input subsystem](https://docs.kernel.org/input/index.html)
- [ ] **P0** [Input programming](https://docs.kernel.org/input/input-programming.html)
- [ ] **P0** [SocketCAN](https://docs.kernel.org/networking/can.html)
- [ ] **P1** [Networking driver documentation](https://docs.kernel.org/networking/device_drivers/index.html)
- [ ] **P1** [PHY abstraction layer](https://docs.kernel.org/networking/phy.html)

## Storage, NVMEM, Watchdog, And Other Board Interfaces

- [ ] **P0** [NVMEM subsystem](https://docs.kernel.org/driver-api/nvmem.html)
- [ ] **P0** [Watchdog driver API](https://docs.kernel.org/watchdog/watchdog-kernel-api.html)
- [ ] **P0** [Watchdog userspace API](https://docs.kernel.org/watchdog/watchdog-api.html)
- [ ] **P1** [MTD documentation](https://docs.kernel.org/driver-api/mtd/index.html)
- [ ] **P1** [MMC/SD/SDIO](https://docs.kernel.org/driver-api/mmc/index.html)
- [ ] **P1** [Thermal framework](https://docs.kernel.org/driver-api/thermal/index.html)
- [ ] **P1** [Hardware monitoring](https://docs.kernel.org/hwmon/index.html)
- [ ] **P1** [RTC subsystem](https://docs.kernel.org/admin-guide/rtc.html)
- [ ] **P1** [LED subsystem](https://docs.kernel.org/leds/index.html)

## Subsystem Study Template

Complete this for every subsystem used by a project:

- [ ] Read its conceptual overview and public API documentation.
- [ ] Read its relevant Device Tree schemas.
- [ ] Locate the consumer and provider headers.
- [ ] Read one simple upstream driver and one TI/vendor-relevant driver.
- [ ] Identify registration, per-device state, callbacks, locking, and teardown.
- [ ] Identify the standard userspace ABI and diagnostic tools.
- [ ] Record runtime PM and suspend/resume behavior.
- [ ] Build with relevant warnings and test at least one failure path.
