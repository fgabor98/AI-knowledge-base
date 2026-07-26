// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static int dt_demo_probe(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "Device Tree matched node %s\n",
		 pdev->dev.of_node ? pdev->dev.of_node->full_name : "<none>");
	return 0;
}

static void dt_demo_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "Device Tree matched node removed\n");
}

static const struct of_device_id dt_demo_of_match[] = {
	{ .compatible = "example,linux-kernel-lab" },
	{ }
};
MODULE_DEVICE_TABLE(of, dt_demo_of_match);

static struct platform_driver dt_demo_driver = {
	.probe = dt_demo_probe,
	.remove = dt_demo_remove,
	.driver = {
		.name = "lab_dt_demo",
		.of_match_table = dt_demo_of_match,
	},
};

module_platform_driver(dt_demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 5: Device Tree matching");

