// SPDX-License-Identifier: GPL-2.0
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/slab.h>

static bool broken;
module_param(broken, bool, 0444);
MODULE_PARM_DESC(broken, "Update in place instead of publishing a new copy");

#define LAB_READERS 2

/* Invariant that every reader must observe: b == 2 * a. */
struct lab_cfg {
	u32 a;
	u32 b;
	struct rcu_head rcu;
};

static struct lab_cfg __rcu *lab_cfg;
static DEFINE_MUTEX(lab_update_lock);	/* RCU does not serialize writers */

static DEFINE_PER_CPU(unsigned long, lab_reads);
static atomic_t lab_torn = ATOMIC_INIT(0);
static unsigned long lab_updates;

static struct task_struct *lab_readers[LAB_READERS];
static struct task_struct *lab_writer;

static int lab_reader_fn(void *data)
{
	while (!kthread_should_stop()) {
		const struct lab_cfg *cfg;
		u32 a, b;

		rcu_read_lock();
		cfg = rcu_dereference(lab_cfg);
		a = READ_ONCE(cfg->a);
		udelay(2);		/* widen the race window */
		b = READ_ONCE(cfg->b);
		rcu_read_unlock();	/* cfg must not be used after this */

		if (b != 2 * a)
			atomic_inc(&lab_torn);
		this_cpu_inc(lab_reads);
		cond_resched();
	}
	return 0;
}

static int lab_update(u32 a)
{
	struct lab_cfg *new, *old;

	new = kmalloc(sizeof(*new), GFP_KERNEL);
	if (!new)
		return -ENOMEM;
	new->a = a;
	new->b = 2 * a;

	mutex_lock(&lab_update_lock);
	old = rcu_replace_pointer(lab_cfg, new,
				  lockdep_is_held(&lab_update_lock));
	mutex_unlock(&lab_update_lock);

	/* Readers may still use old; free it after a grace period. */
	if (old)
		kfree_rcu(old, rcu);
	return 0;
}

/* Wrong on purpose: readers can see the new a with the old b. */
static void lab_update_in_place(u32 a)
{
	struct lab_cfg *cfg;

	mutex_lock(&lab_update_lock);
	cfg = rcu_dereference_protected(lab_cfg,
					lockdep_is_held(&lab_update_lock));
	WRITE_ONCE(cfg->a, a);
	udelay(2);
	WRITE_ONCE(cfg->b, 2 * a);
	mutex_unlock(&lab_update_lock);
}

static int lab_writer_fn(void *data)
{
	u32 a = 0;

	while (!kthread_should_stop()) {
		a++;
		if (broken)
			lab_update_in_place(a);
		else
			lab_update(a);
		lab_updates++;
		usleep_range(100, 200);
	}
	return 0;
}

static void lab_stop_all(void)
{
	int i;

	if (!IS_ERR_OR_NULL(lab_writer))
		kthread_stop(lab_writer);
	for (i = 0; i < LAB_READERS; i++)
		if (!IS_ERR_OR_NULL(lab_readers[i]))
			kthread_stop(lab_readers[i]);

	/* No reader is left, so the pointer can be freed directly. */
	kfree(rcu_dereference_protected(lab_cfg, true));
	RCU_INIT_POINTER(lab_cfg, NULL);
}

static int __init lab_init(void)
{
	int i, ret;

	/* Publish a valid object before any reader can run. */
	ret = lab_update(0);
	if (ret)
		return ret;

	for (i = 0; i < LAB_READERS; i++) {
		lab_readers[i] = kthread_run(lab_reader_fn, NULL,
					     "lab_rcu_r/%d", i);
		if (IS_ERR(lab_readers[i])) {
			ret = PTR_ERR(lab_readers[i]);
			goto err;
		}
	}

	lab_writer = kthread_run(lab_writer_fn, NULL, "lab_rcu_w");
	if (IS_ERR(lab_writer)) {
		ret = PTR_ERR(lab_writer);
		goto err;
	}
	return 0;

err:
	lab_stop_all();
	return ret;
}

static void __exit lab_exit(void)
{
	unsigned long reads = 0;
	int cpu;

	lab_stop_all();

	for_each_possible_cpu(cpu)
		reads += per_cpu(lab_reads, cpu);
	pr_info("lab14_rcu: mode=%s reads=%lu updates=%lu torn=%d\n",
		broken ? "in-place" : "rcu", reads, lab_updates,
		atomic_read(&lab_torn));
}

module_init(lab_init);
module_exit(lab_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 14: RCU publish and reclaim");
