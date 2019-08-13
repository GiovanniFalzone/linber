/*
*/
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/ioctl.h>

#include "../libs/linber_ioctl.h"

#define DEV_NAME DEVICE_FILE_NAME
#define CLA_NAME "linberclass"

#define EMPTY_LIST	-1

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
//struct semaphore Workers;	// waiting for requests, count=MAX_CONCURRENT

//------------------------------------------------------
typedef struct RequestNode{
	struct list_head list;
	int id;
	struct semaphore Request_sem;
}RequestNode;

typedef struct ServiceNode{
	struct list_head list;
	int id;
	char *uri;
	struct list_head RequestsHead;
	// semaphore, block workers waiting for requests
}ServiceNode;

static struct list_head ServicesHead;

// list of requests, each of them with a mutex/lock blocking the single client
// ------------------------------------------------------
static int initialize_queue(struct list_head *head){
	INIT_LIST_HEAD(head);
	return 0;
}

static void print_request_list(struct list_head *head){
	struct list_head *p;
	RequestNode *n;
	if(list_empty(head)){
		printk(KERN_INFO "linber:: Empty list\n");
	} else {
		list_for_each(p, head){
			n = list_entry(p, RequestNode, list);
			printk(KERN_INFO "linber:: list element:%d\n", n->id);
		}
	}
}

static RequestNode *Enqueue_Request(struct list_head *head, int id){
	RequestNode *node;
	node = kmalloc(sizeof(*node) ,GFP_KERNEL);
	node->id = id;
	sema_init(&node->Request_sem, 0);
	list_add_tail(&node->list, head);
	printk(KERN_INFO "linber:: Enqueue Request:%d\n", node->id);
	return node;
}

static RequestNode *Dequeue_Request(struct list_head *head){
	RequestNode *node = NULL;
	if(list_empty(head)){
		printk(KERN_INFO "linber:: Dequeue Request Empty list\n");
	} else {
		node = list_first_entry(head, RequestNode , list);
		printk(KERN_INFO "linber:: Dequeue Request dequeued:%d\n", node->id);
		list_del(&node->list);
	}
	return node;
}
//------------------------------------------------------

static void print_service_list(struct list_head *head){
	struct list_head *p;
	ServiceNode *n;
	if(list_empty(head)){
		printk(KERN_INFO "linber:: Empty list\n");
	} else {
		list_for_each(p, head){
			n = list_entry(p, ServiceNode, list);
			printk(KERN_INFO "linber:: list element:%d\n", n->id);
		}
	}
}

static int Enqueue_Service(struct list_head *head, ServiceNode *node){
	list_add_tail(&node->list, head);
	printk(KERN_INFO "linber:: Enqueue Service:%d\n", node->id);
	print_service_list(head);
	return 0;
}

static int Dequeue_Service(struct list_head *head, ServiceNode *node){
	if(list_empty(head)){
		printk(KERN_INFO "linber:: Dequeue Service Empty list\n");
		return EMPTY_LIST;
	}

	node = list_first_entry(head, ServiceNode , list);
	printk(KERN_INFO "linber:: Dequeue Service dequeued:%d\n", node->id);

	kfree(node->uri);
	list_del(&node->list);

	print_service_list(head);

	return 0;
}

static ServiceNode* findService(char *uri){
	struct list_head *p;
	ServiceNode *n;
	if(!list_empty(&ServicesHead)){
		list_for_each(p, &ServicesHead){
			n = list_entry(p, ServiceNode, list);
			if(strcmp(n->uri, uri) == 0){
				return n;
			}
		}
	}
	return NULL;
}

//------------------------------------------------------

static int linber_create_service(char *uri){
	static int next_service_id = 0;
	ServiceNode *node;
	node = kmalloc(sizeof(*node) ,GFP_KERNEL);
	initialize_queue(&node->RequestsHead);
	node->id =next_service_id++;
	node->uri = uri;
	// initilize workers semaphore
	//	sema_init(&Workers, 1);

	Enqueue_Service(&ServicesHead, node);
	return 0;
}

//------------------------------------------------------
static int linber_register_service(linber_service_struct *obj){
	ServiceNode *node;
	printk(KERN_INFO "linber:: IOCTL Registration received, name:%s, exec_time:%u\n", obj->service_uri, obj->service_time);
	node = findService(obj->service_uri);
	if(node == NULL){
		linber_create_service(obj->service_uri);
	} else {
		printk(KERN_INFO "linber:: Service:%s already exists\n", obj->service_uri);
	}

	return 0;
}

static int linber_request_service(linber_service_struct *obj){
	static int id = 0;
	RequestNode *req_node;
	ServiceNode *ser_node;
	printk(KERN_INFO "linber:: IOCTL Request received, name:%s, exec_time:%u\n", obj->service_uri, obj->service_time);
	ser_node = findService(obj->service_uri);
	if(ser_node != NULL){
		req_node = Enqueue_Request(&ser_node->RequestsHead, id++);
		down_interruptible(&req_node->Request_sem);
	} else {
		printk(KERN_INFO "linber:: Service:%s does not exists\n", obj->service_uri);
	}
	return 0;
}

static int linber_Start_Job(linber_service_struct *obj){
	RequestNode *req_node;
	ServiceNode *node = findService(obj->service_uri);
	if(node != NULL){
		req_node = Dequeue_Request(&node->RequestsHead);
		if(req_node != NULL){
			printk(KERN_INFO "linber:: Serving Request %d, service: %s\n",req_node->id, obj->service_uri);
			up(&req_node->Request_sem);
		}
	} else {
		printk(KERN_INFO "linber:: Service:%s does not exists\n", obj->service_uri);
	}
	return 0;
}

/* called via ioctl when a worker finish to serve a request
*/
static int linber_End_Job(linber_service_struct *obj){
	printk(KERN_INFO "linber:: End job %s\n", obj->service_uri);
	return 0;
}

//-------------------------------------------------------------
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

		case IOCTL_START_JOB_SERVICE:
			linber_Start_Job(obj);
			break;

		case IOCTL_END_JOB_SERVICE:
			linber_End_Job(obj);
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

	initialize_queue(&ServicesHead);

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
