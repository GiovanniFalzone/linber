/*
*/
#include <linux/kernel.h>	/* Needed for KERN_ERR */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/idr.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#include "../libs/linber_ioctl.h"

#define DEV_NAME DEVICE_FILE_NAME
#define CLA_NAME "linberclass"

#define checkMemoryError(value) if(value!=0){return LINBER_USER_MEMORY_ERROR;}


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
	struct semaphore Request_sem;
	unsigned long token;
	int result_cmd;
	int request_len;
	int response_len;
	bool request_accepted;
	bool response_shm_mode;
	bool request_shm_mode;
	union {
		char * data;
		key_t shm_key;
	} request;
	union {
		char * data;
		key_t shm_key;
	} response;
}RequestNode;

typedef struct request_slot{
	RequestNode *request; // array serving requests, size max workers
	struct task_struct *worker;
	struct mutex slot_mutex;
}request_slot;


struct ServingRequests{
	request_slot *Serving_slots_arr; // array serving requests, size max workers
	struct mutex Serving_mutex;
	unsigned int num_active_slots;
	unsigned int max_slots;
	unsigned int minimum_slots;
	unsigned int serving_count;
};

typedef struct ServiceNode{
	int id;
	struct list_head list;
	unsigned long token;
	char *uri;
	int exec_time;
	int Request_count;
	int serving_time;
	bool destroy_me;

	struct Workers{
		int parallelism;
		int count;
		int next_id;
		struct semaphore sem;
	} Workers;

	struct CBS{
		int max_bandwidth;
		struct semaphore sem_Q;
	} CBS_server;

	struct list_head RequestsHead;
	struct list_head Completed_RequestsHead;
	struct ServingRequests Serving_requests;
	struct mutex service_mutex;
}ServiceNode;

struct linber_stuct{
	struct task_struct *linber_manager;
	struct list_head Services;
	struct idr Services_idr;
	unsigned int Services_count;
	unsigned int Request_count;
	unsigned int Serving_count;
	unsigned int Max_Working;
	struct mutex mutex;
} linber;

//-----------------------------------------------
static void CBS_init(ServiceNode *ser_node){
	//-----------------------------------------
	ser_node->CBS_server.max_bandwidth = 4;
	sema_init(&ser_node->CBS_server.sem_Q, ser_node->CBS_server.max_bandwidth);
	//-----------------------------------------
}

static inline void CBS_refill(ServiceNode *ser_node){
	int i=0;
	mutex_lock(&ser_node->service_mutex);
	printk(KERN_INFO "Linber:: ------------Refill for %s------------------\n", ser_node->uri);
	while(	(ser_node->CBS_server.sem_Q.count < ser_node->CBS_server.max_bandwidth) && 	\
			(i++ < ser_node->CBS_server.max_bandwidth)){
		up(&ser_node->CBS_server.sem_Q);
	}
	mutex_unlock(&ser_node->service_mutex);
}


//-------------------------
// check CBS Server bandwidth
// if enough to execute reduce bandwith and proceed serving the request
// else wait for refill
//--------------------------
static inline void CBS_job_check(ServiceNode *ser_node){
	down(&ser_node->CBS_server.sem_Q);
	current->prio = -19;	// max priority
	printk(KERN_INFO "Linber:: Wroker for %s working\n", ser_node->uri);
}


//-------------------------
// if no budget then lower priority to avoid that a worker of a service
// can steal execution time to the others, accuracy 10%
//-------------------------
void CBS_check_bandwidth(ServiceNode *ser_node){
	int i;
	struct task_struct *worker;
	if(ser_node->CBS_server.sem_Q.count <= 0){
		for(i=0; i<ser_node->Serving_requests.max_slots; i++){
//			worker = ser_node->Serving_requests.Serving_slots_arr[i].worker;
			if(worker != NULL){
				//			
			}
		}
	}
}

//-------------------------
// linber_thread is RT thread and every P millis refill the semaphore to max bandwidth
// the task is running e.g at P/10 period and every time check the level of each service
// if the bandwith is empty then reduce the priority of each worker for that service to MIN_PRIO
// at P or every 10*(thread_period = P/10) a refill occour for each service and the workers receive a MAX_PRIO
// so at most Q max bandiwth request can be served in P period
//-------------------------
static int linber_manager_job(void* args){
	static int job_count = 1;
	ServiceNode *ser_node = NULL;
	while(!kthread_should_stop()){
		mutex_lock(&linber.mutex);
		list_for_each_entry(ser_node, &linber.Services, list){
			if(job_count == 0){
				CBS_refill(ser_node);
			} else {
				CBS_check_bandwidth(ser_node);
			}
		}
		mutex_unlock(&linber.mutex);
		job_count = (job_count + 1)%10;
		msleep(100);
	}
	return 0;
}

static void linber_manager_init(void){
	printk(KERN_INFO "Linber:: Creating Linber task manager\n");
	linber.linber_manager = kthread_create(linber_manager_job, NULL, "linber_manager");
	if(linber.linber_manager){
		printk(KERN_INFO "Linber:: Manager Created Sucessfully\n");
		wake_up_process(linber.linber_manager);
	} else {
		printk(KERN_ALERT "Linber:: Thread Creation Failed\n");
	}
}
//------------------------------------------------------


// ------------------------------------------------------
static inline int initialize_queue(struct list_head *head){
	INIT_LIST_HEAD(head);
	return 0;
}

static inline RequestNode* create_Request(void){
	RequestNode *req_node;
	req_node = kmalloc(sizeof(*req_node) ,GFP_KERNEL);
	req_node->request_accepted = false;
	req_node->result_cmd = LINBER_ABORT_REQUEST;	// if not fixed will be aborted	
	get_random_bytes(&req_node->token, sizeof(req_node->token));
	sema_init(&req_node->Request_sem, 0);
	return req_node;
}

static RequestNode *Dequeue_Request(struct list_head *head){
	RequestNode *req_node = NULL;
	if(list_empty(head)){
		;
	} else {
		req_node = list_first_entry(head, RequestNode , list);
		list_del(&req_node->list);
	}
	return req_node;
}
//------------------------------------------------------

static ServiceNode* findService_by_id(int id){
	ServiceNode *ser_node = NULL;
	mutex_lock(&linber.mutex);
	ser_node = idr_find(&linber.Services_idr, id);
	mutex_unlock(&linber.mutex);
	return ser_node;
}

static ServiceNode* findService(char *uri){
	ServiceNode *ser_node = NULL;
	mutex_lock(&linber.mutex);
	list_for_each_entry(ser_node, &linber.Services, list){
		if(strcmp(ser_node->uri, uri) == 0){
			mutex_unlock(&linber.mutex);
			return ser_node;
		}
	}
	mutex_unlock(&linber.mutex);
	return NULL;
}

static RequestNode* findWaiting_or_Completed_Request(ServiceNode* ser_node, unsigned long token){
	RequestNode *req_node = NULL, *node = NULL;
	int i = 0;
	mutex_lock(&ser_node->service_mutex);
	// search in waiting
	list_for_each_entry(node, &linber.Services, list){
		if(node->token == token){
			req_node = node;
			mutex_unlock(&ser_node->service_mutex);
			return req_node;
		}
	}
	// search in serving
	if(req_node == NULL){
		for(i=0; i<ser_node->Serving_requests.max_slots; i++){
			node = ser_node->Serving_requests.Serving_slots_arr[i].request;
			if((node != NULL) && (node->token == token)){
				mutex_unlock(&ser_node->service_mutex);
				return req_node;
			}
		}		
	}
	// search in completed
	if(req_node == NULL){
		list_for_each_entry(node, &ser_node->Completed_RequestsHead, list){
			if(node->token == token){	
				req_node = node;
				mutex_unlock(&ser_node->service_mutex);
				return req_node;
			}
		}
	}
	mutex_unlock(&ser_node->service_mutex);
	return req_node;
}

static inline void destroy_request(RequestNode *req_node){
	req_node->result_cmd = LINBER_ABORT_REQUEST;
	up(&req_node->Request_sem);	// wake up client
}

static inline void clean_service(ServiceNode *ser_node){
	linber.Services_count--;
	list_del(&ser_node->list);
	kfree(ser_node->uri);
	linber.Max_Working -= ser_node->Workers.parallelism;
	kfree(ser_node->Serving_requests.Serving_slots_arr);
	mutex_unlock(&ser_node->service_mutex);
	idr_remove(&linber.Services_idr, ser_node->id);
	kfree(ser_node);	// done by last worker
}

static void destroy_service(ServiceNode *ser_node){
	int i = 0;
	RequestNode *req_node, *next;
	printk(KERN_INFO "linber:: Destroying service %s\n", ser_node->uri);
	mutex_lock(&ser_node->service_mutex);
	list_for_each_entry_safe(req_node, next, &ser_node->RequestsHead, list){
		i++;
		list_del(&req_node->list);
		destroy_request(req_node);
	}
	linber.Request_count -= i;
	
	list_for_each_entry_safe(req_node, next, &ser_node->Completed_RequestsHead, list){
		list_del(&req_node->list);
		destroy_request(req_node);
	}

	for(i=0; i<ser_node->Workers.count; i++){
		up(&ser_node->Workers.sem);
	}
	for(i=0; i<ser_node->Workers.parallelism; i++){
		mutex_unlock(&ser_node->Serving_requests.Serving_slots_arr[i].slot_mutex);
	}

	ser_node->destroy_me = true;

	if(ser_node->Workers.count <= 0){
		clean_service(ser_node);
	} else {
		mutex_unlock(&ser_node->service_mutex);
	}
}


//-----------------------------------------------
static ServiceNode * linber_create_service(linber_service_struct *obj){
	int id = 0;
	ServiceNode *ser_node = NULL;
	ser_node = kmalloc(sizeof(*ser_node) ,GFP_KERNEL);
	initialize_queue(&ser_node->Completed_RequestsHead);
	initialize_queue(&ser_node->RequestsHead);
	get_random_bytes(&ser_node->token, sizeof(ser_node->token));
	ser_node->uri = obj->service_uri;
	ser_node->exec_time = obj->op_params.registration.exec_time;
	ser_node->Workers.parallelism = obj->op_params.registration.Max_Working;
	ser_node->Request_count = 0;
	ser_node->serving_time = 0;
	ser_node->Workers.next_id = 0;
	ser_node->Workers.count = 0;
	ser_node->destroy_me = false;
	ser_node->Serving_requests.serving_count = 0;
	ser_node->Serving_requests.max_slots = ser_node->Workers.parallelism;
	ser_node->Serving_requests.minimum_slots = 1;
	ser_node->Serving_requests.num_active_slots = ser_node->Serving_requests.minimum_slots;
	ser_node->Serving_requests.Serving_slots_arr = kmalloc(ser_node->Serving_requests.max_slots*sizeof(request_slot), GFP_KERNEL);

	CBS_init(ser_node);

	for(id=0; id<ser_node->Serving_requests.max_slots; id++){
		mutex_init(&ser_node->Serving_requests.Serving_slots_arr[id].slot_mutex);
		ser_node->Serving_requests.Serving_slots_arr[id].request = NULL;
	}

	mutex_init(&ser_node->Serving_requests.Serving_mutex);
	mutex_init(&ser_node->service_mutex);
	sema_init(&ser_node->Workers.sem, 0);
	mutex_lock(&linber.mutex);
		list_add_tail(&ser_node->list, &linber.Services);
		id = idr_alloc(&linber.Services_idr, (void*)ser_node, 0, MAX_SERVICES, GFP_KERNEL);
		if(id < 0){
			kfree(ser_node);
			return NULL;
		}
		linber.Services_count++;
		linber.Max_Working += ser_node->Workers.parallelism;
	mutex_unlock(&linber.mutex);
	ser_node->id = id;
	printk(KERN_INFO "linber::  New Service token:%lu %s, workers:%d,  exec time:%d\n", ser_node->token, ser_node->uri, ser_node->Workers.parallelism, ser_node->exec_time);
	return ser_node;
}

static RequestNode* enqueue_request_for_service(ServiceNode *ser_node, char *request, unsigned int request_len, bool blocking, bool shm_mode, key_t req_key){
	RequestNode *req_node;
	req_node = create_Request();
	if(req_node != NULL){
		if(shm_mode){
			req_node->request_shm_mode = true;
			req_node->request.shm_key = req_key;
		} else {
			req_node->request_shm_mode = false;
			req_node->request.data = request;
		}
		req_node->request_len = request_len;

		mutex_lock(&ser_node->service_mutex);
			ser_node->Request_count++;
			ser_node->serving_time += ser_node->exec_time;
			list_add_tail(&req_node->list, &ser_node->RequestsHead);
			mutex_lock(&linber.mutex);
				linber.Request_count++;
			mutex_unlock(&linber.mutex);
		mutex_unlock(&ser_node->service_mutex);

		req_node->request_accepted = true;
		up(&ser_node->Workers.sem);
		if(blocking){
			down(&req_node->Request_sem);
		}
	}
	return req_node;
}

static void enqueue_request_completed(ServiceNode *ser_node, RequestNode *req_node){	
	mutex_lock(&ser_node->service_mutex);
		list_add_tail(&req_node->list, &ser_node->Completed_RequestsHead);
	mutex_unlock(&ser_node->service_mutex);
}

static RequestNode* get_Completed_Request(ServiceNode *ser_node, unsigned long token){
	RequestNode *req_node = NULL, *next;
	mutex_lock(&ser_node->service_mutex);
	list_for_each_entry_safe(req_node, next, &ser_node->Completed_RequestsHead, list){
		if(req_node->token == token){	
			list_del(&req_node->list);
			break;
		}
	}
	mutex_unlock(&ser_node->service_mutex);
	return req_node;
}

static RequestNode* get_request(ServiceNode* ser_node){
	RequestNode* req_node = NULL;
	mutex_lock(&ser_node->service_mutex);
		req_node = Dequeue_Request(&ser_node->RequestsHead);
		if(req_node != NULL){
			ser_node->Request_count--;
			ser_node->serving_time -= ser_node->exec_time;
			if(!req_node->request_accepted){
				kfree(req_node->request.data);
				kfree(req_node->response.data);
				kfree(req_node);
				mutex_unlock(&ser_node->service_mutex);
				return NULL;
			}
		}
	mutex_unlock(&ser_node->service_mutex);
	if(req_node == NULL){
		printk(KERN_INFO "linber::  No pending request for service: %s\n", ser_node->uri);
	}
	return req_node;
}

static request_slot* get_slot_by_id(ServiceNode *ser_node, unsigned int slot_id){
	request_slot *slot = NULL;
	if(slot_id >=0 && slot_id < ser_node->Serving_requests.max_slots){
		slot = &(ser_node->Serving_requests.Serving_slots_arr[slot_id]);;
	} else {
		printk(KERN_ERR "linber::  Wrong slot id %d for service: %s\n", slot_id, ser_node->uri);
	}
	return slot;
}

static unsigned int get_slot(ServiceNode *ser_node, unsigned int worker_id, request_slot **slot){
	unsigned int slot_id = 0;
	slot_id = worker_id % (ser_node->Serving_requests.num_active_slots);
	*slot = &(ser_node->Serving_requests.Serving_slots_arr[slot_id]);
	return slot_id;
}

static inline void decrease_workers(ServiceNode *ser_node){
	mutex_lock(&ser_node->service_mutex);
	ser_node->Workers.count--;
	mutex_unlock(&ser_node->service_mutex);
}

static int Service_check_Worker(ServiceNode *ser_node, unsigned long service_token, int worker_id){
	if((ser_node->token == service_token) && (worker_id >= 0)){
		return 1;
	}
	printk(KERN_ERR "linber:: Wrong service id %lu or worker id %d for service: %s\n", service_token, worker_id, ser_node->uri);
	return 0;
}

static int Service_check_alive(ServiceNode *ser_node){
	mutex_lock(&ser_node->service_mutex);
		if(ser_node->destroy_me){	// check if must be destroied
			ser_node->Workers.count--;
			if(ser_node->Workers.count <= 0){
				clean_service(ser_node);
				return -1;
			} else {
				mutex_unlock(&ser_node->service_mutex);
				return -1;
			}
		}
	mutex_unlock(&ser_node->service_mutex);
	return 0;
}

static int get_and_load_request_to_slot(ServiceNode *ser_node, RequestNode **req_node_ptr, request_slot **slot_ptr, unsigned int *slot_id, unsigned int worker_id){
	request_slot *slot;
	RequestNode *req_node;
	down_interruptible(&ser_node->Workers.sem);		// wait for request

	if(Service_check_alive(ser_node) < 0){
		return -1;
	}

	*slot_id = get_slot(ser_node, worker_id, &slot);
	*slot_ptr = slot;
	mutex_lock(&slot->slot_mutex);	// block until it is sure that can execute the request
	req_node = get_request(ser_node);
	*req_node_ptr = req_node;
	if(req_node != NULL){
		slot->worker = current;
		CBS_job_check(ser_node);

		slot->request = req_node;
		mutex_lock(&ser_node->Serving_requests.Serving_mutex);
			ser_node->Serving_requests.serving_count++;
			mutex_lock(&linber.mutex);
				linber.Request_count--;
				linber.Serving_count++;
			mutex_unlock(&linber.mutex);
		mutex_unlock(&ser_node->Serving_requests.Serving_mutex);
	}
	return 0;
}

static void unload_slot_and_get_request(ServiceNode *ser_node, RequestNode **req_node_ptr, request_slot **slot_ptr, unsigned int slot_id){
	RequestNode *req_node;
	request_slot *slot;
	slot = get_slot_by_id(ser_node, slot_id);
	*slot_ptr = slot;
	if(slot != NULL){
		req_node = slot->request;
		*req_node_ptr = req_node;
		if(req_node != NULL){
			slot->worker = NULL;
			slot->request = NULL;
			mutex_lock(&ser_node->Serving_requests.Serving_mutex);
				ser_node->Serving_requests.serving_count--;
				mutex_lock(&linber.mutex);
					linber.Serving_count--;
				mutex_unlock(&linber.mutex);
			mutex_unlock(&ser_node->Serving_requests.Serving_mutex);
			mutex_unlock(&slot->slot_mutex);	// release slot
		}
	}
}

static inline void service_load_balancing(ServiceNode *ser_node){
	if(ser_node->Request_count > ser_node->Serving_requests.num_active_slots){
		ser_node->Serving_requests.num_active_slots++;
		if(ser_node->Serving_requests.num_active_slots > ser_node->Serving_requests.max_slots){
			ser_node->Serving_requests.num_active_slots = ser_node->Serving_requests.max_slots;
		}
	} else {
		ser_node->Serving_requests.num_active_slots--;
		if(ser_node->Serving_requests.num_active_slots < ser_node->Serving_requests.minimum_slots){
			ser_node->Serving_requests.num_active_slots = ser_node->Serving_requests.minimum_slots;
		}
	}
}

//------------------------------------------------------
static int linber_register_service(linber_service_struct *obj){
	ServiceNode *ser_node;
	printk(KERN_INFO "linber:: IOCTL Registration received\n");
	ser_node = findService(obj->service_uri);
	if(ser_node == NULL){
		ser_node = linber_create_service(obj);
		if(ser_node == NULL){
			return LINBER_SERVICE_REGISTRATION_FAIL;
		}
		checkMemoryError(put_user(ser_node->token, obj->op_params.registration.ptr_service_token));
		checkMemoryError(put_user(ser_node->id, obj->op_params.registration.ptr_service_id));
		return LINBER_SERVICE_REGISTRATION_SUCCESS;
	} else {
		return LINBER_SERVICE_REGISTRATION_ALREADY_EXISTS;
	}
}

static int linber_request_service(linber_service_struct *obj){
	RequestNode *req_node = NULL;
	ServiceNode *ser_node = NULL;
	int ret = LINBER_REQUEST_FAILED, status;
	char *request = NULL;
	key_t request_key = 0;
	unsigned int request_len;
	unsigned long token;
	bool blocking, shm_mode;

	ser_node = findService(obj->service_uri);
	if(ser_node != NULL && (!ser_node->destroy_me)){
		status = obj->op_params.request.status;
		request_len = obj->op_params.request.request_len;
		shm_mode = (obj->op_params.request.request_shm_mode == 1);
		blocking = (obj->op_params.request.blocking == 1);

		if(!shm_mode && (request_len > LINBER_MAX_MEM_SIZE)){
			return LINBER_PAYLOAD_SIZE_ERROR;
		}

		//----------------------init-----------------------
		if(status == LINBER_REQUEST_INIT){
			service_load_balancing(ser_node);
			if(!shm_mode){
				request = kmalloc(request_len, GFP_KERNEL);
				checkMemoryError(copy_from_user(request, obj->op_params.request.request.data, request_len));
			} else {
				request_key = obj->op_params.request.request.shm_key;
			}
			req_node = enqueue_request_for_service(ser_node, request, request_len, blocking, shm_mode, request_key);
			if(req_node == NULL){
				return LINBER_REQUEST_FAILED;
			}
			checkMemoryError(put_user(req_node->token, obj->op_params.request.ptr_token));
			status = LINBER_REQUEST_COMPLETED;
			if(!blocking){
				return LINBER_REQUEST_WAITING;
			}
		}
		//----------------------waiting-----------------------
		if(status == LINBER_REQUEST_WAITING){
			checkMemoryError(get_user(token, obj->op_params.request.ptr_token));
			req_node = findWaiting_or_Completed_Request(ser_node, token);
			if(req_node == NULL){
				return LINBER_REQUEST_FAILED;
			}
			down(&req_node->Request_sem);
			status = LINBER_REQUEST_COMPLETED;
		}
		//----------------------completed-----------------------
		if(status == LINBER_REQUEST_COMPLETED){
			if(req_node != NULL){
				ret = req_node->result_cmd;
				if(ret == LINBER_REQUEST_SUCCESS){	// copy response
					checkMemoryError(put_user(req_node->response_len, obj->op_params.request.ptr_response_len));
					checkMemoryError(put_user(req_node->response_shm_mode, obj->op_params.request.ptr_response_shm_mode));
				}
			}
		}
	} else {
		ret = LINBER_SERVICE_NOT_EXISTS;
	}
	return ret;
}


static int linber_request_service_get_response(linber_service_struct *obj){
	int ret = 0;
	unsigned long token;
	RequestNode *req_node;
	ServiceNode *ser_node;
	ser_node = findService(obj->service_uri);
	if((ser_node != NULL) && (!ser_node->destroy_me)){
		checkMemoryError(get_user(token, obj->op_params.request.ptr_token));
		req_node = get_Completed_Request(ser_node, token);
		if(req_node != NULL){
			ret = req_node->result_cmd;
			if(req_node->response_shm_mode){	// response in shared memory
				checkMemoryError(put_user(req_node->response.shm_key, obj->op_params.request.ptr_shm_response_key));
			} else {
				checkMemoryError(copy_to_user(obj->op_params.request.ptr_response, req_node->response.data, req_node->response_len));
				kfree(req_node->response.data);
			}
			if(!req_node->request_shm_mode){
				kfree(req_node->request.data);
			}
			kfree(req_node);
		} else {
			ret = LINBER_REQUEST_FAILED;
		}
	} else {
		ret = LINBER_SERVICE_NOT_EXISTS;
	}
	return ret;
}


static int linber_register_service_worker(linber_service_struct *obj){
	ServiceNode *ser_node = findService(obj->service_uri);
	int worker_id;
	if(ser_node != NULL){
		if(!Service_check_Worker(ser_node, obj->op_params.register_worker.service_token, 0)){
			return -1;
		}
		mutex_lock(&ser_node->service_mutex);
			ser_node->Workers.count++;
			worker_id = ser_node->Workers.next_id++;
		mutex_unlock(&ser_node->service_mutex);
		checkMemoryError(put_user(worker_id, obj->op_params.register_worker.ptr_worker_id));
	}
	return 0;
}

static int linber_Start_Job(linber_service_struct *obj){
	request_slot *slot;
	RequestNode *req_node;
	ServiceNode *ser_node = findService_by_id(obj->op_params.start_job.service_id);
	unsigned int worker_id, slot_id;
	unsigned long service_token;
	if(ser_node != NULL){
		// check arguments
		worker_id = obj->op_params.start_job.worker_id;
		service_token = obj->op_params.start_job.service_token;
		if(!Service_check_Worker(ser_node, service_token, worker_id)){
			decrease_workers(ser_node);
			printk(KERN_ERR "linber:: Service Check Worker failded token:%lu id:%u\n", service_token, worker_id);
			return LINBER_KILL_WORKER;
		}
		if(get_and_load_request_to_slot(ser_node, &req_node, &slot, &slot_id, worker_id) < 0){
			printk(KERN_ERR "linber:: get slot failded id:%u\n", worker_id);
			return LINBER_KILL_WORKER;
		}
		if(req_node == NULL){
			mutex_unlock(&slot->slot_mutex);	// release slot
			return LINBER_SERVICE_SKIP_JOB;
		} else {
			checkMemoryError(put_user(slot_id, obj->op_params.start_job.ptr_slot_id));
			checkMemoryError(put_user(req_node->request_len, obj->op_params.start_job.ptr_request_len));
			checkMemoryError(put_user(req_node->request_shm_mode, obj->op_params.start_job.ptr_request_shm_mode));
			if(req_node->request_shm_mode){
				checkMemoryError(put_user(req_node->request.shm_key, obj->op_params.start_job.data.ptr_request_key));
			}
		}
	} else {
		decrease_workers(ser_node);
		return LINBER_SERVICE_NOT_EXISTS;
	}
	return 0;
}

static int linber_Start_Job_get_request(linber_service_struct *obj){
	request_slot *slot;
	RequestNode *req_node;
	ServiceNode *ser_node = findService_by_id(obj->op_params.start_job.service_id);
	unsigned int worker_id, slot_id;
	unsigned long service_token;
	if(ser_node != NULL){
		// check arguments
		worker_id = obj->op_params.start_job.worker_id;
		service_token = obj->op_params.start_job.service_token;
		if(!Service_check_Worker(ser_node, service_token, worker_id)){
			decrease_workers(ser_node);
			printk(KERN_ERR "linber:: Service Check Worker failded token:%lu id:%u\n", service_token, worker_id);
			return LINBER_KILL_WORKER;
		}
		checkMemoryError(get_user(slot_id, obj->op_params.start_job.ptr_slot_id));
		slot = get_slot_by_id(ser_node, slot_id);
		req_node = slot->request;
		if(slot == NULL){
			printk(KERN_ERR "linber:: get slot failded id:%u\n", worker_id);
			return LINBER_KILL_WORKER;
		}
		if(req_node == NULL){
			mutex_unlock(&slot->slot_mutex);	// release slot
			return LINBER_SERVICE_SKIP_JOB;
		} else {
			// pass parameters to worker and exec job
			checkMemoryError(copy_to_user(obj->op_params.start_job.data.ptr_request, req_node->request.data, req_node->request_len));
		}
	} else {
		decrease_workers(ser_node);
		return LINBER_SERVICE_NOT_EXISTS;
	}
	return 0;
}


static int linber_End_Job(linber_service_struct *obj){
	int worker_id, slot_id;
	unsigned long service_token;
	request_slot *slot;
	RequestNode *req_node;
	ServiceNode *ser_node = findService_by_id(obj->op_params.end_job.service_id);

	if(ser_node != NULL){
		worker_id = obj->op_params.end_job.worker_id;
		service_token = obj->op_params.end_job.service_token;
		slot_id = obj->op_params.end_job.slot_id;
		if(!Service_check_Worker(ser_node, service_token, worker_id)){
			decrease_workers(ser_node);
			return LINBER_KILL_WORKER;
		} else {
			unload_slot_and_get_request(ser_node, &req_node, &slot, slot_id);
			if(slot != NULL){
				if(req_node != NULL){
					req_node->response_len = obj->op_params.end_job.response_len;
					req_node->response_shm_mode = (obj->op_params.end_job.response_shm_mode == 1);
					if(req_node->response_shm_mode){	// use shared memory
						req_node->response.shm_key = obj->op_params.end_job.response.shm_key;
					} else {
						if((req_node->response_len > LINBER_MAX_MEM_SIZE)){
							req_node->result_cmd = LINBER_REQUEST_FAILED;
							enqueue_request_completed(ser_node, req_node);
							up(&req_node->Request_sem);	// end job
							return LINBER_PAYLOAD_SIZE_ERROR;
						}
						req_node->response.data = kmalloc(req_node->response_len, GFP_KERNEL);
						checkMemoryError(copy_from_user(req_node->response.data, obj->op_params.end_job.response.data, req_node->response_len));
					}
					req_node->result_cmd = LINBER_REQUEST_SUCCESS;
					enqueue_request_completed(ser_node, req_node);
					up(&req_node->Request_sem);	// end job
				} else {
					return LINBER_SERVICE_SKIP_JOB;
				}
			} else {
				decrease_workers(ser_node);
				return LINBER_KILL_WORKER;
			}
		}
	}
	return 0;
}

static int linber_destroy_service(linber_service_struct *obj){
	ServiceNode *node;
	node = findService(obj->service_uri);
	if(node != NULL){
		if(Service_check_Worker(node, obj->op_params.destroy_service.service_token, 0)){
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

	checkMemoryError(copy_from_user(&system, system_user, sizeof(system_status)));
	system.Max_Working = linber.Max_Working;
	system.requests_count = linber.Request_count;
	system.serving_requests_count = linber.Serving_count;
	system.services_count = linber.Services_count;
	if(linber.Services_count > 0){
		checkMemoryError(copy_from_user(services, system.services, MAX_SERVICES * sizeof(service_status)));
		mutex_lock(&linber.mutex);
		list_for_each_entry(ser_node, &linber.Services, list){
			if(i < linber.Services_count){
				mutex_lock(&ser_node->service_mutex);
				services[i].uri_len = strlen(ser_node->uri);
				checkMemoryError(copy_to_user(services[i].uri, ser_node->uri, services[i].uri_len));
				services[i].exec_time = ser_node->exec_time;
				services[i].Max_Working = ser_node->Workers.parallelism;
				services[i].requests_count = ser_node->Request_count;
				services[i].serving_requests_count = ser_node->Serving_requests.serving_count;
				services[i].serving_time = 0;
				if(ser_node->serving_time > 0 && ser_node->Serving_requests.num_active_slots > 0){
					services[i].serving_time = ser_node->exec_time + (ser_node->serving_time / ser_node->Serving_requests.num_active_slots);
					if(services[i].serving_time > services[i].exec_time){
						services[i].serving_time = services[i].serving_time - (services[i].serving_time)%services[i].exec_time;
					}
				}
				mutex_unlock(&ser_node->service_mutex);
			}
			i++;
		}
		mutex_unlock(&linber.mutex);
	}
	checkMemoryError(copy_to_user(system_user, &system, sizeof(system_status)));
	checkMemoryError(copy_to_user(system.services, services, linber.Services_count * sizeof(service_status)));
	kfree(services);
	return 0;
}


//-------------------------------------------------------------
static long	linber_ioctl(struct file *f, unsigned int ioctl_op, unsigned long ioctl_param){ 
	long ret;
	char *uri_tmp;
	linber_service_struct *obj;
	if(ioctl_op != IOCTL_SYSTEM_STATUS){
		obj = kmalloc(sizeof(linber_service_struct), GFP_KERNEL);
		checkMemoryError(copy_from_user((void*)obj, (void*)ioctl_param, sizeof(linber_service_struct)));
		uri_tmp = kmalloc(obj->service_uri_len + 1, GFP_KERNEL);
		checkMemoryError(copy_from_user((void*)uri_tmp, (void*)obj->service_uri, obj->service_uri_len));
		uri_tmp[obj->service_uri_len] = '\0';
		obj->service_uri = uri_tmp;
	}

	switch(ioctl_op){
		case IOCTL_REGISTER_SERVICE:
			ret = linber_register_service(obj);
			break;

		case IOCTL_REQUEST_SERVICE:
			ret = linber_request_service(obj);
			break;

		case IOCTL_REQUEST_SERVICE_GET_RESPONSE:
			ret = linber_request_service_get_response(obj);
			break;

		case IOCTL_REGISTER_SERVICE_WORKER:
			ret = linber_register_service_worker(obj);
			break;

		case IOCTL_START_JOB_SERVICE:
			ret = linber_Start_Job(obj);
			break;

		case IOCTL_START_JOB_GET_REQUEST_SERVICE:
			ret = linber_Start_Job_get_request(obj);
			break;

		case IOCTL_END_JOB_SERVICE:
			ret = linber_End_Job(obj);
			break;

		case IOCTL_DESTROY_SERVICE:
			ret = linber_destroy_service(obj);
			break;

		case IOCTL_SYSTEM_STATUS:
			ret = linber_get_system_status((void*)ioctl_param);
			break;

		default:
			printk(KERN_ERR "linber:: IOCTL operation %d not implemented\n", ioctl_op);
			ret = LINBER_IOCTL_NOT_IMPLEMENTED;
			break;
	}
	if(ioctl_op != IOCTL_SYSTEM_STATUS){
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

	ret = register_chrdev(0, DEV_NAME, &fops);
	if (ret < 0) {
		printk(KERN_ERR "linber:: register_chrdev() failed with: %d\n", ret);
		return ret;
	}
	dev_major = ret;
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

	initialize_queue(&linber.Services);
	idr_init(&linber.Services_idr);
	linber.Request_count = 0;
	linber.Serving_count = 0;
	linber.Max_Working = 0;
	mutex_init(&linber.mutex);
	linber_manager_init();

	return 0;
}

void cleanup_module(void) {
	ServiceNode *ser_node, *next;

	if(!kthread_stop(linber.linber_manager)){
		printk(KERN_ALERT "Linber:: manager stopped");
	}

	list_for_each_entry_safe(ser_node, next, &linber.Services, list){
		list_del(&ser_node->list);
		destroy_service(ser_node);
	}

	idr_destroy(&linber.Services_idr);

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
MODULE_AUTHOR("Giovanni Falzone");
MODULE_DESCRIPTION("");