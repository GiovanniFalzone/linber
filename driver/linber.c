/*
*/
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/ioctl.h>

#include "../libs/linber_ioctl.h"


#define DEV_NAME DEVICE_FILE_NAME
#define CLA_NAME "linberclass"

static int dev_major;
static struct class*	dev_class	= NULL; ///< The device-driver class struct pointer
static struct device* dev_device = NULL; ///< The device-driver device struct pointer



static int		dev_open(struct inode *n, struct file *f) { try_module_get(THIS_MODULE); return 0; }
static int		dev_release(struct inode *n, struct file *f) { module_put(THIS_MODULE); return 0; }
static ssize_t	dev_read(struct file *f, char *buf, size_t sz, loff_t *off) { return 0; }
static ssize_t	dev_write(struct file *f, const char *buf, size_t sz, loff_t *off) { return	0; }

static long		linber_ioctl(struct file *f, unsigned int cmd, unsigned long arg){ 
	switch(cmd){
		case IOCTL_REGISTER_SERVICE:
			printk(KERN_INFO "IOCTL Registration received\n");
			break;

		case IOCTL_REQUEST_SERVICE:
			printk(KERN_INFO "IOCTL Request received\n");
			break;

		default:
			printk(KERN_ERR "IOCTL operation %d not implemented\n", cmd);
			break;
	}

	return 0;
}


struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
	.unlocked_ioctl = linber_ioctl,
};

int init_module(void) {
	int ret;
	printk(KERN_INFO "Registering device %s\n", DEV_NAME);
	dev_major = MAJOR_NUM;
	ret = register_chrdev(MAJOR_NUM, DEV_NAME, &fops);
	if (ret < 0) {
		printk(KERN_ERR "register_chrdev() failed with: %d\n", ret);
		return ret;
	}
	printk(KERN_INFO "Assigned major: %d\n", dev_major);

	dev_class = class_create(THIS_MODULE, CLA_NAME);
	if (IS_ERR(dev_class)) {
		printk(KERN_ERR "class_create() failed\n");
		unregister_chrdev(dev_major, DEV_NAME);
		return PTR_ERR(dev_class);
	}

	dev_device = device_create(dev_class, NULL, MKDEV(dev_major, 0), NULL, DEV_NAME);
	if (IS_ERR(dev_device)) {
		printk(KERN_ERR "device_create() failed\n");
		class_destroy(dev_class);
		unregister_chrdev(dev_major, DEV_NAME);
		return PTR_ERR(dev_device);
	}
	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "Destroying device %s\n", CLA_NAME);
	device_destroy(dev_class, MKDEV(dev_major, 0));
	printk(KERN_INFO "Unregistering device class %s\n", CLA_NAME);
	class_unregister(dev_class);
	printk(KERN_INFO "Destroying device class %s\n", CLA_NAME);
	class_destroy(dev_class);
	printk(KERN_INFO "Unregistering device %s\n", DEV_NAME);
	unregister_chrdev(dev_major, DEV_NAME);
}

MODULE_LICENSE("GPL");
