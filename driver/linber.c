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
#include <linux/random.h>

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


//------------------------------------------------------
typedef struct RequestNode{
	struct list_head list;
	int id;
	struct semaphore Request_sem;
	int cmd;
	char *ret;
	int ret_len;
}RequestNode;

typedef struct request_slot{
	RequestNode *request; // array serving requests, size max workers
	struct mutex slot_mutex;
}request_slot;


struct ServingRequests{
	request_slot *Serving_slots_arr; // array serving requests, size max workers
	struct mutex status_mutex;
	int serving_count;
};

typedef struct ServiceNode{
	struct list_head list;
	unsigned long id;
	char *uri;
	int max_concurrent_workers;
	int count_workers;
	int next_worker_id;
	int exec_time;
	int num_requests;
	int serving_time;
	int destroy_me;
	struct list_head RequestsHead;
	struct ServingRequests Serving_requests;
	struct semaphore Workers_sem;
	struct mutex service_mutex;
}ServiceNode;

static struct list_head ServicesHead;
unsigned int system_services_count;
unsigned int system_requests_count;
unsigned int system_serving_requests_count;
unsigned int system_max_concurrent_workers;
struct mutex System_mutex;

// ------------------------------------------------------
static int initialize_queue(struct list_head *head){
	INIT_LIST_HEAD(head);
	return 0;
}

static RequestNode *Enqueue_Request(struct list_head *head, int id){
	RequestNode *node;
	node = kmalloc(sizeof(*node) ,GFP_KERNEL);
	node->id = id;
	node->cmd = LINBER_ABORT_REQUEST;	// if not fixed will be aborted
	sema_init(&node->Request_sem, 0);
	list_add_tail(&node->list, head);
	return node;
}

static RequestNode *Dequeue_Request(struct list_head *head){
	RequestNode *node = NULL;
	if(list_empty(head)){
		;
	} else {
		node = list_first_entry(head, RequestNode , list);
		list_del(&node->list);
	}
	return node;
}
//------------------------------------------------------

static int Enqueue_Service(struct list_head *head, ServiceNode *node){
	list_add_tail(&node->list, head);
	printk(KERN_INFO "linber:: Enqueue Service:%lu\n", node->id);
	return 0;
}

static ServiceNode* findService(char *uri){
	ServiceNode *node;
	list_for_each_entry(node, &ServicesHead, list){
		if(strcmp(node->uri, uri) == 0){
			return node;
		}
	}
	return NULL;
}

void destroy_request(RequestNode *node_request){
	// set abort option
	node_request->cmd = LINBER_ABORT_REQUEST;
	up(&node_request->Request_sem);	// wake up client
	//	kfree(node_request); // done by the request_service
}

void destroy_service(ServiceNode *node_service){
	int i = 0;
	RequestNode *node_request, *next;
	mutex_lock(&node_service->service_mutex);
		list_for_each_entry_safe(node_request, next, &node_service->RequestsHead, list){
			i++;
			list_del(&node_request->list);
			destroy_request(node_request);
		}
		list_for_each_entry_safe_reverse(node_request, next, &node_service->RequestsHead, list){
			i++;
			list_del(&node_request->list);
			destroy_request(node_request);
		}
		system_requests_count -= i;
		for(i=0; i<node_service->count_workers; i++){
			up(&node_service->Workers_sem);
		}
		for(i=0; i<node_service->max_concurrent_workers; i++){
			mutex_unlock(&node_service->Serving_requests.Serving_slots_arr[i].slot_mutex);
		}
		node_service->destroy_me = 1;
		if(node_service->count_workers <= 0){
			system_services_count--;
			list_del(&node_service->list);
			kfree(node_service->uri);
			system_max_concurrent_workers -= node_service->max_concurrent_workers;
			kfree(node_service->Serving_requests.Serving_slots_arr);
			mutex_unlock(&node_service->service_mutex);
			kfree(node_service);	// done by last worker
		} else {
			mutex_unlock(&node_service->service_mutex);
		}
}

//------------------------------------------------------

static ServiceNode * linber_create_service(linber_service_struct *obj){
	int i = 0;
	ServiceNode *node = NULL;
	node = kmalloc(sizeof(*node) ,GFP_KERNEL);
	initialize_queue(&node->RequestsHead);
	get_random_bytes(&node->id, sizeof(node->id));
	node->uri = obj->service_uri;
	node->exec_time = obj->service_params.registration.exec_time;
	node->max_concurrent_workers = obj->service_params.registration.max_concurrent_workers;
	node->num_requests = 0;
	node->serving_time = 0;
	node->next_worker_id = 0;
	node->count_workers = 0;
	node->destroy_me = 0;
	node->Serving_requests.Serving_slots_arr = kmalloc(node->max_concurrent_workers*sizeof(request_slot), GFP_KERNEL);
	for(i=0; i<node->max_concurrent_workers; i++){
		mutex_init(&node->Serving_requests.Serving_slots_arr[i].slot_mutex);
	}
	node->Serving_requests.serving_count = 0;
	mutex_init(&node->Serving_requests.status_mutex);
	mutex_init(&node->service_mutex);
	sema_init(&node->Workers_sem, 0);
	Enqueue_Service(&ServicesHead, node);
	mutex_lock(&System_mutex);
		system_services_count++;
		system_max_concurrent_workers += node->max_concurrent_workers;
	mutex_unlock(&System_mutex);
	printk(KERN_INFO "linber:: New Service id:%lu %s, workers:%d,  exec time:%d\n", node->id, node->uri, node->max_concurrent_workers, node->exec_time);
	return node;
}

//------------------------------------------------------
static int linber_register_service(linber_service_struct *obj){
	ServiceNode *node;
	printk(KERN_INFO "linber:: IOCTL Registration received\n");
	node = findService(obj->service_uri);
	if(node == NULL){
		node = linber_create_service(obj);
		copy_to_user(obj->service_params.registration.ret_service_id, &node->id, sizeof(node->id));
	} else {
		printk(KERN_INFO "linber:: Service:%s already exists\n", obj->service_uri);
	}
	return 0;
}

static int linber_request_service(linber_service_struct *obj){
	int ret = 0;
	static int id = 0;
	RequestNode *req_node;
	ServiceNode *ser_node;
	ser_node = findService(obj->service_uri);
	if(ser_node != NULL && (ser_node->destroy_me == 0)){
		mutex_lock(&ser_node->service_mutex);
			req_node = Enqueue_Request(&ser_node->RequestsHead, id++);
			ser_node->num_requests++;
			ser_node->serving_time += ser_node->exec_time;
			up(&ser_node->Workers_sem);
		mutex_unlock(&ser_node->service_mutex);
		mutex_lock(&System_mutex);
			system_requests_count++;
		mutex_unlock(&System_mutex);
		down_interruptible(&req_node->Request_sem);
		// request completed or aborted
		ret = req_node->cmd;
		if(ret != LINBER_ABORT_REQUEST){
			// copy result
		}
		kfree(req_node);
	} else {
		printk(KERN_INFO "linber:: Request Service:%s does not exists\n", obj->service_uri);
		ret = LINBER_ABORT_REQUEST;
	}
	return ret;
}


static int linber_register_service_worker(linber_service_struct *obj){
	ServiceNode *ser_node = findService(obj->service_uri);
	int worker_id;
	if(ser_node != NULL){
		mutex_lock(&ser_node->service_mutex);
			ser_node->count_workers++;
			worker_id = ser_node->next_worker_id;
			ser_node->next_worker_id = (ser_node->next_worker_id + 1)%ser_node->max_concurrent_workers;
			copy_to_user(obj->service_params.register_worker.ret_worker_id, &worker_id, sizeof(worker_id));
		mutex_unlock(&ser_node->service_mutex);
	}
	return 0;
}

static int linber_service_Worker_check_id(ServiceNode *ser_node, unsigned long service_id, int worker_id){
	if((ser_node->id == service_id) && (worker_id >= 0 && worker_id <ser_node->max_concurrent_workers)){
		return 1;
	}
	return 0;
}


static int linber_Start_Job(linber_service_struct *obj){
	RequestNode *req_node;
	ServiceNode *ser_node = findService(obj->service_uri);
	int worker_id;
	unsigned long service_id;
	if(ser_node != NULL){
		// check arguments
		worker_id = obj->service_params.start_job.worker_id;
		service_id = obj->service_params.start_job.service_id;
		if(!linber_service_Worker_check_id(ser_node, service_id, worker_id)){
			printk(KERN_INFO "linber:: Wrong service id %lu or worker id %d for service: %s\n", service_id, worker_id, obj->service_uri);
			return LINBER_KILL_WORKER;
		}

		down_interruptible(&ser_node->Workers_sem);		// wait for request
		mutex_lock(&ser_node->service_mutex);
			if(ser_node->destroy_me == 1){	// check if must e destroyied
				ser_node->count_workers--;
				if(ser_node->count_workers <= 0){
					system_services_count--;
					list_del(&ser_node->list);
					kfree(ser_node->uri);
					system_max_concurrent_workers -= ser_node->max_concurrent_workers;
					kfree(ser_node->Serving_requests.Serving_slots_arr);
					mutex_unlock(&ser_node->service_mutex);
					kfree(ser_node);	// done by last worker
					printk(KERN_INFO "linber:: Service destroyied\n");
				} else {
					mutex_unlock(&ser_node->service_mutex);
				}
				return LINBER_KILL_WORKER;
			}
		mutex_unlock(&ser_node->service_mutex);

		mutex_lock(&ser_node->Serving_requests.Serving_slots_arr[worker_id].slot_mutex);	// block until it ir sure that can execute the request
			mutex_lock(&ser_node->service_mutex);
				req_node = Dequeue_Request(&ser_node->RequestsHead);
				if(req_node != NULL){
					ser_node->num_requests--;
					ser_node->serving_time -= ser_node->exec_time;
				}
			mutex_unlock(&ser_node->service_mutex);

			if(req_node == NULL){
				printk(KERN_INFO "linber:: No pending request for service: %s\n", obj->service_uri);
				mutex_unlock(&ser_node->Serving_requests.Serving_slots_arr[worker_id].slot_mutex);	// release slot
				return LINBER_SKIP_JOB;
			} else {
				ser_node->Serving_requests.Serving_slots_arr[worker_id].request = req_node;
				mutex_lock(&ser_node->Serving_requests.status_mutex);
					ser_node->Serving_requests.serving_count++;
				mutex_unlock(&ser_node->Serving_requests.status_mutex);

				mutex_lock(&System_mutex);
					system_requests_count--;
					system_serving_requests_count++;
				mutex_unlock(&System_mutex);

				// pass parameters to worker and exec job
			}
	} else {
		printk(KERN_INFO "linber::Start Job  Service:%s does not exists\n", obj->service_uri);
		return LINBER_KILL_WORKER;
	}
	return 0;
}

	// find request in serving_requests
	// copy ret to client userspace
	// wake up client
	// destroy request structure
static int linber_End_Job(linber_service_struct *obj){
	RequestNode *req_node;
	ServiceNode *ser_node = findService(obj->service_uri);
	int worker_id;
	unsigned long service_id;
	if(ser_node != NULL){
		worker_id = obj->service_params.end_job.worker_id;
		service_id = obj->service_params.end_job.service_id;
		if(!linber_service_Worker_check_id(ser_node, service_id, worker_id)){
			printk(KERN_INFO "linber:: Wrong service id %lu or worker id %d for service: %s\n", service_id,  worker_id, obj->service_uri);
			return LINBER_KILL_WORKER;
		} else {
			req_node = ser_node->Serving_requests.Serving_slots_arr[worker_id].request;
			if(req_node != NULL){
				ser_node->Serving_requests.Serving_slots_arr[worker_id].request = NULL;
				mutex_lock(&ser_node->Serving_requests.status_mutex);
					ser_node->Serving_requests.serving_count--;
				mutex_unlock(&ser_node->Serving_requests.status_mutex);
				mutex_lock(&System_mutex);
					system_serving_requests_count--;
				mutex_unlock(&System_mutex);
				req_node->cmd = LINBER_SUCCESS_REQUEST;

				// copy service return in the request ret

				up(&req_node->Request_sem);	// end job
				mutex_unlock(&ser_node->Serving_requests.Serving_slots_arr[worker_id].slot_mutex);	// release slot
			} else {
				printk(KERN_INFO "linber:: End job spourious job %s\n", obj->service_uri);
				return LINBER_SKIP_JOB;
			}
		}
	}
	return 0;
}

static int linber_destroy_service(linber_service_struct *obj){
	ServiceNode *node;
	node = findService(obj->service_uri);
	if(node != NULL){
		if(linber_service_Worker_check_id(node, obj->service_params.destroy_service.service_id, 0)){
			printk(KERN_INFO "linber:: destroying service %s\n", obj->service_uri);
			destroy_service(node);
			return 0;
		}
	}
	return -1;
}

static int linber_get_system_status(void* system_user){
	system_status system;
	service_status *services = kmalloc(MAX_SERVICES * sizeof(service_status), GFP_KERNEL);
	ServiceNode *ser_node;
	int i = 0;

	copy_from_user(&system, system_user, sizeof(system_status));
	mutex_lock(&System_mutex);
	system.max_concurrent_workers = system_max_concurrent_workers;
	system.requests_count = system_requests_count;
	system.serving_requests_count = system_serving_requests_count;
	system.services_count = system_services_count;
	if(system_services_count > 0){
		copy_from_user(services, system.services, MAX_SERVICES * sizeof(service_status));
		list_for_each_entry(ser_node, &ServicesHead, list){
			if(i < system_services_count){
				services[i].uri_len = strlen(ser_node->uri);
				copy_to_user(services[i].uri, ser_node->uri, services[i].uri_len);
				services[i].id = ser_node->id;
				services[i].max_concurrent_workers = ser_node->max_concurrent_workers;
				services[i].requests_count = ser_node->num_requests;
				services[i].serving_time = ser_node->serving_time;
				services[i].serving_requests_count = ser_node->Serving_requests.serving_count;
			}
			i++;
		}
	}
	mutex_unlock(&System_mutex);
	copy_to_user(system_user, &system, sizeof(system_status));
	copy_to_user(system.services, services, system_services_count * sizeof(service_status));
	kfree(services);
	return 0;
}


//-------------------------------------------------------------
static long		linber_ioctl(struct file *f, unsigned int cmd, unsigned long ioctl_param){ 
	int ret;
	char *uri_tmp;
	linber_service_struct *obj;
	if(cmd != IOCTL_SYSTEM_STATUS){
		obj = kmalloc(sizeof(linber_service_struct), GFP_KERNEL);
		copy_from_user((void*)obj, (void*)ioctl_param, sizeof(linber_service_struct));
		uri_tmp = kmalloc(obj->service_uri_len + 1, GFP_KERNEL);
		copy_from_user((void*)uri_tmp, (void*)obj->service_uri, obj->service_uri_len);
		uri_tmp[obj->service_uri_len] = '\0';
		obj->service_uri = uri_tmp;
	}

	switch(cmd){
		case IOCTL_REGISTER_SERVICE:
			ret = linber_register_service(obj);
			break;

		case IOCTL_REQUEST_SERVICE:
			//copy from user params
			ret = linber_request_service(obj);
			break;

		case IOCTL_REGISTER_SERVICE_WORKER:
			ret = linber_register_service_worker(obj);
			break;

		case IOCTL_START_JOB_SERVICE:
			ret = linber_Start_Job(obj);
			break;

		case IOCTL_END_JOB_SERVICE:
			// copy from user ret
			ret = linber_End_Job(obj);
			break;

		case IOCTL_DESTROY_SERVICE:
			ret = linber_destroy_service(obj);
			break;

		case IOCTL_SYSTEM_STATUS:
			ret = linber_get_system_status((void*)ioctl_param);
			break;

		default:
			printk(KERN_ERR "linber:: IOCTL operation %d not implemented\n", cmd);
			ret = -1;
			break;
	}
	if(cmd != IOCTL_SYSTEM_STATUS){
		kfree(obj);
	}
	return ret;
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
	system_requests_count = 0;
	system_serving_requests_count = 0;
	system_max_concurrent_workers = 0;
	mutex_init(&System_mutex);

	return 0;
}

void cleanup_module(void) {
	ServiceNode *node_service, *next;
	list_for_each_entry_safe(node_service, next, &ServicesHead, list){
		list_del(&node_service->list);
		destroy_service(node_service);

	}
	list_for_each_entry_safe_reverse(node_service, next, &ServicesHead, list){
		list_del(&node_service->list);
		destroy_service(node_service);
	}

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
