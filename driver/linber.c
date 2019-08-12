/*
*/
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sempahore.h>
#include <linux/device.h>
#include <linux/ioctl.h>

#include "../libs/linber_ioctl.h"


#define DEV_NAME DEVICE_FILE_NAME
#define CLA_NAME "linberclass"

static int dev_major;
static struct class*	dev_class	= NULL; ///< The device-driver class struct pointer
static struct device* dev_device = NULL; ///< The device-driver device struct pointer

static int		dev_open(struct inode *n, struct file *f) {
	try_module_get(THIS_MODULE);
	return 0;
}

static int		dev_release(struct inode *n, struct file *f) {
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t	dev_read(struct file *f, char *buf, size_t sz, loff_t *off) {
	return 0;
}

static ssize_t	dev_write(struct file *f, const char *buf, size_t sz, loff_t *off) {
	return	0;
}

/*
*/
struct semaphore Workers;	// waiting for requests, count=MAX_CONCURRENT
// list of requests, each of them with a mutex/lock blocking the single client


static int linber_finishJob(){
	return 0;
}

static int linber_register_service(linber_service_struct *obj){
	printk(KERN_INFO "linber:: IOCTL Registration received, name:%s, len:%u, exec_time:%u\n", obj->service_uri, obj->service_uri_len, obj->service_time);
	// if new service, then create new service
		sema_init(&Workers, 1);

	// else add new worker to service
	return 0;
}

static int linber_request_service(linber_service_struct *obj){
	// search service
	// if not wxisting return error
	// else wait for workers
	return 0;
}


static long		linber_ioctl(struct file *f, unsigned int cmd, unsigned long ioctl_param){ 
	char *uri_tmp;
	linber_service_struct *obj = kmalloc(sizeof(linber_service_struct), GFP_KERNEL);
	copy_from_user(obj, (void *)ioctl_param, sizeof(linber_service_struct));
	uri_tmp = kmalloc(obj->service_uri_len, GFP_KERNEL);
	copy_from_user((char*)uri_tmp, (char*)obj->service_uri, obj->service_uri_len);
	obj->service_uri = uri_tmp;

	switch(cmd){
		case IOCTL_REGISTER_SERVICE:
			linber_register_service(obj);
			break;

		case IOCTL_REQUEST_SERVICE:
			linber_request_service(obj);
			break;

		default:
			printk(KERN_ERR "linber:: IOCTL operation %d not implemented\n", cmd);
			break;
	}
	kfree(obj);
	return 0;
}


static int linber_uevent(struct device *dev, struct kobj_uevent_env *env){
	add_uevent_var(env, "DEVMODE=%#o", 0666);
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
	printk(KERN_INFO "linber:: Registering device %s\n", DEV_NAME);
	dev_major = MAJOR_NUM;
	ret = register_chrdev(MAJOR_NUM, DEV_NAME, &fops);
	if (ret < 0) {
		printk(KERN_ERR "linber:: register_chrdev() failed with: %d\n", ret);
		return ret;
	}
	printk(KERN_INFO "linber:: Assigned major: %d\n", dev_major);

	dev_class = class_create(THIS_MODULE, CLA_NAME);
	if (IS_ERR(dev_class)) {
		printk(KERN_ERR "linber:: class_create() failed\n");
		unregister_chrdev(dev_major, DEV_NAME);
		return PTR_ERR(dev_class);
	}
	dev_class->dev_uevent = linber_uevent;

	dev_device = device_create(dev_class, NULL, MKDEV(dev_major, 0), NULL, DEV_NAME);
	if (IS_ERR(dev_device)) {
		printk(KERN_ERR "linber:: device_create() failed\n");
		class_destroy(dev_class);
		unregister_chrdev(dev_major, DEV_NAME);
		return PTR_ERR(dev_device);
	}
	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "linber:: Destroying device %s\n", CLA_NAME);
	device_destroy(dev_class, MKDEV(dev_major, 0));
	printk(KERN_INFO "linber:: Unregistering device class %s\n", CLA_NAME);
	class_unregister(dev_class);
	printk(KERN_INFO "linber:: Destroying device class %s\n", CLA_NAME);
	class_destroy(dev_class);
	printk(KERN_INFO "linber:: Unregistering device %s\n", DEV_NAME);
	unregister_chrdev(dev_major, DEV_NAME);
}

MODULE_LICENSE("GPL");
