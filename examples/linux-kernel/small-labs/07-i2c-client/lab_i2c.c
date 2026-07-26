// SPDX-License-Identifier: GPL-2.0
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>

static int lab_i2c_probe(struct i2c_client *client)
{
	int value;

	if (!i2c_check_functionality(client->adapter,
				    I2C_FUNC_SMBUS_BYTE_DATA))
		return dev_err_probe(&client->dev, -EOPNOTSUPP,
				     "adapter lacks SMBus byte-data support\n");

	value = i2c_smbus_read_byte_data(client, 0x00);
	if (value < 0)
		return dev_err_probe(&client->dev, value,
				     "register 0x00 read failed\n");

	dev_info(&client->dev, "client probed; register 0x00 = 0x%02x\n",
		 value);
	return 0;
}

static void lab_i2c_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "client removed\n");
}

static const struct of_device_id lab_i2c_of_match[] = {
	{ .compatible = "example,linux-kernel-i2c" },
	{ }
};
MODULE_DEVICE_TABLE(of, lab_i2c_of_match);

static const struct i2c_device_id lab_i2c_ids[] = {
	{ "lab_i2c_demo", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lab_i2c_ids);

static struct i2c_driver lab_i2c_driver = {
	.probe = lab_i2c_probe,
	.remove = lab_i2c_remove,
	.id_table = lab_i2c_ids,
	.driver = {
		.name = "lab_i2c_demo",
		.of_match_table = lab_i2c_of_match,
	},
};

module_i2c_driver(lab_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 7: I2C client driver");

