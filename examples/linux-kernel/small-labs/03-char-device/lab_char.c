// SPDX-License-Identifier: GPL-2.0
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define DEMO_NAME "lab_char"
#define DEMO_NODE "demo0"
#define DEMO_BUFFER_SIZE 128

struct demo_device {
	struct cdev cdev;
	struct class *class;
	struct device *device;
	dev_t devt;
	struct mutex lock;
	char buffer[DEMO_BUFFER_SIZE];
	size_t length;
};

static struct demo_device demo;

static int demo_open(struct inode *inode, struct file *file)
{
	file->private_data = container_of(inode->i_cdev, struct demo_device, cdev);
	pr_info("lab3_char: open\n");
	return 0;
}

static int demo_release(struct inode *inode, struct file *file)
{
	pr_info("lab3_char: release\n");
	return 0;
}

static ssize_t demo_read(struct file *file, char __user *user_buffer,
			 size_t count, loff_t *position)
{
	struct demo_device *device = file->private_data;
	size_t available;
	ssize_t ret;

	if (mutex_lock_interruptible(&device->lock))
		return -ERESTARTSYS;

	if (*position >= device->length) {
		ret = 0;
		goto out_unlock;
	}

	available = device->length - *position;
	count = min(count, available);
	if (copy_to_user(user_buffer, device->buffer + *position, count)) {
		ret = -EFAULT;
		goto out_unlock;
	}

	*position += count;
	ret = count;

out_unlock:
	mutex_unlock(&device->lock);
	return ret;
}

static ssize_t demo_write(struct file *file, const char __user *user_buffer,
			  size_t count, loff_t *position)
{
	struct demo_device *device = file->private_data;
	size_t to_copy;
	ssize_t ret;

	to_copy = min(count, (size_t)DEMO_BUFFER_SIZE - 1);
	if (mutex_lock_interruptible(&device->lock))
		return -ERESTARTSYS;

	if (copy_from_user(device->buffer, user_buffer, to_copy)) {
		ret = -EFAULT;
		goto out_unlock;
	}

	device->buffer[to_copy] = '\0';
	device->length = to_copy;
	*position = to_copy;
	ret = to_copy;

out_unlock:
	mutex_unlock(&device->lock);
	return ret;
}

static const struct file_operations demo_fops = {
	.owner = THIS_MODULE,
	.open = demo_open,
	.release = demo_release,
	.read = demo_read,
	.write = demo_write,
	.llseek = noop_llseek,
};

static int __init demo_init(void)
{
	int ret;

	mutex_init(&demo.lock);
	ret = alloc_chrdev_region(&demo.devt, 0, 1, DEMO_NAME);
	if (ret)
		return ret;

	cdev_init(&demo.cdev, &demo_fops);
	ret = cdev_add(&demo.cdev, demo.devt, 1);
	if (ret)
		goto err_unregister;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	demo.class = class_create(DEMO_NAME);
#else
	demo.class = class_create(THIS_MODULE, DEMO_NAME);
#endif
	if (IS_ERR(demo.class)) {
		ret = PTR_ERR(demo.class);
		goto err_cdev;
	}

	demo.device = device_create(demo.class, NULL, demo.devt, NULL, DEMO_NODE);
	if (IS_ERR(demo.device)) {
		ret = PTR_ERR(demo.device);
		goto err_class;
	}

	pr_info("lab3_char: registered /dev/%s (%u:%u)\n", DEMO_NODE,
		MAJOR(demo.devt), MINOR(demo.devt));
	return 0;

err_class:
	class_destroy(demo.class);
err_cdev:
	cdev_del(&demo.cdev);
err_unregister:
	unregister_chrdev_region(demo.devt, 1);
	return ret;
}

static void __exit demo_exit(void)
{
	device_destroy(demo.class, demo.devt);
	class_destroy(demo.class);
	cdev_del(&demo.cdev);
	unregister_chrdev_region(demo.devt, 1);
	pr_info("lab3_char: unregistered /dev/%s\n", DEMO_NODE);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 3: dummy character device");
