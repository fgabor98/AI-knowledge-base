// SPDX-License-Identifier: GPL-2.0
#include <linux/bottom_half.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0)
#define lab_timer_delete_sync timer_delete_sync
#else
#define lab_timer_delete_sync del_timer_sync
#endif

static unsigned int period_ms = 2000;
module_param(period_ms, uint, 0444);
MODULE_PARM_DESC(period_ms, "Interval between rounds in milliseconds");

static struct timer_list lab_timer;
static struct workqueue_struct *lab_wq;
static struct task_struct *lab_task;
static DECLARE_WAIT_QUEUE_HEAD(lab_waitq);
static bool lab_kick;

static void lab_report(const char *what)
{
	pr_info("lab13_defer: %-9s softirq=%d task=%d irqs_off=%d comm=%s\n",
		what, !!in_serving_softirq(), !!in_task(), irqs_disabled(),
		in_task() ? current->comm : "(whoever was interrupted)");
}

/* Sleepable: runs in a kworker. */
static void lab_work_fn(struct work_struct *work)
{
	might_sleep();
	lab_report("work");
}
static DECLARE_WORK(lab_work, lab_work_fn);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
/* Atomic: BH work runs in softirq context and replaces tasklets. */
static void lab_bh_work_fn(struct work_struct *work)
{
	lab_report("bh-work");
}
static DECLARE_WORK(lab_bh_work, lab_bh_work_fn);
#endif

/* Sleepable: a dedicated thread with its own loop and state. */
static int lab_thread_fn(void *data)
{
	while (!kthread_should_stop()) {
		wait_event_interruptible(lab_waitq,
					 READ_ONCE(lab_kick) ||
					 kthread_should_stop());
		if (xchg(&lab_kick, false))
			lab_report("kthread");
	}
	return 0;
}

static void lab_timer_fn(struct timer_list *t)
{
	lab_report("timer");

	queue_work(lab_wq, &lab_work);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
	queue_work(system_bh_wq, &lab_bh_work);
#endif
	WRITE_ONCE(lab_kick, true);
	wake_up_interruptible(&lab_waitq);

	mod_timer(&lab_timer, jiffies + msecs_to_jiffies(period_ms));
}

static int __init lab_init(void)
{
	if (period_ms < 10 || period_ms > 60000)
		return -EINVAL;

	lab_wq = alloc_workqueue("lab_defer", WQ_UNBOUND, 0);
	if (!lab_wq)
		return -ENOMEM;

	lab_task = kthread_run(lab_thread_fn, NULL, "lab_defer");
	if (IS_ERR(lab_task)) {
		destroy_workqueue(lab_wq);
		return PTR_ERR(lab_task);
	}

	timer_setup(&lab_timer, lab_timer_fn, 0);
	mod_timer(&lab_timer, jiffies + msecs_to_jiffies(period_ms));
	return 0;
}

static void __exit lab_exit(void)
{
	/* First the producer, then everything it can trigger. */
	lab_timer_delete_sync(&lab_timer);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
	cancel_work_sync(&lab_bh_work);
#endif
	kthread_stop(lab_task);
	destroy_workqueue(lab_wq);	/* drains pending work */
}

module_init(lab_init);
module_exit(lab_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 13: where deferred code runs");
