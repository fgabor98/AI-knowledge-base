// SPDX-License-Identifier: GPL-2.0
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

struct lab_item {
	struct list_head node;
	u32 id;
};

static DEFINE_MUTEX(lab_lock);	/* protects lab_items and the counters */
static LIST_HEAD(lab_items);
static u32 lab_count;
static u32 lab_next_id;

static u32 lab_tunable = 42;	/* plain debug variable, exported directly */
static struct dentry *lab_dir;

static int lab_add(u32 n)
{
	while (n--) {
		struct lab_item *item = kzalloc(sizeof(*item), GFP_KERNEL);

		if (!item)
			return -ENOMEM;
		mutex_lock(&lab_lock);
		item->id = lab_next_id++;
		list_add_tail(&item->node, &lab_items);
		lab_count++;
		mutex_unlock(&lab_lock);
	}
	return 0;
}

static void lab_clear(void)
{
	struct lab_item *item, *tmp;

	mutex_lock(&lab_lock);
	list_for_each_entry_safe(item, tmp, &lab_items, node) {
		list_del(&item->node);
		kfree(item);
	}
	lab_count = 0;
	mutex_unlock(&lab_lock);
}

/* --- /proc/lab_seq: iterate over the list --- */

static void *lab_seq_start(struct seq_file *m, loff_t *pos)
{
	mutex_lock(&lab_lock);
	return seq_list_start(&lab_items, *pos);
}

static void *lab_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	return seq_list_next(v, &lab_items, pos);
}

/* Called after every start(), even if start() returned NULL. */
static void lab_seq_stop(struct seq_file *m, void *v)
{
	mutex_unlock(&lab_lock);
}

static int lab_seq_show(struct seq_file *m, void *v)
{
	const struct lab_item *item = list_entry(v, struct lab_item, node);

	seq_printf(m, "item %u\n", item->id);
	return 0;
}

static const struct seq_operations lab_seq_ops = {
	.start = lab_seq_start,
	.next = lab_seq_next,
	.stop = lab_seq_stop,
	.show = lab_seq_show,
};

/* --- debugfs: one formatted file, two write-only commands --- */

static int lab_summary_show(struct seq_file *m, void *unused)
{
	mutex_lock(&lab_lock);
	seq_printf(m, "count: %u\nnext_id: %u\n", lab_count, lab_next_id);
	mutex_unlock(&lab_lock);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(lab_summary);

static int lab_add_set(void *data, u64 val)
{
	return val > 100000 ? -ERANGE : lab_add(val);
}
DEFINE_DEBUGFS_ATTRIBUTE(lab_add_fops, NULL, lab_add_set, "%llu\n");

static int lab_clear_set(void *data, u64 val)
{
	lab_clear();
	return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(lab_clear_fops, NULL, lab_clear_set, "%llu\n");

static int __init lab_init(void)
{
	int ret;

	ret = lab_add(3);
	if (ret)
		goto err_clear;

	if (!proc_create_seq("lab_seq", 0444, NULL, &lab_seq_ops)) {
		ret = -ENOMEM;
		goto err_clear;
	}

	/* debugfs is best effort: its return values are not checked. */
	lab_dir = debugfs_create_dir("lab_seq", NULL);
	debugfs_create_u32("tunable", 0644, lab_dir, &lab_tunable);
	debugfs_create_file("summary", 0444, lab_dir, NULL, &lab_summary_fops);
	debugfs_create_file_unsafe("add", 0200, lab_dir, NULL, &lab_add_fops);
	debugfs_create_file_unsafe("clear", 0200, lab_dir, NULL, &lab_clear_fops);
	return 0;

err_clear:
	lab_clear();
	return ret;
}

static void __exit lab_exit(void)
{
	/* Both calls wait for running file operations before returning. */
	debugfs_remove(lab_dir);
	remove_proc_entry("lab_seq", NULL);
	lab_clear();
}

module_init(lab_init);
module_exit(lab_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 12: procfs seq_file and debugfs");
