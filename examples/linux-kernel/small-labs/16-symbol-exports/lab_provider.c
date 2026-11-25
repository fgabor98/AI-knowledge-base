// SPDX-License-Identifier: GPL-2.0
#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/version.h>

#include "lab_provider.h"

static atomic_t lab_counter = ATOMIC_INIT(0);

u32 lab_provider_next(void)
{
	return atomic_inc_return(&lab_counter);
}
/* The namespace became a string literal in 6.13. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
EXPORT_SYMBOL_NS_GPL(lab_provider_next, "KB_LAB");
#else
EXPORT_SYMBOL_NS_GPL(lab_provider_next, KB_LAB);
#endif

static int __init lab_provider_init(void)
{
	return 0;
}

static void __exit lab_provider_exit(void)
{
}

module_init(lab_provider_init);
module_exit(lab_provider_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 16: module that exports a symbol");
