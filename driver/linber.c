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

#include <linux/sched.h>

#include "../libs/linber_ioctl.h"

#define DEV_NAME DEVICE_FILE_NAME
#define CLA_NAME "linberclass"

#define CHECK_MEMORY_ERROR(value) if(value!=0){return LINBER_USER_MEMORY_ERROR;}
//#define DEBUG_MODULE


static int dev_major;
static struct class*	dev_class	= NULL; ///< The device-driver class struct pointer
static struct device* dev_device = NULL; ///< The device-driver device struct pointer

//------------------------------------------------------
struct sched_attr {
    __u32 size;
    __u32 sched_policy;
    __u64 sched_flags;

    /* SCHED_NORMAL, SCHED_BATCH */
    __s32 sched_nice;

    /* SCHED_FIFO, SCHED_RR */
    __u32 sched_priority;

    /* SCHED_DEADLINE */
    __u64 sched_runtime;
    __u64 sched_deadline;
    __u64 sched_period;
};


typedef struct RequestNode{
	struct list_head list;				// used to implement the list
	struct semaphore Request_sem;		// used to stop the client waiting for the response
	unsigned long token;				// used to identify the single request when the client use a non blocking tecnique
	bool request_accepted;				// used by workers to check if the request in the queue must be served or destroyied
	unsigned long abs_deadline_ns;		// absolute deadline resect to boot time
	int result_cmd;						// used to mark the request as completed or failed
	unsigned int request_len;
	unsigned int response_len;
	bool response_shm_mode;
	bool request_shm_mode;
	union {
		char * data;					// address where is stored the payload
		key_t shm_key;
	} request;
	union {
		char * data;					// address where is stored the payload
		key_t shm_key;
	} response;
}RequestNode;


typedef struct worker_struct{
	struct task_struct *task;				// worker process
	RequestNode *request;					// array serving requests, size max workers
}worker_struct;

typedef struct ServiceNode{
	struct list_head list;					// used to implement the list
	char *uri;								// service uri
	int id;									// used by idr to find the service without iterating on the list
	unsigned long token;					// security, to check the operations called on a service
	unsigned int period;					// CBS period
	unsigned int budget;					// CBS budget
	unsigned int exec_time_ns;				// execution time for a single request
	unsigned int Request_count;				// how many pending requests
	bool destroy_me;						// used to destroy to stop workers and requests coming while destroying

	struct Workers{
		worker_struct *worker_slots;		// worker slot used to store the serving request and worker infos
		unsigned int Max_Workers;			// predefined maximum number of workers
		unsigned int count_registered;		// how many workers are registered out of Max_Workers
		unsigned int next_id;				// used to identify the worker's slot in the array
		struct semaphore sem_for_request;	// used to stop the workers when there are no pending requests
	} Workers;

	struct list_head RequestsHead;
	struct list_head Completed_RequestsHead;	// list of completed requests, the client take the request using a token

	struct mutex service_mutex;				// to protect the lists and avoid to delete a node while another process is iterating
}ServiceNode;

struct linber_stuct{
	struct list_head Services;				// queue list of services
	struct idr Services_idr;				// idr of services, key:Service id, value: service node address
	unsigned int Services_count;
	unsigned int Request_count;				// total number of request between services
	unsigned int Max_Working;				// maximum number of workers between services
	struct mutex mutex;						// used to protect the service list in case of deletion during iterating
} linber;



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


// ------------------------------------------------------
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
		for(i=0; i<ser_node->Workers.Max_Workers; i++){
			node = ser_node->Workers.worker_slots[i].request;
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
	linber.Max_Working -= ser_node->Workers.Max_Workers;
	kfree(ser_node->Workers.worker_slots);
	mutex_unlock(&ser_node->service_mutex);
	idr_remove(&linber.Services_idr, ser_node->id);
	kfree(ser_node);	// done by last worker
}

static void destroy_service(ServiceNode *ser_node){
	int i = 0;
	RequestNode *req_node = NULL, *next = NULL;
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

	for(i=0; i<ser_node->Workers.count_registered; i++){
		up(&ser_node->Workers.sem_for_request);
	}

	ser_node->destroy_me = true;

	if(ser_node->Workers.count_registered <= 0){
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
	INIT_LIST_HEAD(&ser_node->Completed_RequestsHead);
	INIT_LIST_HEAD(&ser_node->RequestsHead);
	mutex_init(&ser_node->service_mutex);
	get_random_bytes(&ser_node->token, sizeof(ser_node->token));	// 64 bit random token
	ser_node->uri = obj->service_uri;
	ser_node->Request_count = 0;
	ser_node->destroy_me = false;

	ser_node->exec_time_ns = obj->op_params.registration.exec_time_ns;
	ser_node->budget = obj->op_params.registration.service_budget_ns;
	ser_node->period = obj->op_params.registration.service_period_ns;

	sema_init(&ser_node->Workers.sem_for_request, 0);
	ser_node->Workers.Max_Workers = obj->op_params.registration.Max_Working;
	ser_node->Workers.next_id = 0;
	ser_node->Workers.count_registered = 0;
	ser_node->Workers.worker_slots = kmalloc(ser_node->Workers.Max_Workers*sizeof(worker_struct), GFP_KERNEL);
	for(id=0; id<ser_node->Workers.Max_Workers; id++){
		ser_node->Workers.worker_slots[id].request = NULL;
		ser_node->Workers.worker_slots[id].task = NULL;
	}


	mutex_lock(&linber.mutex);
		list_add_tail(&ser_node->list, &linber.Services);
		id = idr_alloc(&linber.Services_idr, (void*)ser_node, 0, MAX_SERVICES, GFP_KERNEL);
		if(id < 0){
			kfree(ser_node);
			return NULL;
		}
		linber.Services_count++;
		linber.Max_Working += ser_node->Workers.Max_Workers;
	mutex_unlock(&linber.mutex);
	ser_node->id = id;
	printk(KERN_INFO "linber::  New Service token:%lu %s, workers:%d,  exec time:%d\n", ser_node->token, ser_node->uri, ser_node->Workers.Max_Workers, ser_node->exec_time_ns);
	return ser_node;
}

static RequestNode* enqueue_request_for_service(	ServiceNode *ser_node, 			\
													char *request,					\
													unsigned int request_len,		\
													bool blocking,					\
													bool shm_mode,					\
													key_t req_key,					\
													unsigned long abs_deadline_ns	\
													){
	RequestNode *req_node;
	req_node = create_Request();
	if(req_node != NULL){
		req_node->abs_deadline_ns = abs_deadline_ns;
		if(shm_mode){
			req_node->request_shm_mode = true;
			req_node->request.shm_key = req_key;
		} else {
			req_node->request_shm_mode = false;
			req_node->request.data = request;
		}
		req_node->request_len = request_len;

		mutex_lock(&ser_node->service_mutex);
			list_add_tail(&req_node->list, &ser_node->RequestsHead);
			ser_node->Request_count++;
			req_node->request_accepted = true;
		mutex_unlock(&ser_node->service_mutex);

		up(&ser_node->Workers.sem_for_request);
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

static worker_struct* get_worker_by_id(ServiceNode *ser_node, unsigned int worker_id){
	worker_struct *worker = NULL;
	if((worker_id >= 0) && (worker_id < ser_node->Workers.Max_Workers)){
		worker = &(ser_node->Workers.worker_slots[worker_id]);
		if((worker->task->pid != current->pid)){
			worker = NULL;
		}
	}
	if(worker == NULL){
		printk(KERN_ERR "linber::  Wrong worker id %d for service: %s\n", worker_id, ser_node->uri);
	}
	return worker;
}

static inline void decrease_workers(ServiceNode *ser_node){
	mutex_lock(&ser_node->service_mutex);
	ser_node->Workers.count_registered--;
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
			ser_node->Workers.count_registered--;
			if(ser_node->Workers.count_registered <= 0){
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

static RequestNode* worker_wait_for_request(ServiceNode *ser_node, unsigned int worker_id){
	RequestNode *req_node = NULL;
	worker_struct *worker = NULL;

	worker = get_worker_by_id(ser_node, worker_id);
	if(worker == NULL){
		return NULL;
	}

	if(worker->request != NULL){
		// Job Fault, Worker executed StartJob without executing EndJob, possible in some error
		up(&worker->request->Request_sem);	// end job
		worker->request = NULL;
	}

	down_interruptible(&ser_node->Workers.sem_for_request);		// wait for request

	if(Service_check_alive(ser_node) >= 0){
		req_node = get_request(ser_node);
		if(req_node != NULL){
			worker->request = req_node;
		}
	}
	return req_node;
}

static RequestNode* worker_get_request(ServiceNode *ser_node,unsigned int worker_id){
	RequestNode *req_node = NULL;
	worker_struct *worker = NULL;
	worker = get_worker_by_id(ser_node, worker_id);
	if(worker != NULL){
		req_node = worker->request;
		worker->request = NULL;
	}
	return req_node;
}


//------------------------------------------------------
static int linber_register_service(linber_service_struct *obj){
	ServiceNode *ser_node;
	ser_node = findService(obj->service_uri);
	if(ser_node == NULL){
		ser_node = linber_create_service(obj);
		if(ser_node == NULL){
			return LINBER_SERVICE_REGISTRATION_FAIL;
		}
		CHECK_MEMORY_ERROR(put_user(ser_node->token, obj->op_params.registration.ptr_service_token));
		CHECK_MEMORY_ERROR(put_user(ser_node->id, obj->op_params.registration.ptr_service_id));
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
	unsigned long token, abs_deadline_ns;
	bool blocking, shm_mode;

	ser_node = findService(obj->service_uri);
	if(ser_node != NULL && (!ser_node->destroy_me)){
		status = obj->op_params.request.status;

		//----------------------init-----------------------
		if(status == LINBER_REQUEST_INIT){
			request_len = obj->op_params.request.request_len;
			shm_mode = (obj->op_params.request.request_shm_mode == 1);
			blocking = (obj->op_params.request.blocking == 1);
			abs_deadline_ns = obj->op_params.request.abs_deadline_ns;
			if(!shm_mode){
				if(request_len > LINBER_MAX_MEM_SIZE){
					return LINBER_PAYLOAD_SIZE_ERROR;
				}
				request = kmalloc(request_len, GFP_KERNEL);
				CHECK_MEMORY_ERROR(copy_from_user(request, obj->op_params.request.request.data, request_len));
			} else {
				request_key = obj->op_params.request.request.shm_key;
			}
			req_node = enqueue_request_for_service(ser_node, request, request_len, blocking, shm_mode, request_key, abs_deadline_ns);
			if(req_node == NULL){
				return LINBER_REQUEST_FAILED;
			}
			CHECK_MEMORY_ERROR(put_user(req_node->token, obj->op_params.request.ptr_token));
			status = LINBER_REQUEST_COMPLETED;
			if(!blocking){
				return LINBER_REQUEST_WAITING;
			}
		}
		//----------------------waiting-----------------------
		if(status == LINBER_REQUEST_WAITING){
			CHECK_MEMORY_ERROR(get_user(token, obj->op_params.request.ptr_token));
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
					CHECK_MEMORY_ERROR(put_user(req_node->response_len, obj->op_params.request.ptr_response_len));
					CHECK_MEMORY_ERROR(put_user(req_node->response_shm_mode, obj->op_params.request.ptr_response_shm_mode));
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
		CHECK_MEMORY_ERROR(get_user(token, obj->op_params.request.ptr_token));
		req_node = get_Completed_Request(ser_node, token);
		if(req_node != NULL){
			ret = req_node->result_cmd;
			if(req_node->response_shm_mode){	// response in shared memory
				CHECK_MEMORY_ERROR(put_user(req_node->response.shm_key, obj->op_params.request.ptr_shm_response_key));
			} else {
				CHECK_MEMORY_ERROR(copy_to_user(obj->op_params.request.ptr_response, req_node->response.data, req_node->response_len));
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


static void Dispatch_as_BestEffort(void){
	int res;
	struct sched_attr attr;
	attr.size = sizeof(struct sched_attr);
	attr.sched_flags = SCHED_FLAG_RESET_ON_FORK;
	attr.sched_policy = SCHED_FIFO;
	attr.sched_nice = 0;
	attr.sched_priority = 99;
	attr.sched_runtime = 0;
	attr.sched_period = attr.sched_deadline = 0;
	res = sched_setattr(current, &attr);
	if (res < 0){
		printk(KERN_INFO "Linber:: Error %i setting Best Effort scheduling\n", res);
	} else {
		#ifdef DEBUG_MODULE
			printk(KERN_INFO "Linber:: Dispatched as Best Effort\n");
		#endif
	}
}

//------------------
// set as SCHED_DEADLINE with Service Period as period, Service Period as deadline and One request exec time as runtime
//------------------
static int Dispatch_as_RealTime(unsigned long exec_time_ns, unsigned long period_ns, unsigned long rel_deadline_ns){
	int res;
	struct sched_attr attr;
	attr.size = sizeof(struct sched_attr);
	attr.sched_flags = 0;//SCHED_FLAG_RECLAIM | SCHED_FLAG_RESET_ON_FORK;
	attr.sched_policy = SCHED_DEADLINE;
	attr.sched_runtime = exec_time_ns;
	attr.sched_period = period_ns;
	attr.sched_deadline = rel_deadline_ns;
	res = sched_setattr(current, &attr);
	if (res < 0){
		printk(KERN_INFO "Linber:: SCHED_DEADLINE FAULT %i with Q:%lu P:%lu D:%lu\n", res, exec_time_ns/1000000, period_ns/1000000, rel_deadline_ns/1000000);
	} else {
		#ifdef DEBUG_MODULE
			printk(KERN_INFO "Linber:: Dispatched as RealTime Q:%lu P:%lu D:%lu\n", exec_time_ns/1000000, period_ns/1000000, rel_deadline_ns/1000000);
		#endif
	}
	return res;
}


static int linber_register_service_worker(linber_service_struct *obj){
	ServiceNode *ser_node = findService(obj->service_uri);
	int worker_id, service_period;

	if(ser_node != NULL){
		if(!Service_check_Worker(ser_node, obj->op_params.register_worker.service_token, 0)){
			return -1;
		}
		if(ser_node->Workers.next_id < ser_node->Workers.Max_Workers){
			mutex_lock(&ser_node->service_mutex);
				ser_node->Workers.count_registered++;
				worker_id = ser_node->Workers.next_id++;
			mutex_unlock(&ser_node->service_mutex);
			ser_node->Workers.worker_slots[worker_id].task = current;
			CHECK_MEMORY_ERROR(put_user(worker_id, obj->op_params.register_worker.ptr_worker_id));
//-----------------------------------------------------
// I don't know why but this is required
			service_period = 1000000000;
			service_period = service_period/2;
			Dispatch_as_RealTime(ser_node->exec_time_ns, service_period, service_period);
//-----------------------------------------------------
		}
	} else {
		return -1;
	}
	return 0;
}

static int linber_Start_Job(linber_service_struct *obj){
	RequestNode *req_node = NULL;
	ServiceNode *ser_node = findService_by_id(obj->op_params.start_job.service_id);
	int rel_deadline_ns, service_period;
	unsigned int worker_id;
	unsigned long service_token;
	ktime_t now;

	if(ser_node != NULL){
		// check arguments
		worker_id = obj->op_params.start_job.worker_id;
		service_token = obj->op_params.start_job.service_token;
		if(!Service_check_Worker(ser_node, service_token, worker_id)){
			decrease_workers(ser_node);
			printk(KERN_ERR "linber:: Service Check Worker failded token:%lu id:%u\n", service_token, worker_id);
			return LINBER_KILL_WORKER;
		}
		req_node = worker_wait_for_request(ser_node, worker_id);
		if(req_node == NULL){
			return LINBER_SERVICE_SKIP_JOB;
		} else {
			if(req_node->abs_deadline_ns == -1){	// unsigned long -1 is the maximum value 2^64 -1
				Dispatch_as_BestEffort();
			} else {
				now = ktime_get();		// CLOCK_MONOTONIC, since boot without considering suspend
				rel_deadline_ns = (req_node->abs_deadline_ns - ktime_to_ns(now)); // abs_deadline - now
				if(rel_deadline_ns <= 0){
					printk(KERN_INFO "Linber:: request expired rel: %i abs:%lu\n", rel_deadline_ns, req_node->abs_deadline_ns);
					Dispatch_as_BestEffort();
				} else {
					service_period = 1000000000;
					rel_deadline_ns = (rel_deadline_ns > service_period)?service_period:rel_deadline_ns;
					if(Dispatch_as_RealTime(ser_node->exec_time_ns, service_period, rel_deadline_ns) < 0){
						Dispatch_as_BestEffort();
					}
				}
			}
			CHECK_MEMORY_ERROR(put_user(req_node->request_len, obj->op_params.start_job.ptr_request_len));
			CHECK_MEMORY_ERROR(put_user(req_node->request_shm_mode, obj->op_params.start_job.ptr_request_shm_mode));
			if(req_node->request_shm_mode){
				CHECK_MEMORY_ERROR(put_user(req_node->request.shm_key, obj->op_params.start_job.data.ptr_request_key));
			}
		}
	} else {
		return LINBER_SERVICE_NOT_EXISTS;
	}
	return 0;
}

static int linber_Start_Job_get_request(linber_service_struct *obj){
	worker_struct *worker;
	RequestNode *req_node;
	ServiceNode *ser_node = findService_by_id(obj->op_params.start_job.service_id);
	unsigned int worker_id;
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

		worker = get_worker_by_id(ser_node, worker_id);
		if(worker != NULL){
			req_node = worker->request;
			if(req_node == NULL){
				return LINBER_SERVICE_SKIP_JOB;
			} else {
				// pass parameters to worker and exec job
				CHECK_MEMORY_ERROR(copy_to_user(obj->op_params.start_job.data.ptr_request, req_node->request.data, req_node->request_len));
			}
		}
	} else {
		return LINBER_SERVICE_NOT_EXISTS;
	}
	return 0;
}


static int linber_End_Job(linber_service_struct *obj){
	int worker_id;
	unsigned long service_token;
	RequestNode *req_node;
	ServiceNode *ser_node = findService_by_id(obj->op_params.end_job.service_id);

	if(ser_node != NULL){
		worker_id = obj->op_params.end_job.worker_id;
		service_token = obj->op_params.end_job.service_token;
		if(!Service_check_Worker(ser_node, service_token, worker_id)){
			decrease_workers(ser_node);
			return LINBER_KILL_WORKER;
		} else {
			req_node = worker_get_request(ser_node, worker_id);
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
					CHECK_MEMORY_ERROR(copy_from_user(req_node->response.data, obj->op_params.end_job.response.data, req_node->response_len));
				}
				req_node->result_cmd = LINBER_REQUEST_SUCCESS;
				enqueue_request_completed(ser_node, req_node);
				up(&req_node->Request_sem);	// end job
			} else {
				return LINBER_SERVICE_SKIP_JOB;
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
	ServiceNode ser_node_cpy;
	int i = 0;

	CHECK_MEMORY_ERROR(copy_from_user(&system, system_user, sizeof(system_status)));
	system.Max_Working = linber.Max_Working;
	system.services_count = linber.Services_count;
	if(linber.Services_count > 0){
		CHECK_MEMORY_ERROR(copy_from_user(services, system.services, MAX_SERVICES * sizeof(service_status)));
		mutex_lock(&linber.mutex);
			list_for_each_entry(ser_node, &linber.Services, list){
				if(i < linber.Services_count){
					mutex_lock(&ser_node->service_mutex);
						ser_node_cpy = *ser_node;
						services[i].uri_len = strlen(ser_node->uri);
						CHECK_MEMORY_ERROR(copy_to_user(services[i].uri, ser_node->uri, services[i].uri_len));
					mutex_unlock(&ser_node->service_mutex);
					services[i].Max_Working = ser_node_cpy.Workers.Max_Workers;
					services[i].requests_count = ser_node_cpy.Request_count;
					services[i].exec_time_ms = nSEC_TO_mSEC(ser_node_cpy.exec_time_ns);
					services[i].period_ms = nSEC_TO_mSEC(ser_node->period);
					services[i].budget_ms = nSEC_TO_mSEC(ser_node->budget);
				}
				i++;
			}
		mutex_unlock(&linber.mutex);
	}
	CHECK_MEMORY_ERROR(copy_to_user(system_user, &system, sizeof(system_status)));
	CHECK_MEMORY_ERROR(copy_to_user(system.services, services, linber.Services_count * sizeof(service_status)));
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
		CHECK_MEMORY_ERROR(copy_from_user((void*)obj, (void*)ioctl_param, sizeof(linber_service_struct)));
		uri_tmp = kmalloc(obj->service_uri_len + 1, GFP_KERNEL);
		CHECK_MEMORY_ERROR(copy_from_user((void*)uri_tmp, (void*)obj->service_uri, obj->service_uri_len));
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

	INIT_LIST_HEAD(&linber.Services);
	idr_init(&linber.Services_idr);
	linber.Request_count = 0;
	linber.Max_Working = 0;
	mutex_init(&linber.mutex);

	return 0;
}

void cleanup_module(void) {
	ServiceNode *ser_node, *next;

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