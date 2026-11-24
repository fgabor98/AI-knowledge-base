// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/version.h>

#include "lab_provider.h"

static int __init lab_consumer_init(void)
{
	pr_info("lab16_consumer: value %u from lab_provider\n",
		lab_provider_next());
	return 0;
}

static void __exit lab_consumer_exit(void)
{
}

module_init(lab_consumer_init);
module_exit(lab_consumer_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 16: module that uses an exported symbol");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
MODULE_IMPORT_NS("KB_LAB");
#else
MODULE_IMPORT_NS(KB_LAB);
#endif
