// SPDX-License-Identifier: GPL-2.0
#include <linux/err.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>

static struct input_dev *lab_input;
static DEFINE_MUTEX(lab_lock);	/* protects lab_input against late writes */

/* A parameter with callbacks: validate and act on every write. */
static int lab_press_set(const char *val, const struct kernel_param *kp)
{
	unsigned int times;
	int ret = kstrtouint(val, 0, &times);

	if (ret)
		return ret;
	if (times > 10)
		return -ERANGE;

	mutex_lock(&lab_lock);
	if (!lab_input) {
		/* Also reached for "insmod lab_input.ko press=1". */
		ret = -ENODEV;
	} else {
		while (times--) {
			input_report_key(lab_input, KEY_F24, 1);
			input_sync(lab_input);
			input_report_key(lab_input, KEY_F24, 0);
			input_sync(lab_input);
		}
	}
	mutex_unlock(&lab_lock);
	return ret;
}

static const struct kernel_param_ops lab_press_ops = {
	.set = lab_press_set,
};

module_param_cb(press, &lab_press_ops, NULL, 0200);
MODULE_PARM_DESC(press, "Write N (0..10) to report N presses of KEY_F24");

static int __init lab_init(void)
{
	struct input_dev *input;
	int ret;

	input = input_allocate_device();
	if (!input)
		return -ENOMEM;

	input->name = "lab-virtual-key";
	input->phys = "lab/input0";
	input->id.bustype = BUS_VIRTUAL;
	input_set_capability(input, EV_KEY, KEY_F24);

	ret = input_register_device(input);
	if (ret) {
		/* Not registered yet: free, do not unregister. */
		input_free_device(input);
		return ret;
	}

	mutex_lock(&lab_lock);
	lab_input = input;
	mutex_unlock(&lab_lock);
	return 0;
}

static void __exit lab_exit(void)
{
	struct input_dev *input;

	mutex_lock(&lab_lock);
	input = lab_input;
	lab_input = NULL;
	mutex_unlock(&lab_lock);

	/* Drops the last reference too: no input_free_device() after this. */
	input_unregister_device(input);
}

module_init(lab_init);
module_exit(lab_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 15: virtual input device");
