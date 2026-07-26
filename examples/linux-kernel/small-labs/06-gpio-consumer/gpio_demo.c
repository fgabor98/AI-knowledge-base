// SPDX-License-Identifier: GPL-2.0
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static int gpio_demo_probe(struct platform_device *pdev)
{
	struct gpio_desc *reset_gpio;

	reset_gpio = devm_gpiod_get_optional(&pdev->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(reset_gpio))
		return dev_err_probe(&pdev->dev, PTR_ERR(reset_gpio),
				     "failed to get reset GPIO\n");

	if (!reset_gpio) {
		dev_info(&pdev->dev,
			 "matched successfully; no reset-gpios property was supplied\n");
		return 0;
	}

	dev_info(&pdev->dev, "reset GPIO acquired; driving it active\n");
	gpiod_set_value_cansleep(reset_gpio, 1);
	return 0;
}

static void gpio_demo_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "GPIO consumer removed\n");
}

static const struct of_device_id gpio_demo_of_match[] = {
	{ .compatible = "example,linux-kernel-gpio" },
	{ }
};
MODULE_DEVICE_TABLE(of, gpio_demo_of_match);

static struct platform_driver gpio_demo_driver = {
	.probe = gpio_demo_probe,
	.remove = gpio_demo_remove,
	.driver = {
		.name = "lab_gpio_demo",
		.of_match_table = gpio_demo_of_match,
	},
};

module_platform_driver(gpio_demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 6: GPIO consumer");
