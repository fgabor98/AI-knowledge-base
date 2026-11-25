// SPDX-License-Identifier: GPL-2.0
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irq_work.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/numa.h>
#include <linux/preempt.h>
#include <linux/sched.h>
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
	pr_info_ratelimited("lab8_irq: hard handler irq=%d hardirq=%d irqs_off=%d\n",
			    irq, !!in_hardirq(), irqs_disabled());
	return IRQ_WAKE_THREAD;
}

static irqreturn_t lab_irq_thread(int irq, void *data)
{
	unsigned int count = atomic_inc_return(&event_count);

	pr_info("lab8_irq: threaded handler event=%u hardirq=%d irqs_off=%d comm=%s\n",
		count, !!in_hardirq(), irqs_disabled(), current->comm);
	return IRQ_HANDLED;
}

/*
 * A timer callback runs in softirq context with interrupts enabled, which is
 * not a legal place to call generic_handle_irq(). irq_work gives a genuine
 * hard interrupt context (a self-IPI where the architecture has one), so the
 * hard handler below runs under the same rules as for real hardware.
 */
static void lab_irq_inject(struct irq_work *work)
{
	generic_handle_irq(virtual_irq);
}

static struct irq_work event_irq_work = IRQ_WORK_INIT_HARD(lab_irq_inject);

static void lab_irq_timer(struct timer_list *timer)
{
	irq_work_queue(&event_irq_work);
	mod_timer(&event_timer, jiffies + msecs_to_jiffies(period_ms));
}

static void lab_irq_free_desc(void)
{
	/* Remove the flow handler while the chip is still set. */
	irq_set_handler(virtual_irq, NULL);
	irq_set_chip(virtual_irq, NULL);
	irq_free_descs(virtual_irq, 1);
}

static int __init lab_irq_init(void)
{
	int ret;

	if (period_ms < 10 || period_ms > 60000)
		return -EINVAL;

	/*
	 * irq_alloc_descs() would record this module as the owner of the
	 * descriptor. request_irq() takes a reference on that owner, so the
	 * module would pin itself and could never be unloaded. Passing no owner
	 * is safe because lab_irq_exit() frees the IRQ before the descriptor.
	 */
	virtual_irq = __irq_alloc_descs(-1, 0, 1, NUMA_NO_NODE, NULL, NULL);
	if (virtual_irq < 0)
		return virtual_irq;

	irq_set_chip_and_handler(virtual_irq, &lab_irq_chip, handle_simple_irq);
	/* 32-bit ARM marks new descriptors as not requestable by default. */
	irq_clear_status_flags(virtual_irq, IRQ_NOREQUEST);
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
	lab_irq_free_desc();
	return ret;
}

static void __exit lab_irq_exit(void)
{
	/* Stop the producers in order, then remove the handler they reach. */
	lab_timer_delete_sync(&event_timer);
	irq_work_sync(&event_irq_work);
	free_irq(virtual_irq, NULL);
	lab_irq_free_desc();
	pr_info("lab8_irq: unloaded after %u events\n",
		atomic_read(&event_count));
}

module_init(lab_irq_init);
module_exit(lab_irq_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 8: simulated hard and threaded IRQ");
