#ifndef LINBER_IOCTL_H
#define LINBER_IOCTL_H
//----------------------
//#define DEBUG_MESSAGE
//----------------------

#define MAGIC_NUM		0x20
#define SEQ_NUM			0

#define MAX_SERVICES	10
#define MAX_URI_LEN		64

#define LINBER_MAX_MEM_SIZE	((1<<22) - 32)		// 4Mb

#define IOCTL_REGISTER_SERVICE						_IOWR(MAGIC_NUM, SEQ_NUM+1, char*)
#define IOCTL_REGISTER_SERVICE_WORKER				_IOWR(MAGIC_NUM, SEQ_NUM+2, char*)
#define IOCTL_START_JOB_SERVICE						_IOWR(MAGIC_NUM, SEQ_NUM+3, char*)
#define IOCTL_START_JOB_GET_REQUEST_SERVICE			_IOWR(MAGIC_NUM, SEQ_NUM+4, char*)
#define IOCTL_END_JOB_SERVICE						_IOWR(MAGIC_NUM, SEQ_NUM+5, char*)
#define IOCTL_DESTROY_SERVICE						_IOWR(MAGIC_NUM, SEQ_NUM+6, char*)
#define IOCTL_SYSTEM_STATUS							_IOWR(MAGIC_NUM, SEQ_NUM+7, char*)
#define IOCTL_REQUEST_SERVICE						_IOWR(MAGIC_NUM, SEQ_NUM+8, char*)
#define IOCTL_REQUEST_SERVICE_GET_RESPONSE			_IOWR(MAGIC_NUM, SEQ_NUM+9, char*)


#define DEVICE_FILE_NAME		"linber"
#define DEVICE_FILE_FULL_PATH	"/dev/" DEVICE_FILE_NAME

#define LINBER_IOCTL_NOT_IMPLEMENTED 	-32
#define LINBER_USER_MEMORY_ERROR		-31
#define LINBER_PAYLOAD_SIZE_ERROR		-30

#define LINBER_SERVICE_REGISTRATION_FAIL			-2
#define LINBER_SERVICE_REGISTRATION_ALREADY_EXISTS	-1
#define LINBER_SERVICE_REGISTRATION_SUCCESS			0

#define LINBER_REQUEST_SUCCESS						0
#define LINBER_ABORT_REQUEST						-1
#define LINBER_REQUEST_FAILED						-2

#define LINBER_SERVICE_SKIP_JOB		1
#define LINBER_KILL_WORKER			-1

#define LINBER_REQUEST_ACCEPTED 	1
#define LINBER_SERVICE_NOT_EXISTS	-3

#define LINBER_REQUEST_INIT			0
#define LINBER_REQUEST_WAITING		1
#define LINBER_REQUEST_COMPLETED	2

#define SEC_TO_NSEC(sec) sec*1000000000
#define mSEC_TO_NSEX(msec) msec*1000000

#define FALSE	0
#define TRUE	1

typedef int boolean;


typedef struct linber_service_struct {
	char *service_uri;
	unsigned int service_uri_len;
	union op_params{
		struct registration {
			unsigned long exec_time_ns;
			unsigned int Max_Working;
			int *ptr_service_id;
			unsigned long *ptr_service_token;
		} registration;

		struct register_worker {
			unsigned long service_token;
			unsigned int *ptr_worker_id;
		} register_worker;

		struct destroy_service {
			unsigned long service_token;
		} destroy_service;

		struct request {
			boolean blocking;
			boolean request_shm_mode;
			boolean *ptr_response_shm_mode;
			int status;
			unsigned long abs_deadline_ns;
			unsigned long *ptr_token;
			int request_len;
			int *ptr_response_len;
			union {
				char *data;
				key_t shm_key;
			} request;
			char *ptr_response;
			key_t *ptr_shm_response_key;
		} request;

		struct start_job{
			int service_id;
			unsigned int worker_id;	// start job
			unsigned long service_token;
			int *ptr_request_len;
			boolean *ptr_request_shm_mode;
			union {
				char *ptr_request;
				key_t *ptr_request_key;
			} data;
		} start_job;

		struct end_job{
			int service_id;
			unsigned int worker_id;
			unsigned long service_token;
			int response_len;
			boolean response_shm_mode;
			union {
				char *data;
				key_t shm_key;
			} response;
		} end_job;
	} op_params;
} linber_service_struct;

typedef struct service_status{
	char * uri;
	unsigned int uri_len;
	unsigned long exec_time_ms;
	unsigned int requests_count;
	unsigned int Max_Working;
} service_status;

typedef struct system_status {
	unsigned int Max_Working;
	unsigned int services_count;
	service_status *services;
}system_status;

#endif