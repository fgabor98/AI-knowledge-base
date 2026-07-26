// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>

static int __init lab_hello_init(void)
{
	pr_info("lab1_hello: loaded\n");
	return 0;
}

static void __exit lab_hello_exit(void)
{
	pr_info("lab1_hello: unloaded\n");
}

module_init(lab_hello_init);
module_exit(lab_hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 1: hello module");

