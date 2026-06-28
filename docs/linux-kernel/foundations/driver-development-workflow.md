---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Driver Development Workflow

## What Problem Does This Solve?

Driver development is an evidence-driven loop. Beginners often lose time because they edit one source tree, build another, load an old module, test with stale Device Tree data, or debug multiple changes at once.

This page defines a repeatable workflow for small, safe iterations.

## Core Concepts

- edit
- build
- deploy
- load
- bind
- inspect
- test
- unload
- reboot
- clean logs
- minimal reproduction
- artifact identity
- known-good checkpoint

## The Basic Loop

Use this loop for module-based development:

```text
edit one small behavior
-> build exact module
-> copy or install exact module
-> load or trigger binding
-> inspect dmesg and sysfs
-> run one test
-> unload or reboot
-> record result
```

The loop should answer one question per iteration.

Bad iteration:

```text
changed driver source
changed Device Tree
changed kernel config
changed userspace test app
changed boot arguments
now the device does not work
```

Good iteration:

```text
added one dev_info() in probe
rebuilt module
loaded module
confirmed exact log appears
```

## Step 1: Decide The Question

Examples of good iteration questions:

- Did the new module get loaded?
- Did the device match the driver?
- Did `probe` run?
- Did the driver read the expected Device Tree property?
- Did the IRQ fire?
- Did the file operation run?
- Did the I2C transaction fail?

Write the question before changing code.

Example note:

```text
Question: Does the platform driver's probe run after adding compatible "example,demo"?
Expected evidence: dmesg contains "demo: probe"; sysfs device has driver symlink.
```

## Step 2: Build The Exact Artifact

External module:

```bash
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Out-of-tree kernel build directory:

```bash
make -C /path/to/linux O=/path/to/build M=$PWD modules
```

Cross-build example:

```bash
make -C /path/to/linux \
  O=/path/to/build \
  ARCH=arm64 \
  CROSS_COMPILE=aarch64-linux-gnu- \
  M=$PWD modules
```

Check the artifact:

```bash
modinfo ./demo.ko
modinfo ./demo.ko | grep -E 'filename|vermagic|depends|alias'
```

## Step 3: Deploy Without Ambiguity

For local VM tests:

```bash
sudo insmod ./demo.ko
```

For target boards:

```bash
scp demo.ko root@target:/tmp/demo.ko
ssh root@target 'insmod /tmp/demo.ko'
```

Avoid copying into `/lib/modules` while still experimenting unless you also run the correct dependency update:

```bash
sudo cp demo.ko /lib/modules/$(uname -r)/extra/
sudo depmod -a
sudo modprobe demo
```

If a module can autoload, verify the alias:

```bash
modinfo demo.ko | grep alias
```

## Step 4: Clear Or Timestamp Logs

You usually cannot clear the kernel ring buffer on production-like systems, but in a lab you can use:

```bash
sudo dmesg -C
```

Or preserve timestamps:

```bash
dmesg --time-format=iso --follow
```

For systemd systems:

```bash
journalctl -k -f
```

Good practice:

```bash
echo "=== loading demo $(date --iso-8601=seconds) ===" | sudo tee /dev/kmsg
sudo insmod ./demo.ko
```

This marker makes logs easier to review.

## Step 5: Load, Bind, Or Trigger Probe

Loading a module and probing a device are different events.

Load module:

```bash
sudo insmod demo.ko
lsmod | grep demo
```

Check module metadata:

```bash
cat /sys/module/demo/refcnt
find /sys/module/demo -maxdepth 2 -type f | sort
```

But a loaded driver may not have bound to hardware.

Check device binding:

```bash
find /sys/bus/platform/drivers -maxdepth 2 -name '*demo*' -print
find /sys/bus/platform/devices -maxdepth 1 -name '*demo*' -print
```

For a specific platform device:

```bash
readlink /sys/bus/platform/devices/48000000.demo/driver
cat /sys/bus/platform/devices/48000000.demo/modalias
```

Manual bind/unbind can be useful in a lab:

```bash
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/bind
echo 48000000.demo | sudo tee /sys/bus/platform/drivers/demo/unbind
```

Use this carefully. Some hardware does not tolerate arbitrary unbind/rebind cycles.

## Step 6: Inspect Runtime State

Common inspection commands:

```bash
dmesg | tail -100
lsmod | grep demo
modinfo ./demo.ko
find /sys/bus/platform/devices -maxdepth 1 -name '*demo*'
find /sys/class -maxdepth 3 -name '*demo*'
cat /proc/interrupts | grep -i demo
```

For character devices:

```bash
ls -l /dev/demo*
udevadm info /dev/demo0
```

For IIO:

```bash
ls /sys/bus/iio/devices/
find /sys/bus/iio/devices/iio:deviceX -maxdepth 1 -type f -print
```

For input:

```bash
ls -l /dev/input/
udevadm info /dev/input/eventX
evtest /dev/input/eventX
```

## Step 7: Test One Behavior

Examples:

Character device:

```bash
cat /dev/demo0
echo test | sudo tee /dev/demo0
```

Sysfs attribute:

```bash
cat /sys/bus/platform/devices/48000000.demo/status
echo 1 | sudo tee /sys/bus/platform/devices/48000000.demo/enable
```

IRQ:

```bash
grep demo /proc/interrupts
# trigger hardware event
grep demo /proc/interrupts
```

Trace:

```bash
sudo trace-cmd record -e irq -e workqueue sleep 5
sudo trace-cmd report
```

Keep the test narrow.

## Step 8: Unload Or Reboot Cleanly

Unload:

```bash
sudo rmmod demo
```

If unload fails:

```text
module is in use
```

Inspect:

```bash
lsmod | grep demo
cat /sys/module/demo/refcnt
lsof | grep demo
```

Common causes:

- open file descriptor
- bound device still active
- subsystem reference
- workqueue/timer/IRQ still using state

Sometimes rebooting is the correct lab reset. Do not keep stacking new experiments on a suspicious runtime state.

## Step 9: Record Result

Minimal useful note:

```text
Kernel: 6.6.32-custom
Source: abc1234
Module: demo.ko sha256=...
DTB: board-test.dtb sha256=...
Question: Did probe read reset GPIO?
Result: probe failed with -EPROBE_DEFER until gpio controller loaded.
Evidence: dmesg lines copied below.
Next: enable gpio controller built-in or include module in initramfs.
```

This is not bureaucracy. It prevents re-debugging the same issue.

## Workflow Examples

### Example: Adding A Probe Log

Change:

```c
static int demo_probe(struct platform_device *pdev)
{
        dev_info(&pdev->dev, "probe entered\n");
        return 0;
}
```

Build:

```bash
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Load:

```bash
sudo dmesg -C
sudo insmod demo.ko
dmesg | tail
```

Expected:

```text
demo 48000000.demo: probe entered
```

If not seen:

- module did not load
- no device matched
- wrong module loaded
- log level filtering

### Example: Testing Device Tree Match

Expected:

```dts
demo@48000000 {
        compatible = "example,demo";
        reg = <0x48000000 0x1000>;
};
```

Driver:

```c
static const struct of_device_id demo_of_match[] = {
        { .compatible = "example,demo" },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

Check runtime tree:

```bash
tr '\0' '\n' < /proc/device-tree/demo@48000000/compatible
```

Check module alias:

```bash
modinfo demo.ko | grep of:
```

## Common Mistakes

- Loading `demo.ko` from the wrong directory.
- Forgetting to unload before testing a rebuilt module.
- Testing with a stale DTB.
- Interpreting module load success as probe success.
- Changing multiple variables per iteration.
- Not preserving the first error line.
- Ignoring `modinfo` aliases and vermagic.
- Using `printk` everywhere instead of targeted `dev_*` logs.

## Debugging Checklist

- What exact artifact did I build?
- What exact artifact did I load?
- Does vermagic match?
- Did module load?
- Did device exist?
- Did driver bind?
- Did `probe` run?
- Did `probe` return success?
- Did userspace node or subsystem object appear?
- Did the test exercise the intended callback?
- Can I unload or restore cleanly?

## Related Topics

- [Kernel Development Lab Setup](kernel-development-lab-setup.md)
- [Debugging Ladder](debugging-ladder.md)
- [Failure Taxonomy](failure-taxonomy.md)
- [Kernel Module Lifecycle](../fundamentals/kernel-module-lifecycle.md)

## References

- Linux kernel modules build documentation: <https://docs.kernel.org/kbuild/modules.html>
- Dynamic debug HOWTO: <https://docs.kernel.org/admin-guide/dynamic-debug-howto.html>
- Driver Model binding: <https://docs.kernel.org/driver-api/driver-model/binding.html>
