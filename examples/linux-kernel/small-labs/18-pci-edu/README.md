# Lab 18: PCI Device With MSI And DMA

This checkpoint drives QEMU's `edu` teaching device: enumeration and matching,
a BAR mapping, an MSI interrupt with a completion, and streaming DMA in both
directions. It needs a QEMU guest (x86-64) started with `-device edu`, and
kernel 6.11 or later for `pcim_iomap_region()`.

```sh
make KDIR=/path/to/guest-kernel-build
# in the guest
lspci -nn | grep 1234:11e8 || grep . /sys/bus/pci/devices/*/vendor
insmod ./lab_edu.ko
dmesg | grep lab_edu
grep lab_edu /proc/interrupts
echo 0000:00:04.0 > /sys/bus/pci/drivers/lab_edu/unbind   # use your address
rmmod lab_edu
```

Expected evidence: `id 0x10000ed`, `10! = 3628800`, and
`DMA round trip: ok`.

Experiments:

- Remove `pci_set_master()`. Probe fails with `factorial failed` and
  `-ETIMEDOUT` before any DMA: an MSI is itself a memory write by the device,
  so it never arrives.
- Boot with `CONFIG_DMA_API_DEBUG=y` and remove one `dma_unmap_single()`. Read
  the warning at unbind.
- Print `pci_dev_msi_enabled(pdev)`, then boot with `pci=nomsi` and compare.
