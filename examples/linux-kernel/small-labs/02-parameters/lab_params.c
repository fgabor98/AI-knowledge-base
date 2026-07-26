// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>

static int interval_ms = 1000;
module_param(interval_ms, int, 0444);
MODULE_PARM_DESC(interval_ms, "Diagnostic interval in milliseconds (1..60000)");

static int __init lab_params_init(void)
{
	if (interval_ms < 1 || interval_ms > 60000) {
		pr_err("lab2_params: interval_ms must be between 1 and 60000\n");
		return -EINVAL;
	}

	pr_info("lab2_params: loaded with interval_ms=%d\n", interval_ms);
	return 0;
}

static void __exit lab_params_exit(void)
{
	pr_info("lab2_params: unloaded\n");
}

module_init(lab_params_init);
module_exit(lab_params_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 2: module parameters");

