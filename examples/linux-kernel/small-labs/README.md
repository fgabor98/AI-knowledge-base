# Small Driver Lab Checkpoints

This directory contains the runnable artifacts for
[Small Lab Progression](../../../docs/linux-kernel/foundations/small-lab-progression.md).
The examples deliberately stay small and use a software-only `demo` identity
where hardware is not required.

## Checkpoints

| Lab | Checkpoint | Main artifact |
| --- | --- | --- |
| 1 | Hello module | `hello.ko` |
| 2 | Module parameters | `lab_params.ko` |
| 3 | Dummy character device | `lab_char.ko` |
| 4 | Software platform device | `platform_demo.ko` and `platform_demo_device.ko` |
| 5 | Device Tree matching | `dt_demo.ko` and `demo-overlay.dtbo` |
| 6 | GPIO consumer | `gpio_demo.ko` and `gpio-demo-overlay.dtbo` |
| 7 | I2C client | `lab_i2c.ko` and `lab_i2c_client.ko` |
| 8 | Threaded virtual IRQ | `lab_irq.ko` |
| 9 | Basic tracing | `trace-demo.sh` |
| 10 | ioctl ABI | `lab_ioctl.ko` and `test_ioctl` |
| 11 | Blocking I/O and poll | `lab_poll.ko` and `test_poll` |
| 12 | seq_file and debugfs | `lab_seq.ko` |
| 13 | Deferred contexts | `lab_defer.ko` |
| 14 | RCU | `lab_rcu.ko` |
| 15 | Virtual input device | `lab_input.ko` |
| 16 | Exported symbols | `lab_provider.ko` and `lab_consumer.ko` |
| 17 | kprobe events | `kprobe-demo.sh` |
| 18 | PCI, MSI, and DMA | `lab_edu.ko` |
| 19 | Network device | `lab_net.ko` |

Each directory has its own `Makefile` and `README.md`. The modules are
intentionally separate checkpoints rather than one large driver. Lab 4 and
Lab 7 need a helper module to create a software device. Lab 5 and Lab 6 need a
Device Tree node on a target that supports overlays. Lab 9 observes the Lab 8
module and therefore does not add another kernel module. Lab 17 instruments
Lab 3. Lab 18 needs QEMU's `edu` device.

Labs 1 to 4, 7, 8, and 10 to 19 were run on Linux 6.18.65 under QEMU with
KASAN, lockdep, atomic-sleep checks, DMA API debugging, and kmemleak without
reports, and all modules build against 7.0. Run new experiments in
[that lab environment](../qemu-lab), not on a workstation.

## Common build requirement

Install headers and build tools for the running kernel, then build from the
individual checkpoint directory:

```sh
sudo apt install build-essential device-tree-compiler trace-cmd i2c-tools linux-headers-$(uname -r)
make
```

The `KDIR` variable can point at another prepared kernel tree or headers
directory:

```sh
make KDIR=/path/to/linux-build
```

Do not load a module built for a different kernel release or configuration.
