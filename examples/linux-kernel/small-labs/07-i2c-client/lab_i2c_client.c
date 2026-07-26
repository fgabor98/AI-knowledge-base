// SPDX-License-Identifier: GPL-2.0
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>

static int bus;
module_param(bus, int, 0444);
MODULE_PARM_DESC(bus, "I2C adapter number");

static int address = 0x48;
module_param(address, int, 0444);
MODULE_PARM_DESC(address, "7-bit I2C address");

static struct i2c_client *lab_client;

static int __init lab_i2c_client_init(void)
{
	struct i2c_board_info info = {
		.type = "lab_i2c_demo",
		.addr = address,
	};
	struct i2c_adapter *adapter;

	if (bus < 0 || address < 0 || address > 0x7f)
		return -EINVAL;

	adapter = i2c_get_adapter(bus);
	if (!adapter)
		return -ENODEV;

	lab_client = i2c_new_client_device(adapter, &info);
	i2c_put_adapter(adapter);
	if (IS_ERR(lab_client))
		return PTR_ERR(lab_client);

	pr_info("lab7_i2c_client: registered bus %d address 0x%02x\n",
		bus, address);
	return 0;
}

static void __exit lab_i2c_client_exit(void)
{
	i2c_unregister_device(lab_client);
	pr_info("lab7_i2c_client: unregistered client\n");
}

module_init(lab_i2c_client_init);
module_exit(lab_i2c_client_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 7: software I2C client helper");

