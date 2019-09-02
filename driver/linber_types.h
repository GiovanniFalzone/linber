#ifndef LINBER_TYPES
#define LINBER_TYPES

#include <linux/list.h>
#include <linux/idr.h>
#include <linux/rbtree.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include "linber_RBtree.h"

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
	struct list_head list;				// used to implement the list, Completed queue
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
	struct rb_root Requests_rbtree;
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

#endif