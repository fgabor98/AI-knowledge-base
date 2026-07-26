// SPDX-License-Identifier: GPL-2.0
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define DRIVER_NAME "lab_platform_demo"
#define DEVICE_NODE "platform_demo0"
#define BUFFER_SIZE 128

struct platform_demo {
	struct cdev cdev;
	struct class *class;
	struct device *device;
	dev_t devt;
	struct mutex lock;
	char buffer[BUFFER_SIZE];
	size_t length;
};

static int demo_open(struct inode *inode, struct file *file)
{
	file->private_data = container_of(inode->i_cdev,
					 struct platform_demo, cdev);
	pr_info("lab4_platform: open\n");
	return 0;
}

static int demo_release(struct inode *inode, struct file *file)
{
	pr_info("lab4_platform: release\n");
	return 0;
}

static ssize_t demo_read(struct file *file, char __user *user_buffer,
			 size_t count, loff_t *position)
{
	struct platform_demo *demo = file->private_data;
	size_t available;
	ssize_t ret;

	if (mutex_lock_interruptible(&demo->lock))
		return -ERESTARTSYS;
	if (*position >= demo->length) {
		ret = 0;
		goto out_unlock;
	}

	available = demo->length - *position;
	count = min(count, available);
	if (copy_to_user(user_buffer, demo->buffer + *position, count)) {
		ret = -EFAULT;
		goto out_unlock;
	}

	*position += count;
	ret = count;

out_unlock:
	mutex_unlock(&demo->lock);
	return ret;
}

static ssize_t demo_write(struct file *file, const char __user *user_buffer,
			  size_t count, loff_t *position)
{
	struct platform_demo *demo = file->private_data;
	size_t to_copy = min(count, (size_t)BUFFER_SIZE - 1);
	ssize_t ret;

	if (mutex_lock_interruptible(&demo->lock))
		return -ERESTARTSYS;
	if (copy_from_user(demo->buffer, user_buffer, to_copy)) {
		ret = -EFAULT;
		goto out_unlock;
	}

	demo->buffer[to_copy] = '\0';
	demo->length = to_copy;
	*position = to_copy;
	ret = to_copy;

out_unlock:
	mutex_unlock(&demo->lock);
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

static int platform_demo_probe(struct platform_device *pdev)
{
	struct platform_demo *demo;
	int ret;

	demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	mutex_init(&demo->lock);
	ret = alloc_chrdev_region(&demo->devt, 0, 1, DRIVER_NAME);
	if (ret)
		return ret;

	cdev_init(&demo->cdev, &demo_fops);
	ret = cdev_add(&demo->cdev, demo->devt, 1);
	if (ret)
		goto err_unregister;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	demo->class = class_create(DRIVER_NAME);
#else
	demo->class = class_create(THIS_MODULE, DRIVER_NAME);
#endif
	if (IS_ERR(demo->class)) {
		ret = PTR_ERR(demo->class);
		goto err_cdev;
	}

	demo->device = device_create(demo->class, &pdev->dev, demo->devt,
				     demo, DEVICE_NODE);
	if (IS_ERR(demo->device)) {
		ret = PTR_ERR(demo->device);
		goto err_class;
	}

	platform_set_drvdata(pdev, demo);
	dev_info(&pdev->dev, "probed; userspace node is /dev/%s\n", DEVICE_NODE);
	return 0;

err_class:
	class_destroy(demo->class);
err_cdev:
	cdev_del(&demo->cdev);
err_unregister:
	unregister_chrdev_region(demo->devt, 1);
	return ret;
}

static void platform_demo_remove(struct platform_device *pdev)
{
	struct platform_demo *demo = platform_get_drvdata(pdev);

	device_destroy(demo->class, demo->devt);
	class_destroy(demo->class);
	cdev_del(&demo->cdev);
	unregister_chrdev_region(demo->devt, 1);
	dev_info(&pdev->dev, "removed\n");
}

static struct platform_driver platform_demo_driver = {
	.probe = platform_demo_probe,
	.remove = platform_demo_remove,
	.driver = {
		.name = DRIVER_NAME,
	},
};

module_platform_driver(platform_demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 4: software platform driver");
