---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Small Lab Progression

## What Problem Does This Solve?

Beginners learn kernel development best by adding one concept at a time. Jumping straight into a real I2C/SPI/IRQ/Device Tree driver combines too many unknowns: build setup, module loading, device matching, hardware wiring, bus protocol, execution context, and userspace testing.

This page defines a staged lab sequence. Each lab should produce observable evidence before moving to the next.

## Core Concepts

- hello module
- module parameters
- character device
- platform driver
- Device Tree matching
- GPIO consumer
- I2C client
- SPI client
- IRQ handling
- threaded IRQ
- tracing
- known-good checkpoint

## Lab Rules

- Add one new concept per lab.
- Keep each previous lab as a known-good checkpoint.
- Record commands and expected evidence.
- Prefer fake or dummy devices before unstable hardware.
- Use a recoverable VM, QEMU target, or spare board.
- Do not mix kernel config, Device Tree, source, and userspace changes unless the lab explicitly requires it.

## Lab 1: Hello Module

Goal:

- build, load, inspect, and unload a trivial module

Concepts:

- module init
- module exit
- `pr_info`
- `insmod`
- `rmmod`
- `dmesg`

Expected code shape:

```c
static int __init hello_init(void)
{
        pr_info("hello: loaded\n");
        return 0;
}

static void __exit hello_exit(void)
{
        pr_info("hello: unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
```

Commands:

```bash
make
sudo insmod hello.ko
dmesg | tail
lsmod | grep hello
sudo rmmod hello
dmesg | tail
```

Success evidence:

- load log appears
- module appears in `lsmod`
- unload log appears

Common failures:

- invalid module format
- unknown symbol
- missing license warning
- wrong build tree

## Lab 2: Module Parameters

Goal:

- pass simple configuration into a module and inspect it

Concepts:

- `module_param`
- parameter permissions
- `/sys/module/<module>/parameters`
- input validation

Example:

```c
static int interval_ms = 1000;
module_param(interval_ms, int, 0644);
MODULE_PARM_DESC(interval_ms, "Polling interval in milliseconds");
```

Load:

```bash
sudo insmod hello_params.ko interval_ms=500
cat /sys/module/hello_params/parameters/interval_ms
```

Success evidence:

- parameter value appears in sysfs
- driver logs effective value

Common failures:

- invalid parameter type
- permissions too broad
- treating module parameters as board hardware description

## Lab 3: Dummy Character Device

Goal:

- expose a minimal userspace file interface

Concepts:

- major/minor number
- `struct cdev`
- `struct file_operations`
- `open`
- `read`
- `write`
- `release`
- `copy_to_user`
- device class
- `/dev` node

Expected behavior:

```bash
cat /dev/demo0
echo test | sudo tee /dev/demo0
```

Inspection:

```bash
ls -l /dev/demo0
udevadm info /dev/demo0
cat /proc/devices | grep demo
```

Success evidence:

- `/dev/demo0` exists
- `open` and `release` logs appear
- read/write callbacks run

Common failures:

- missing class/device creation
- wrong permissions
- bad userspace copy handling
- `rmmod` blocked by open file descriptor

## Lab 4: Convert To Platform Driver Without Real Hardware

Goal:

- understand device/driver binding without depending on real hardware

Concepts:

- `struct platform_driver`
- `struct platform_device`
- `probe`
- `remove`
- `platform_set_drvdata`
- sysfs device path

Approach:

- create a fake platform device from a helper module, or use a simple software-only platform device
- bind a platform driver to it
- keep the character device behavior from Lab 3

Inspection:

```bash
find /sys/bus/platform/devices -maxdepth 1 -name '*demo*'
readlink /sys/bus/platform/devices/demo.0/driver
udevadm info /dev/demo0
```

Success evidence:

- platform device appears
- driver symlink exists
- `probe` and `remove` logs appear

Common failures:

- driver name mismatch
- no platform device created
- module load succeeds but probe never runs

## Lab 5: Device Tree Matched Platform Driver

Goal:

- match a platform driver using `compatible`

Concepts:

- Device Tree node
- `compatible`
- `of_match_table`
- `MODULE_DEVICE_TABLE`
- runtime DTB inspection
- deployed DTB identity

Example Device Tree:

```dts
demo@48000000 {
        compatible = "example,demo";
        reg = <0x48000000 0x1000>;
        status = "okay";
};
```

Driver match:

```c
static const struct of_device_id demo_of_match[] = {
        { .compatible = "example,demo" },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Inspection:

```bash
tr '\0' '\n' < /proc/device-tree/demo@48000000/compatible
cat /sys/bus/platform/devices/*demo*/modalias
modinfo demo.ko | grep of:
```

Success evidence:

- runtime Device Tree contains the node
- platform device exists
- driver probes

Common failures:

- edited source DTS but deployed old DTB
- `status = "disabled"`
- missing `MODULE_DEVICE_TABLE`
- compatible string typo

## Lab 6: GPIO Consumer

Goal:

- request a GPIO by role and use descriptor APIs

Concepts:

- `gpiod_get`
- optional GPIOs
- active-low handling
- Device Tree GPIO property
- pinctrl dependency

Device Tree example:

```dts
demo {
        compatible = "example,demo";
        reset-gpios = <&gpio1 3 GPIO_ACTIVE_LOW>;
};
```

Driver example:

```c
demo->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
if (IS_ERR(demo->reset_gpio))
        return dev_err_probe(dev, PTR_ERR(demo->reset_gpio),
                             "failed to get reset gpio\n");
```

Inspection:

```bash
cat /sys/kernel/debug/gpio
```

Success evidence:

- GPIO appears requested with expected label
- active-low behavior is handled by descriptor API

Common failures:

- wrong property name
- GPIO controller not probed
- pinmux not configured
- manually inverting active-low logic

## Lab 7: I2C Or SPI Client

Goal:

- bind to a real or dummy bus client and perform one transaction

Concepts:

- I2C client or SPI device
- bus address or chip select
- Device Tree child node
- register read
- sleepable bus transfer
- regmap optional

I2C inspection:

```bash
ls /sys/bus/i2c/devices/
cat /sys/bus/i2c/devices/1-0048/modalias
```

SPI inspection:

```bash
ls /sys/bus/spi/devices/
cat /sys/bus/spi/devices/spi0.0/modalias
```

Success evidence:

- client device exists
- driver probes
- one read transaction succeeds

Common failures:

- wrong I2C address
- wrong SPI mode
- missing pinmux
- hardware held in reset
- bus transfer attempted in hard IRQ context

## Lab 8: IRQ And Threaded IRQ

Goal:

- respond to a hardware or simulated interrupt

Concepts:

- IRQ resource
- trigger type
- hard IRQ handler
- threaded IRQ handler
- `/proc/interrupts`
- wakeups

Driver pattern:

```c
ret = devm_request_threaded_irq(dev, irq,
                                demo_irq,
                                demo_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(dev),
                                demo);
```

Inspection:

```bash
cat /proc/interrupts | grep demo
```

Success evidence:

- interrupt count increases when event occurs
- hard handler does minimal work
- threaded handler performs sleepable work

Common failures:

- wrong trigger type
- line stuck active
- interrupt not acknowledged
- handler returns wrong `irqreturn_t`

## Lab 9: Basic Tracing

Goal:

- prove callback ordering and timing without adding logs everywhere

Concepts:

- ftrace
- tracepoints
- trace events
- filtering
- short capture windows

Example:

```bash
sudo trace-cmd record -e irq -e workqueue sleep 5
sudo trace-cmd report
```

Manual tracing:

```bash
cd /sys/kernel/tracing
echo function | sudo tee current_tracer
echo demo_* | sudo tee set_ftrace_filter
echo 1 | sudo tee tracing_on
# run test
echo 0 | sudo tee tracing_on
sudo cat trace
```

Success evidence:

- trace shows intended callback path
- trace window is small enough to review

Common failures:

- tracing too broadly
- forgetting to disable tracing
- missing debugfs/tracing filesystem mount

## Lab Review Checklist

For each lab, record:

- kernel version
- source commit
- module artifact path
- Device Tree artifact if relevant
- exact build command
- exact load command
- expected evidence
- actual evidence
- first failure line
- next change

## Common Mistakes

- Starting with real hardware before dummy software paths work.
- Adding IRQ, GPIO, and I2C support in one step.
- Not keeping a known-good checkpoint.
- Forgetting to unload old modules.
- Testing with stale DTB or stale module.
- Ignoring runtime inspection paths.

## Related Topics

- [Kernel Module Lifecycle](../fundamentals/kernel-module-lifecycle.md)
- [Character Device Basics](../fundamentals/character-device-basics.md)
- [Platform Devices And Platform Drivers](../fundamentals/platform-devices-and-drivers.md)
- [GPIO Consumer API](../driver-interfaces/gpio-consumer-api.md)
- [I2C Client Drivers](../driver-interfaces/i2c-client-drivers.md)
- [SPI Device Drivers](../driver-interfaces/spi-device-drivers.md)
- [Threaded Interrupts](../driver-interfaces/threaded-interrupts.md)

## References

- Building external modules: <https://docs.kernel.org/kbuild/modules.html>
- Driver API: <https://docs.kernel.org/driver-api/index.html>
- ftrace: <https://docs.kernel.org/trace/ftrace.html>
