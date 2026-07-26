// SPDX-License-Identifier: GPL-2.0
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/numa.h>
#include <linux/timer.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
#define lab_timer_delete_sync timer_delete_sync
#else
#define lab_timer_delete_sync del_timer_sync
#endif

static unsigned int period_ms = 1000;
module_param(period_ms, uint, 0444);
MODULE_PARM_DESC(period_ms, "Interval between simulated interrupts in milliseconds");

static int virtual_irq;
static struct timer_list event_timer;
static atomic_t event_count = ATOMIC_INIT(0);

static void lab_irq_noop(struct irq_data *data)
{
}

static struct irq_chip lab_irq_chip = {
	.name = "lab-virtual-irq",
	.irq_ack = lab_irq_noop,
	.irq_mask = lab_irq_noop,
	.irq_unmask = lab_irq_noop,
};

static irqreturn_t lab_irq_handler(int irq, void *data)
{
	pr_info_ratelimited("lab8_irq: hard handler ran for irq %d\n", irq);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t lab_irq_thread(int irq, void *data)
{
	unsigned int count = atomic_inc_return(&event_count);

	pr_info("lab8_irq: threaded handler completed event %u\n", count);
	return IRQ_HANDLED;
}

static void lab_irq_timer(struct timer_list *timer)
{
	generic_handle_irq(virtual_irq);
	mod_timer(&event_timer, jiffies + msecs_to_jiffies(period_ms));
}

static int __init lab_irq_init(void)
{
	int ret;

	if (period_ms < 10 || period_ms > 60000)
		return -EINVAL;

	virtual_irq = irq_alloc_descs(-1, 0, 1, NUMA_NO_NODE);
	if (virtual_irq < 0)
		return virtual_irq;

	irq_set_chip_and_handler(virtual_irq, &lab_irq_chip, handle_simple_irq);
	ret = request_threaded_irq(virtual_irq, lab_irq_handler, lab_irq_thread,
				   IRQF_ONESHOT, "lab_virtual_irq", NULL);
	if (ret)
		goto err_irq_desc;

	timer_setup(&event_timer, lab_irq_timer, 0);
	mod_timer(&event_timer, jiffies + msecs_to_jiffies(period_ms));
	pr_info("lab8_irq: registered simulated irq %d, period %u ms\n",
		virtual_irq, period_ms);
	return 0;

err_irq_desc:
	irq_set_chip_and_handler(virtual_irq, NULL, handle_bad_irq);
	irq_free_descs(virtual_irq, 1);
	return ret;
}

static void __exit lab_irq_exit(void)
{
	lab_timer_delete_sync(&event_timer);
	free_irq(virtual_irq, NULL);
	irq_set_chip_and_handler(virtual_irq, NULL, handle_bad_irq);
	irq_set_chip_data(virtual_irq, NULL);
	irq_free_descs(virtual_irq, 1);
	pr_info("lab8_irq: unloaded after %u events\n",
		atomic_read(&event_count));
}

module_init(lab_irq_init);
module_exit(lab_irq_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 8: simulated hard and threaded IRQ");
