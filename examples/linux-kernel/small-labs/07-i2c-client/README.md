# Lab 7: I2C Client Driver

This checkpoint uses the kernel `i2c-stub` adapter when available. The helper
module creates a client at a selected bus/address, and the driver performs one
SMBus byte-data read during `probe()`. A real I2C sensor can be used instead
by describing it in Device Tree and omitting the helper.

First check whether the stub adapter is available:

```sh
sudo modprobe i2c-stub chip_addr=0x48
ls /sys/bus/i2c/devices/
```

Then build and load the driver and helper:

```sh
make
sudo insmod ./lab_i2c.ko
sudo insmod ./lab_i2c_client.ko bus=0 address=0x48
dmesg | tail -n 30
```

The adapter number may differ; use `i2cdetect -l` or inspect
`/sys/bus/i2c/devices/` and pass the correct `bus=` value. Expected evidence
includes an I2C client and a `register 0x00` probe log.

Unload the helper before the driver:

```sh
sudo rmmod lab_i2c_client
sudo rmmod lab_i2c
sudo modprobe -r i2c-stub
```

I2C transfers may sleep. This is why later interrupt work for a bus-connected
device belongs in a threaded IRQ or workqueue rather than a hard IRQ handler.

