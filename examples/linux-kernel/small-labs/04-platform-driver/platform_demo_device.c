// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static struct platform_device *demo_device;

static int __init demo_device_init(void)
{
	demo_device = platform_device_register_simple("lab_platform_demo",
						      0, NULL, 0);
	if (IS_ERR(demo_device))
		return PTR_ERR(demo_device);

	pr_info("lab4_platform_device: registered software platform device\n");
	return 0;
}

static void __exit demo_device_exit(void)
{
	platform_device_unregister(demo_device);
	pr_info("lab4_platform_device: unregistered software platform device\n");
}

module_init(demo_device_init);
module_exit(demo_device_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 4: fake platform device helper");
