// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "lab_ioctl.h"

#define LAB_DEFAULT_RATE_HZ 10

static DEFINE_MUTEX(lab_lock);
static u32 lab_flags;
static u32 lab_rate_hz = LAB_DEFAULT_RATE_HZ;
static u64 lab_set_count;

static int lab_check_config(const struct lab_ioctl_config *cfg)
{
	if (cfg->version != LAB_IOCTL_ABI_VERSION)
		return -EINVAL;
	if (cfg->flags & ~LAB_IOCTL_FLAGS_ALL)
		return -EINVAL;
	if (cfg->rate_hz < 1 || cfg->rate_hz > 1000)
		return -ERANGE;
	/* Rejecting non-zero now keeps the fields usable later. */
	if (memchr_inv(cfg->reserved, 0, sizeof(cfg->reserved)))
		return -EINVAL;
	return 0;
}

static long lab_set_config(void __user *argp)
{
	struct lab_ioctl_config cfg;
	int ret;

	/* Copy once, then validate and use only the kernel copy. */
	if (copy_from_user(&cfg, argp, sizeof(cfg)))
		return -EFAULT;

	ret = lab_check_config(&cfg);
	if (ret)
		return ret;

	mutex_lock(&lab_lock);
	lab_flags = cfg.flags;
	lab_rate_hz = cfg.rate_hz;
	lab_set_count++;
	mutex_unlock(&lab_lock);
	return 0;
}

static long lab_get_status(void __user *argp)
{
	struct lab_ioctl_status status;

	/* memset also clears padding, so no kernel stack bytes leak. */
	memset(&status, 0, sizeof(status));
	status.version = LAB_IOCTL_ABI_VERSION;

	mutex_lock(&lab_lock);
	status.flags = lab_flags;
	status.rate_hz = lab_rate_hz;
	status.set_count = lab_set_count;
	mutex_unlock(&lab_lock);

	if (copy_to_user(argp, &status, sizeof(status)))
		return -EFAULT;
	return 0;
}

static long lab_reset(void)
{
	mutex_lock(&lab_lock);
	lab_flags = 0;
	lab_rate_hz = LAB_DEFAULT_RATE_HZ;
	lab_set_count = 0;
	mutex_unlock(&lab_lock);
	return 0;
}

static long lab_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case LAB_IOCTL_SET_CONFIG:
		return lab_set_config(argp);
	case LAB_IOCTL_GET_STATUS:
		return lab_get_status(argp);
	case LAB_IOCTL_RESET:
		return lab_reset();
	default:
		return -ENOTTY;
	}
}

static const struct file_operations lab_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = lab_ioctl,
	/* The structures have the same layout for 32-bit userspace. */
	.compat_ioctl = compat_ptr_ioctl,
	.llseek = noop_llseek,
};

static struct miscdevice lab_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lab_ioctl",
	.fops = &lab_fops,
};

module_misc_device(lab_misc);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 10: ioctl ABI on a misc device");
