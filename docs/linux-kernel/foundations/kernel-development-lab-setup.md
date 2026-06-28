---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel Development Lab Setup

## What Problem Does This Solve?

Kernel experiments can crash, hang, or make a system unbootable. A proper lab lets you learn quickly while keeping a recovery path. This is not optional for driver work: even a simple module can dereference a bad pointer, deadlock, flood logs, or wedge hardware.

This page describes safe beginner lab setups and the minimum checks needed before loading custom kernel code.

## Core Concepts

- VM
- QEMU
- spare embedded board
- serial console
- recovery kernel
- known-good boot path
- matching source and config
- module build tree
- snapshots
- log capture
- rootfs staging
- module signing policy

## Lab Options

### Option 1: VM

A VM is the easiest place to learn module mechanics and debugging tools.

Use it for:

- hello modules
- character devices
- sysfs/debugfs experiments
- workqueue/timer/waitqueue examples
- simple tracing
- crash recovery practice

Limitations:

- no real embedded buses unless emulated
- limited hardware interrupt work
- Device Tree may not be representative on x86

Good safety properties:

- snapshots
- easy reboot
- no product hardware risk

### Option 2: QEMU Kernel Lab

QEMU is useful when you want full control over kernel image, command line, root filesystem, and sometimes emulated devices.

Use it for:

- booting custom kernels
- testing initramfs
- early console work
- crash and panic experiments
- ARM or RISC-V architecture practice

Example conceptual boot command:

```bash
qemu-system-x86_64 \
  -kernel arch/x86/boot/bzImage \
  -append "console=ttyS0 root=/dev/sda rw" \
  -drive file=rootfs.img,format=raw \
  -nographic
```

The exact command depends on architecture and root filesystem layout.

### Option 3: Spare Embedded Board

A spare board is the right target when you need real GPIO, I2C, SPI, IRQs, regulators, clocks, pinctrl, and Device Tree behavior.

Use it for:

- Device Tree matched platform drivers
- GPIO/I2C/SPI clients
- pinmux debugging
- interrupt debugging
- power sequencing
- suspend/resume

Minimum safety requirements:

- serial console
- known-good boot media
- recovery image or fallback boot entry
- ability to power-cycle the board
- access to deployed kernel image, DTB, modules, and firmware

### Option 4: Actual Product Hardware

Use actual target hardware only when the lab flow is already proven.

Before loading experimental code:

- capture a full boot log
- confirm recovery procedure
- confirm watchdog behavior
- know how to restore boot media
- document the original kernel, DTB, and module versions

## Minimum Lab Checklist

Before loading custom code, confirm:

- You know which kernel is running:

```bash
uname -a
uname -r
```

- You know whether kernel config is available:

```bash
zcat /proc/config.gz | head
```

If `/proc/config.gz` is unavailable, try:

```bash
ls /boot/config-$(uname -r)
```

- You know where modules are installed:

```bash
ls /lib/modules/$(uname -r)
```

- You have build metadata:

```bash
ls /lib/modules/$(uname -r)/build
```

- You can capture kernel logs:

```bash
dmesg --follow
journalctl -k -f
```

- You can recover if the system hangs.

## Matching Source, Config, And Modules

External modules must be built against the correct kernel build tree.

Check vermagic:

```bash
modinfo ./demo.ko | grep vermagic
uname -r
```

If vermagic does not match, module loading may fail:

```text
invalid module format
```

Common causes:

- built against wrong kernel headers
- different compiler flags
- different `CONFIG_MODVERSIONS`
- stale build directory
- module copied from another board

## Minimal External Module Lab

Create a tiny module only in a safe lab:

```c
#include <linux/init.h>
#include <linux/module.h>

static int __init demo_init(void)
{
        pr_info("demo: loaded\n");
        return 0;
}

static void __exit demo_exit(void)
{
        pr_info("demo: unloaded\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal kernel lab module");
```

Example external module Makefile:

```make
obj-m += demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Build:

```bash
make
```

Load:

```bash
sudo insmod demo.ko
dmesg | tail -20
```

Inspect:

```bash
lsmod | grep demo
modinfo demo.ko
cat /sys/module/demo/refcnt
```

Unload:

```bash
sudo rmmod demo
dmesg | tail -20
```

## Serial Console

For embedded work, serial console is often the most important debugging tool.

It captures:

- bootloader logs
- early kernel logs
- kernel panic output
- login shell before networking works
- watchdog-reset evidence if persistent logs are absent

Typical connection:

```bash
sudo picocom -b 115200 /dev/ttyUSB0
```

or:

```bash
sudo minicom -D /dev/ttyUSB0 -b 115200
```

Keep logs:

```bash
script -f boot.log
sudo picocom -b 115200 /dev/ttyUSB0
```

## Kernel Command Line For Labs

Useful lab options vary by platform, but common ones include:

```text
console=ttyS0,115200
earlycon
loglevel=8
ignore_loglevel
panic=10
rootwait
```

Use production command-line policy separately. Lab options are for visibility and recovery, not necessarily shipping defaults.

## Recovery Planning

A recovery plan should answer:

- How do I boot a known-good kernel?
- How do I restore a known-good DTB?
- How do I remove a broken module from the root filesystem?
- How do I interrupt bootloader autoboot?
- How do I power-cycle the board?
- How do I capture logs if networking is down?

Example boot partition safety layout:

```text
/boot/Image-good
/boot/Image-test
/boot/board-good.dtb
/boot/board-test.dtb
/boot/extlinux/extlinux.conf
```

Use bootloader entries so you can choose the known-good path.

## Snapshot And Artifact Discipline

Record the artifact identity for every test:

```bash
uname -a
cat /proc/cmdline
sha256sum /boot/Image /boot/*.dtb 2>/dev/null
modinfo ./demo.ko
git rev-parse HEAD
```

This prevents "I tested a different kernel than I edited" failures.

## Recommended Directory Layout

For a personal lab:

```text
kernel-lab/
  linux/                  # kernel source
  build/                  # O= output directory
  modules/
    hello/
    char-demo/
    platform-demo/
  rootfs/
  logs/
  notes/
```

Keep logs and notes near the artifacts:

```text
logs/
  2026-06-27-hello-module-load.log
  2026-06-27-platform-probe-failure.log
notes/
  board-boot-settings.md
  recovery-procedure.md
```

## Common Mistakes

- Loading experimental modules on a non-recoverable system.
- Building against headers that do not match the running kernel.
- Relying only on SSH for a board that may lose networking.
- Not saving the original DTB before testing Device Tree changes.
- Making bootloader, kernel, DTB, rootfs, and module changes simultaneously.
- Testing without a known-good boot path.
- Forgetting module signing or secure boot policy.

## Debugging Checklist

- Does `uname -r` match the build directory?
- Does `modinfo` vermagic match the running kernel?
- Can you see kernel logs live?
- Can you reboot or power-cycle if the system hangs?
- Can you boot a known-good image?
- Can you remove a broken module from the target filesystem?
- Are test artifacts named so they cannot be confused with production artifacts?

## Related Topics

- [Kernel Build And Install Overview](../source-build-and-tailoring/kernel-build-and-install-overview.md)
- [Kernel Module Lifecycle](../fundamentals/kernel-module-lifecycle.md)
- [Driver Development Workflow](driver-development-workflow.md)
- [Dmesg And Log Levels](../debugging/dmesg-and-log-levels.md)

## References

- Linux kernel module documentation: <https://docs.kernel.org/kbuild/modules.html>
- Linux kernel admin guide: <https://docs.kernel.org/admin-guide/index.html>
- Linux kernel command-line parameters: <https://docs.kernel.org/admin-guide/kernel-parameters.html>
