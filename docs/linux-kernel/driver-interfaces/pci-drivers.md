---
status: draft
reviewed: false
domain: linux-kernel
difficulty: intermediate
last_reviewed: null
---

# PCI And PCIe Device Drivers

## What Problem Does This Solve?

PCI devices describe themselves: the kernel discovers them by enumeration instead of reading a Device Tree node. NVMe drives, Wi-Fi modules, Ethernet controllers, and FPGA cards on embedded boards all appear this way. On TI K3 devices such as AM64x, a PCIe root complex is itself a platform device, and everything behind it is enumerated.

This page covers the endpoint driver: matching, BARs, interrupts, DMA, and removal.

## Core Concepts

- vendor, device, subsystem, and class IDs
- configuration space and BARs
- `struct pci_driver`, `struct pci_dev`
- `pcim_enable_device()`, `pcim_iomap_region()`
- bus mastering and DMA mask
- INTx, MSI, MSI-X
- surprise removal and error recovery
- Device Tree host bridge versus enumerated endpoint

## Mental Model

```text
Device Tree: PCIe host bridge (platform driver)
-> link training
-> enumeration reads configuration space, assigns BARs
-> struct pci_dev per function
-> match by ID table
-> probe(): enable, map BARs, DMA mask, interrupts, register with a subsystem
```

The address of a register block is assigned at enumeration. Nothing in the driver is hard-coded except offsets inside a BAR.

| Aspect | Platform device | PCI device |
| --- | --- | --- |
| discovery | Device Tree | enumeration |
| identity | `compatible` | vendor and device ID |
| registers | `reg` | BARs |
| interrupt | `interrupts` property | MSI, MSI-X, or INTx |
| power and reset | clocks, regulators, resets | PCI power states, managed by the core |
| removal | rare | hotplug and surprise removal possible |

## Skeleton

```c
static const struct pci_device_id demo_ids[] = {
    { PCI_DEVICE(0x1234, 0x11e8) },
    { }
};
MODULE_DEVICE_TABLE(pci, demo_ids);

static int demo_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct device *dev = &pdev->dev;
    struct demo_priv *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    ret = pcim_enable_device(pdev);
    if (ret)
        return dev_err_probe(dev, ret, "cannot enable device\n");

    priv->regs = pcim_iomap_region(pdev, 0, KBUILD_MODNAME);
    if (IS_ERR(priv->regs))
        return dev_err_probe(dev, PTR_ERR(priv->regs), "cannot map BAR 0\n");

    ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
    if (ret)
        return dev_err_probe(dev, ret, "no usable DMA mask\n");

    pci_set_master(pdev);
    pci_set_drvdata(pdev, priv);

    return demo_setup_irq_and_register(priv);
}

static struct pci_driver demo_driver = {
    .name = "demo_pci",
    .id_table = demo_ids,
    .probe = demo_probe,
    .remove = demo_remove,
};
module_pci_driver(demo_driver);
```

`pcim_*` functions are device-managed: the device is disabled and the BAR released after `remove()` or a failed probe. `pcim_iomap_region()` exists since 6.11 and returns an error pointer; before that use `pcim_iomap_regions()` plus `pcim_iomap_table()`.

The order matters. Enable the device before touching BARs, set the DMA mask before allocating DMA memory, and enable bus mastering before the device may start DMA.

## Register Access

Use `readl()`/`writel()` and relatives on the mapped BAR, exactly as for [MMIO](../memory-and-io/mmio-and-register-access.md). Two PCI properties need attention:

- **Writes are posted.** `writel()` returns before the device has seen the write. If the device must have received it, for example before a reset delay, read a register back.
- **A missing device reads as all ones.** After surprise removal or a link failure, every `readl()` returns `0xffffffff`. A polling loop waiting for a bit to become set would think it succeeded. Compare with `~0U` where that matters, and use `pci_device_is_present()` in error paths.

Configuration space is accessed with `pci_read_config_word()` and `pci_write_config_word()`. Find capabilities with `pci_find_capability()`. Ordinary drivers rarely need this, because the core handles power states, MSI, and link settings.

## Interrupts

```c
ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
if (ret < 0)
    return ret;

irq = pci_irq_vector(pdev, 0);
ret = devm_request_irq(dev, irq, demo_irq, IRQF_SHARED, KBUILD_MODNAME, priv);
```

| Type | Property |
| --- | --- |
| INTx | level-triggered wire, shared: handler must check its device and return `IRQ_NONE` otherwise |
| MSI | message write, up to 32 vectors, not shared |
| MSI-X | up to 2048 independently masked vectors |

`PCI_IRQ_ALL_TYPES` prefers MSI-X, then MSI, then INTx. `IRQF_SHARED` is required for INTx and harmless for MSI. The vectors must be freed *after* the IRQ: either call `pci_free_irq_vectors()` manually after `free_irq()`, or register it with `devm_add_action_or_reset()` *before* `devm_request_irq()` so that managed cleanup runs in the right order.

An MSI is a DMA-like write, so it needs `pci_set_master()`.

## DMA

PCI uses the normal DMA API with `&pdev->dev`. See [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md). PCI-specific points:

- If `DMA_BIT_MASK(64)` fails, fall back to 32 bits only when the hardware really supports only that.
- On timeout, stop the device before unmapping or freeing buffers. `pci_clear_master()` prevents further DMA when the device does not respond to a stop command.
- An IOMMU turns a stray DMA into a logged fault instead of memory corruption. Develop with it enabled.

## Removal

```c
static void demo_remove(struct pci_dev *pdev)
{
    struct demo_priv *priv = pci_get_drvdata(pdev);

    demo_unregister(priv);      /* no new users */
    demo_stop_hw(priv);         /* device quiet, interrupts masked */
    pci_clear_master(pdev);     /* no more DMA */
}
```

`remove()` also runs for a device that has already vanished. It must not loop forever on a register that reads `0xffffffff`.

Also provide `.shutdown` or a suspend path if the device can keep doing DMA across a `kexec` or reboot.

## Power Management And Errors

The PCI core saves configuration space and changes power states around the driver's `dev_pm_ops`. The driver stops its own activity in `suspend()` and reprograms its BAR registers in `resume()`, because D3cold loses them. For runtime PM, the core holds a usage reference that the driver releases with `pm_runtime_put_noidle()` at the end of probe and takes back in `remove()`.

`struct pci_error_handlers` lets a driver take part in recovery after an AER error. Without it, the core resets or disconnects the device.

## Inspecting

```sh
lspci -nnk
lspci -vvv -s 01:00.0
cat /sys/bus/pci/devices/0000:01:00.0/modalias
ls -l /sys/bus/pci/devices/0000:01:00.0/driver
grep -H . /sys/bus/pci/devices/0000:01:00.0/{vendor,device,class,resource,current_link_speed,current_link_width}
grep demo_pci /proc/interrupts
```

Bind a driver to an ID that is not in its table, for testing:

```sh
echo 1234 11e8 | sudo tee /sys/bus/pci/drivers/demo_pci/new_id
```

Rescan after the device comes up late, as with an FPGA:

```sh
echo 1 | sudo tee /sys/bus/pci/devices/0000:01:00.0/remove
echo 1 | sudo tee /sys/bus/pci/rescan
```

## Bring-Up On An Embedded Board

| Observation | Look at |
| --- | --- |
| no host bridge in `lspci` | host driver probe, PHY, clocks, Device Tree |
| host bridge but no endpoint | link training: reference clock, PERST# GPIO and timing, lanes, power |
| endpoint but a BAR is unassigned | `ranges` of the host bridge, window size, `dmesg` |
| registers fine but no interrupts | MSI controller, `msi-map`, bus mastering |
| DMA faults | `dma-ranges`, IOMMU, DMA mask |
| works cold, fails after reboot | PERST# and power sequencing |

## Runnable Example

`examples/linux-kernel/small-labs/18-pci-edu` drives QEMU's `edu` device with MSI and DMA in both directions. No hardware is needed.

## Common Mistakes

- Trusting a register value without considering `0xffffffff`.
- Freeing MSI vectors before the IRQ.
- Forgetting `pci_set_master()`, so DMA and MSI do nothing.
- Returning `IRQ_HANDLED` for a shared INTx line without checking the device.
- Freeing buffers while the device may still write to them.
- Assuming a write has reached the device when `writel()` returns.

## Practice Exercises

1. Run Lab 18 and perform its three experiments.
2. Trace the probe of a real PCI driver and list the managed resources it acquires.
3. Compare `pci_alloc_irq_vectors()` use in an Ethernet driver with that in Lab 18.

## Debugging Checklist

- Is the device enumerated, with every BAR assigned?
- Which driver is bound, and does `modalias` match an ID table?
- Is the link at the expected speed and width?
- Is bus mastering on (`lspci -vv`: `BusMaster+`)?
- MSI enabled (`MSI: Enable+`) and counting in `/proc/interrupts`?
- Any AER or IOMMU messages?

## Related Topics

- [MMIO And Register Access](../memory-and-io/mmio-and-register-access.md)
- [DMA Mapping Basics](../memory-and-io/dma-mapping-basics.md)
- [IRQ Handling](irq-handling.md)
- [Network Device Drivers](network-device-drivers.md)

## Official References

- [How to write Linux PCI drivers](https://docs.kernel.org/PCI/pci.html)
- [The MSI driver guide](https://docs.kernel.org/PCI/msi-howto.html)
- [PCI power management](https://docs.kernel.org/power/pci.html)
- [PCI error recovery](https://docs.kernel.org/PCI/pci-error-recovery.html)
