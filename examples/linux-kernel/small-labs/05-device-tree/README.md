# Lab 5: Device Tree Matched Platform Driver

This checkpoint adds a `compatible` match table and a small Device Tree
overlay. The module has no MMIO or hardware dependency; the observable result
is that the firmware-described node becomes a platform device and reaches
`probe()`.

Build the module and overlay:

```sh
make
```

Apply `demo-overlay.dtbo` using the overlay mechanism of the target board or
bootloader, then load the driver:

```sh
sudo insmod ./dt_demo.ko
find /sys/bus/platform/devices -maxdepth 1 -name '*linux*lab*'
tr '\0' '\n' < /proc/device-tree/linux_kernel_lab_demo/compatible
dmesg | tail -n 30
```

The exact overlay-loading command is target-specific. On a board without
runtime overlay support, add the node from `demo-overlay.dts` to the board's
test Device Tree, rebuild the DTB, and boot that DTB. Check the deployed tree,
not only the source file.

Expected evidence includes the `example,linux-kernel-lab` compatible string,
a platform device, and a `Device Tree matched` probe log.

