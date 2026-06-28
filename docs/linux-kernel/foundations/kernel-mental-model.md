---
status: draft
reviewed: false
domain: linux-kernel
difficulty: beginner
last_reviewed: null
---

# Kernel Mental Model

## What Problem Does This Solve?

Kernel programming is hard to learn if the kernel is seen as just "a C program with special APIs." The Linux kernel is the privileged runtime that owns hardware, memory protection, scheduling, interrupts, power state, and the interfaces userspace depends on.

This page gives a beginner-level map for driver developers: where driver code sits, how userspace reaches it, what the kernel owns, and how to reason about the path between hardware and userspace.

## Core Concepts

- kernel space
- user space
- system calls
- processes and tasks
- interrupts
- hardware resources
- drivers
- subsystems
- kernel objects
- userspace ABI
- UAPI vs internal kernel API
- firmware description
- device nodes
- sysfs
- debugfs
- module vs built-in code

## Kernel Space Vs User Space

Modern Linux systems separate normal application code from privileged kernel code.

| Area | Runs what | Can directly access hardware? | Typical failure |
|---|---|---:|---|
| user space | applications, shells, daemons, libraries | no | process exits, segfaults, permission error |
| kernel space | scheduler, memory manager, filesystems, networking, drivers | yes, through controlled kernel code | oops, panic, hang, data corruption |

Userspace code runs with memory protection. If a userspace program dereferences a bad pointer, normally that process dies. If kernel code dereferences a bad pointer in the wrong path, the whole system may crash or become unstable.

Example userspace failure:

```c
int *p = NULL;
*p = 1;       /* userspace process likely receives SIGSEGV */
```

Example kernel failure:

```c
int *p = NULL;
*p = 1;       /* kernel oops or panic depending on context and config */
```

The consequence is the first kernel habit: keep experiments small, inspect every error path, and work in a recoverable lab.

## System Calls

Userspace cannot normally call kernel functions directly. It enters the kernel through controlled interfaces. The most familiar one is the system call.

Example:

```text
userspace read(fd, buf, len)
-> architecture syscall entry
-> VFS read path
-> file_operations.read or read_iter
-> driver or subsystem callback
```

If `fd` refers to a regular file, the path goes through filesystem code. If it refers to a character device, the path may reach a driver's `struct file_operations`.

Example character-device callbacks:

```c
static const struct file_operations demo_fops = {
        .owner = THIS_MODULE,
        .open = demo_open,
        .read = demo_read,
        .write = demo_write,
        .release = demo_release,
};
```

The driver does not invent a new syscall. It registers callbacks behind an existing kernel interface.

## Processes And Tasks

The kernel represents schedulable execution with `struct task_struct`. Userspace usually says "process" or "thread"; kernel code often says "task."

Why this matters for drivers:

- A file operation such as `read()` usually runs in process context on behalf of a userspace task.
- A workqueue callback runs in a kernel worker task.
- An interrupt handler is not running as a normal userspace process.
- The current task can sleep only in contexts where scheduling is legal.

Example diagnostic command:

```bash
ps -eLo pid,tid,comm,state,wchan:30
```

This can help identify userspace tasks blocked in a driver-visible path, such as waiting for input or a device event.

## Interrupts

Hardware can signal the CPU through interrupts. A driver may register an interrupt handler so hardware events become kernel events.

Conceptual path:

```text
hardware signal
-> interrupt controller
-> generic IRQ core
-> driver hard IRQ handler
-> optional threaded IRQ, workqueue, wait queue, or subsystem event
```

Hard interrupt context is constrained. It cannot sleep, should run quickly, and should defer slow work.

Example split:

```c
static irqreturn_t demo_irq(int irq, void *data)
{
        struct demo_dev *demo = data;

        demo->events++;
        return IRQ_WAKE_THREAD;
}

static irqreturn_t demo_irq_thread(int irq, void *data)
{
        struct demo_dev *demo = data;

        /* Sleepable work such as I2C/SPI transactions can live here. */
        demo_read_status_over_i2c(demo);
        return IRQ_HANDLED;
}
```

The important beginner rule is not "interrupts are fast." The rule is: know whether the code may sleep.

## Drivers

A driver is kernel code that integrates a piece of hardware or virtual hardware with kernel subsystems.

Drivers usually do some combination of:

- match a device
- acquire resources
- initialize hardware
- register with a subsystem
- handle runtime callbacks
- expose a userspace ABI indirectly through that subsystem
- clean up when the device or driver is removed

Example platform driver shape:

```c
static int demo_probe(struct platform_device *pdev)
{
        struct demo_dev *demo;

        demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
        if (!demo)
                return -ENOMEM;

        demo->base = devm_platform_ioremap_resource(pdev, 0);
        if (IS_ERR(demo->base))
                return PTR_ERR(demo->base);

        platform_set_drvdata(pdev, demo);
        return 0;
}

static void demo_remove(struct platform_device *pdev)
{
        struct demo_dev *demo = platform_get_drvdata(pdev);

        /* Stop hardware and asynchronous callbacks here. */
}
```

Even this minimal shape already shows allocation, error handling, resource acquisition, and per-device state ownership.

## Subsystems

Most real drivers should register with an existing subsystem instead of inventing custom userspace interfaces.

Examples:

| Hardware or function | Typical kernel subsystem |
|---|---|
| push button | input |
| ADC, sensor, IMU | IIO |
| GPIO-controlled reset line | GPIO consumer API |
| I2C EEPROM | nvmem or misc/char depending on purpose |
| network MAC | networking |
| serial port | TTY/serial |
| PWM backlight | backlight and PWM |
| regulator | regulator framework |

Subsystems matter because they define:

- common userspace ABI
- common power-management behavior
- shared locking and lifetime expectations
- standard Device Tree bindings
- standard debug and inspection points

Bad beginner pattern:

```text
hardware button
-> custom character device with ad hoc read format
```

Better pattern:

```text
hardware button
-> input driver
-> /dev/input/eventX
-> userspace reads standard input events
```

## Kernel Objects And Ownership

Kernel code is full of objects with explicit lifetimes:

- `struct device`
- `struct module`
- `struct file`
- `struct inode`
- `struct cdev`
- `struct platform_device`
- `struct i2c_client`
- subsystem-specific device objects

A driver often has its own private state:

```c
struct demo_dev {
        struct device *dev;
        void __iomem *base;
        struct mutex lock;
        int irq;
        bool running;
};
```

The key question is: who owns this object, and when can it be freed?

Examples:

- memory allocated with `devm_kzalloc(&pdev->dev, ...)` is tied to the device lifetime
- memory stored in `file->private_data` must remain valid while the file is open
- workqueue callbacks must not run after their backing state is freed
- IRQ handlers must be stopped before freeing state they access

## Userspace ABI

Userspace ABI means the interface userspace programs depend on. For drivers, common ABI surfaces include:

- device nodes under `/dev`
- sysfs attributes
- input events
- IIO sysfs and buffers
- network interfaces
- ioctl commands
- netlink messages
- character device read/write formats

Once userspace relies on an ABI, changing it can break applications.

Example: a sysfs attribute should normally be small, textual, and stable:

```text
/sys/bus/iio/devices/iio:device0/in_voltage0_raw
```

Avoid exposing internal debug state as product ABI. Use debugfs for debug-only state and sysfs or subsystem ABI for stable state.

## UAPI Vs Internal Kernel API

The kernel has two very different kinds of interfaces:

| Interface type | Consumer | Stability expectation |
|---|---|---|
| UAPI | userspace programs | must remain compatible |
| internal kernel API | in-kernel code | can change between kernel versions |

UAPI appears in places such as:

- `include/uapi/`
- ioctl command numbers and structs
- sysfs ABI documented under `Documentation/ABI/`
- netlink protocols
- input event codes
- IIO userspace attributes

Internal kernel APIs appear in places such as:

- `include/linux/`
- driver subsystem helper functions
- internal structs used only by the kernel
- callbacks registered with subsystem cores

Example mistake:

```c
struct demo_internal_state {
        struct mutex lock;
        void __iomem *base;
        unsigned long flags;
};
```

This must not be copied directly to userspace. It contains kernel pointers, lock state, and implementation details.

Better userspace ABI struct:

```c
struct demo_status_uapi {
        __u32 version;
        __u32 flags;
        __u64 counter;
        __u32 reserved[4];
};
```

Practical rules:

- never expose raw kernel pointers to userspace
- use fixed-width UAPI types such as `__u32` and `__u64`
- include reserved fields for future extension when designing binary structs
- document sysfs files and ioctl behavior
- treat internal kernel helper APIs as version-specific

## Reference Ownership Preview

Kernel objects often remain alive because somebody holds a reference. This is deeper than normal C pointer validity: a pointer is safe only if the object lifetime is guaranteed for the current use.

Common reference patterns:

| Pattern | Purpose |
|---|---|
| `kref` | generic reference-counted object lifetime |
| `refcount_t` | safer reference counters |
| `get_device()` / `put_device()` | hold and release a `struct device` |
| `try_module_get()` / `module_put()` | hold and release module ownership |
| `get_file()` / `fput()` | hold and release open file references |

Example:

```c
get_device(dev);
/* dev is held across this operation */
put_device(dev);
```

For most beginner drivers, the immediate practical rule is:

- do not let async callbacks outlive the object they use
- do not store pointers in userspace-visible state without a lifetime plan
- do not free driver state until files, IRQs, work, timers, and subsystem users are stopped

The detailed treatment belongs in [Reference Counting And Lifetime](../execution-and-concurrency/reference-counting-and-lifetime.md).

## Firmware Description

Many embedded devices are not discoverable. Linux learns they exist from firmware descriptions such as Device Tree or ACPI.

Example Device Tree node:

```dts
demo@48000000 {
        compatible = "example,demo";
        reg = <0x48000000 0x1000>;
        interrupts = <42>;
        reset-gpios = <&gpio1 3 GPIO_ACTIVE_LOW>;
};
```

Driver-side match table:

```c
static const struct of_device_id demo_of_match[] = {
        { .compatible = "example,demo" },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);
```

The Device Tree describes board facts. The driver should not hard-code board wiring.

## Module Vs Built-In Code

The same driver source can often be:

- built into the kernel image: `CONFIG_DEMO=y`
- built as a loadable module: `CONFIG_DEMO=m`
- not built: option unset

Why it matters:

- Built-in drivers are available during early boot.
- Modules can be loaded, unloaded, replaced, and signed separately.
- `module_exit()` does nothing for built-in code.
- Module parameters and built-in parameters have different practical workflows.

Example inspection:

```bash
zgrep CONFIG_I2C /proc/config.gz
lsmod
modinfo ./demo.ko
cat /sys/module/demo/parameters/*
```

## End-To-End Examples

### Example 1: Reading From A Character Device

```text
cat /dev/demo0
-> userspace open/read
-> VFS
-> demo_fops.open
-> demo_fops.read
-> copy_to_user()
-> bytes returned to cat
```

What to inspect:

```bash
ls -l /dev/demo0
udevadm info /dev/demo0
dmesg --follow
```

Typical failure split:

- `/dev/demo0` missing: class/device registration or udev issue
- `open` fails: file permissions, driver open callback, or device state
- `read` returns `-EFAULT`: userspace copy problem
- `read` blocks forever: wait queue or event condition problem

### Example 2: Platform Driver From Device Tree

```text
DTB contains compatible = "example,demo"
-> platform device created
-> platform bus compares compatible with driver of_match_table
-> probe runs
-> driver maps registers and requests IRQ
-> subsystem object appears in sysfs
```

What to inspect:

```bash
find /sys/bus/platform/devices -maxdepth 1 -name '*demo*' -print
cat /sys/bus/platform/devices/48000000.demo/modalias
readlink /sys/bus/platform/devices/48000000.demo/driver
dmesg | grep -i demo
```

Typical failure split:

- no platform device: Device Tree node absent, disabled, or wrong deployed DTB
- no driver bind: `compatible` mismatch or missing module alias
- probe fails: missing resource, provider not ready, hardware init failure

### Example 3: Button As Input Device

```text
GPIO interrupt
-> driver IRQ handler
-> input_report_key()
-> input_sync()
-> /dev/input/eventX
-> userspace reads input_event structs
```

What to inspect:

```bash
cat /proc/interrupts
ls -l /dev/input/
udevadm info /dev/input/eventX
evtest /dev/input/eventX
```

Typical failure split:

- no interrupt count: pinmux, IRQ specifier, interrupt controller, or wiring
- interrupt count increases but no event: driver reporting path
- event exists but app cannot read: permissions or userspace policy

## Common Mistakes

- Treating Device Tree as driver configuration rather than hardware description.
- Creating a custom character device when an existing subsystem already has the right ABI.
- Assuming `probe` means "the device works" rather than "the driver was matched and tried to initialize it."
- Debugging `/dev` before checking whether the device and driver exist in sysfs.
- Forgetting that a module can be loaded while the hardware device does not exist.
- Forgetting that a device can exist while no driver has bound to it.
- Reading only the driver source and ignoring Kconfig, Makefile, Device Tree binding, and subsystem documentation.

## Debugging Checklist

- What userspace-visible interface is expected: `/dev`, sysfs, input, IIO, network, or something else?
- Does the kernel device object exist?
- Is the driver built and registered?
- Did the device match the driver?
- Did `probe` run?
- Did `probe` succeed?
- Which resources did the driver acquire?
- Which subsystem did the driver register with?
- Which context runs the failing callback?
- What is the first failing log line or return code?

## Related Topics

- [Device Model Primer](device-model-primer.md)
- [Execution Context Primer](execution-context-primer.md)
- [Driver Development Workflow](driver-development-workflow.md)
- [Linux Device Driver Fundamentals](../fundamentals/index.md)

## References

- Linux kernel Driver API: <https://docs.kernel.org/driver-api/index.html>
- Driver Basics: <https://docs.kernel.org/driver-api/basics.html>
- Driver Model: <https://docs.kernel.org/driver-api/driver-model/index.html>
- Driver Binding: <https://docs.kernel.org/driver-api/driver-model/binding.html>
