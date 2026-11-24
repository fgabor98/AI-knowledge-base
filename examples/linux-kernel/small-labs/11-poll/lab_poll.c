// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/kfifo.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

static unsigned int period_ms = 1000;
module_param(period_ms, uint, 0444);
MODULE_PARM_DESC(period_ms, "Interval between generated samples in milliseconds");

/* lab_lock serializes the producer and all readers. */
static DEFINE_MUTEX(lab_lock);
static DECLARE_WAIT_QUEUE_HEAD(lab_readq);
static DEFINE_KFIFO(lab_fifo, u32, 16);
static u32 lab_next;
static u32 lab_dropped;

static void lab_produce(struct work_struct *work);
static DECLARE_DELAYED_WORK(lab_work, lab_produce);

static void lab_produce(struct work_struct *work)
{
	mutex_lock(&lab_lock);
	if (!kfifo_put(&lab_fifo, lab_next))
		lab_dropped++;
	lab_next++;
	mutex_unlock(&lab_lock);

	/* The condition changed first; only then wake the waiters. */
	wake_up_interruptible_poll(&lab_readq, EPOLLIN | EPOLLRDNORM);
	schedule_delayed_work(&lab_work, msecs_to_jiffies(period_ms));
}

static ssize_t lab_read(struct file *file, char __user *buf, size_t count,
			loff_t *ppos)
{
	unsigned int copied;
	int ret;

	if (count < sizeof(u32))
		return -EINVAL;

	if (mutex_lock_interruptible(&lab_lock))
		return -ERESTARTSYS;

	/*
	 * A loop, not an if: another reader may take the sample between the
	 * wakeup and this task getting the lock back.
	 */
	while (kfifo_is_empty(&lab_fifo)) {
		mutex_unlock(&lab_lock);

		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_event_interruptible(lab_readq,
					     !kfifo_is_empty(&lab_fifo)))
			return -ERESTARTSYS;
		if (mutex_lock_interruptible(&lab_lock))
			return -ERESTARTSYS;
	}

	ret = kfifo_to_user(&lab_fifo, buf, count, &copied);
	mutex_unlock(&lab_lock);
	return ret ? ret : copied;
}

static __poll_t lab_poll(struct file *file, poll_table *wait)
{
	/* Register first, then look at the state. */
	poll_wait(file, &lab_readq, wait);
	return kfifo_is_empty(&lab_fifo) ? 0 : EPOLLIN | EPOLLRDNORM;
}

static int lab_open(struct inode *inode, struct file *file)
{
	/* A stream: no file position, so reject seeking and pread(). */
	return stream_open(inode, file);
}

static const struct file_operations lab_fops = {
	.owner = THIS_MODULE,	/* blocks rmmod while a file is open */
	.open = lab_open,
	.read = lab_read,
	.poll = lab_poll,
};

static struct miscdevice lab_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lab_poll",
	.fops = &lab_fops,
};

static int __init lab_init(void)
{
	int ret;

	if (period_ms < 10 || period_ms > 60000)
		return -EINVAL;

	/* Everything the callbacks use is ready, so register last. */
	ret = misc_register(&lab_misc);
	if (ret)
		return ret;

	schedule_delayed_work(&lab_work, msecs_to_jiffies(period_ms));
	return 0;
}

static void __exit lab_exit(void)
{
	misc_deregister(&lab_misc);
	/* Safe although the work requeues itself. */
	cancel_delayed_work_sync(&lab_work);
	pr_info("lab11_poll: produced %u, dropped %u\n", lab_next, lab_dropped);
}

module_init(lab_init);
module_exit(lab_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 11: blocking read and poll");
