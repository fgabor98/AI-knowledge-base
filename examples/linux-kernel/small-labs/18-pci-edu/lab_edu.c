// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for QEMU's "edu" teaching device (PCI 1234:11e8).
 * Register description: docs/specs/edu.rst in the QEMU source.
 */
#include <linux/bits.h>
#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/random.h>

#define EDU_ID			0x00
#define EDU_LIVENESS		0x04
#define EDU_FACTORIAL		0x08
#define EDU_STATUS		0x20
#define   EDU_STATUS_IRQ_FACT	BIT(7)
#define EDU_IRQ_STATUS		0x24
#define EDU_IRQ_ACK		0x64
#define EDU_DMA_SRC		0x80
#define EDU_DMA_DST		0x88
#define EDU_DMA_COUNT		0x90
#define EDU_DMA_CMD		0x98
#define   EDU_DMA_START		BIT(0)
#define   EDU_DMA_FROM_DEVICE	BIT(1)
#define   EDU_DMA_IRQ		BIT(2)

#define EDU_IRQ_FACT		0x001
#define EDU_IRQ_DMA		0x100

#define EDU_SRAM		0x40000	/* device-side address of its buffer */
#define EDU_DMA_SIZE		256

struct lab_edu {
	struct pci_dev *pdev;
	void __iomem *regs;
	struct completion fact_done;
	struct completion dma_done;
};

static irqreturn_t lab_edu_irq(int irq, void *data)
{
	struct lab_edu *edu = data;
	u32 status = readl(edu->regs + EDU_IRQ_STATUS);

	if (!status)
		return IRQ_NONE;

	writel(status, edu->regs + EDU_IRQ_ACK);

	if (status & EDU_IRQ_FACT)
		complete(&edu->fact_done);
	if (status & EDU_IRQ_DMA)
		complete(&edu->dma_done);
	return IRQ_HANDLED;
}

static int lab_edu_factorial(struct lab_edu *edu)
{
	u32 val;

	reinit_completion(&edu->fact_done);
	writel(EDU_STATUS_IRQ_FACT, edu->regs + EDU_STATUS);
	writel(10, edu->regs + EDU_FACTORIAL);

	if (!wait_for_completion_timeout(&edu->fact_done, HZ))
		return -ETIMEDOUT;

	val = readl(edu->regs + EDU_FACTORIAL);
	dev_info(&edu->pdev->dev, "10! = %u\n", val);
	return val == 3628800 ? 0 : -EIO;
}

static int lab_edu_dma(struct lab_edu *edu, dma_addr_t addr, bool to_device)
{
	u32 cmd = EDU_DMA_START | EDU_DMA_IRQ;

	if (!to_device)
		cmd |= EDU_DMA_FROM_DEVICE;

	reinit_completion(&edu->dma_done);
	/* Registers from 0x80 up accept 64-bit accesses. */
	writeq(to_device ? addr : EDU_SRAM, edu->regs + EDU_DMA_SRC);
	writeq(to_device ? EDU_SRAM : addr, edu->regs + EDU_DMA_DST);
	writeq(EDU_DMA_SIZE, edu->regs + EDU_DMA_COUNT);
	/* writeq() orders the buffer contents before this doorbell. */
	writeq(cmd, edu->regs + EDU_DMA_CMD);

	if (!wait_for_completion_timeout(&edu->dma_done, HZ)) {
		/* Stop the device from writing to memory we are about to free. */
		pci_clear_master(edu->pdev);
		return -ETIMEDOUT;
	}
	return 0;
}

/* Send a pattern to the device and read it back: both DMA directions. */
static int lab_edu_dma_roundtrip(struct lab_edu *edu)
{
	struct device *dev = &edu->pdev->dev;
	dma_addr_t tx_dma, rx_dma;
	u8 *tx, *rx;
	int ret;

	tx = kmalloc(EDU_DMA_SIZE, GFP_KERNEL);
	rx = kzalloc(EDU_DMA_SIZE, GFP_KERNEL);
	if (!tx || !rx) {
		ret = -ENOMEM;
		goto out_free;
	}
	get_random_bytes(tx, EDU_DMA_SIZE);

	tx_dma = dma_map_single(dev, tx, EDU_DMA_SIZE, DMA_TO_DEVICE);
	ret = dma_mapping_error(dev, tx_dma);
	if (ret)
		goto out_free;

	rx_dma = dma_map_single(dev, rx, EDU_DMA_SIZE, DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, rx_dma);
	if (ret)
		goto out_unmap_tx;

	ret = lab_edu_dma(edu, tx_dma, true);
	if (!ret)
		ret = lab_edu_dma(edu, rx_dma, false);

	/* The CPU may look at rx only after this unmap. */
	dma_unmap_single(dev, rx_dma, EDU_DMA_SIZE, DMA_FROM_DEVICE);
out_unmap_tx:
	dma_unmap_single(dev, tx_dma, EDU_DMA_SIZE, DMA_TO_DEVICE);

	if (!ret && memcmp(tx, rx, EDU_DMA_SIZE))
		ret = -EIO;
	dev_info(dev, "DMA round trip: %s\n", ret ? "failed" : "ok");
out_free:
	kfree(rx);
	kfree(tx);
	return ret;
}

static void lab_edu_free_vectors(void *data)
{
	pci_free_irq_vectors(data);
}

static int lab_edu_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device *dev = &pdev->dev;
	struct lab_edu *edu;
	int ret;

	edu = devm_kzalloc(dev, sizeof(*edu), GFP_KERNEL);
	if (!edu)
		return -ENOMEM;
	edu->pdev = pdev;
	init_completion(&edu->fact_done);
	init_completion(&edu->dma_done);

	ret = pcim_enable_device(pdev);
	if (ret)
		return dev_err_probe(dev, ret, "cannot enable device\n");

	edu->regs = pcim_iomap_region(pdev, 0, KBUILD_MODNAME);
	if (IS_ERR(edu->regs))
		return dev_err_probe(dev, PTR_ERR(edu->regs), "cannot map BAR 0\n");

	/* The device emits 28 address bits by default. */
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(28));
	if (ret)
		return dev_err_probe(dev, ret, "no usable DMA mask\n");
	pci_set_master(pdev);

	dev_info(dev, "id %#x\n", readl(edu->regs + EDU_ID));
	writel(0x12345678, edu->regs + EDU_LIVENESS);
	if (readl(edu->regs + EDU_LIVENESS) != (u32)~0x12345678)
		return dev_err_probe(dev, -EIO, "liveness check failed\n");

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_INTX);
	if (ret < 0)
		return dev_err_probe(dev, ret, "no interrupt vector\n");
	ret = devm_add_action_or_reset(dev, lab_edu_free_vectors, pdev);
	if (ret)
		return ret;

	ret = devm_request_irq(dev, pci_irq_vector(pdev, 0), lab_edu_irq,
			       IRQF_SHARED, KBUILD_MODNAME, edu);
	if (ret)
		return dev_err_probe(dev, ret, "cannot request IRQ\n");

	pci_set_drvdata(pdev, edu);

	ret = lab_edu_factorial(edu);
	if (ret)
		return dev_err_probe(dev, ret, "factorial failed\n");
	return lab_edu_dma_roundtrip(edu);
}

static void lab_edu_remove(struct pci_dev *pdev)
{
	struct lab_edu *edu = pci_get_drvdata(pdev);

	/* Quiesce the device; devm then frees the IRQ, vectors, and BAR. */
	writel(0, edu->regs + EDU_STATUS);
	pci_clear_master(pdev);
}

static const struct pci_device_id lab_edu_ids[] = {
	{ PCI_DEVICE(0x1234, 0x11e8) },
	{ }
};
MODULE_DEVICE_TABLE(pci, lab_edu_ids);

static struct pci_driver lab_edu_driver = {
	.name = "lab_edu",
	.id_table = lab_edu_ids,
	.probe = lab_edu_probe,
	.remove = lab_edu_remove,
};
module_pci_driver(lab_edu_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 18: PCI driver for QEMU's edu device");
