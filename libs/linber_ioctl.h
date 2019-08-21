#ifndef LINBER_IOCTL_H
#define LINBER_IOCTL_H

#include <linux/ioctl.h>
#define MAGIC_NUM		0x20
#define SEQ_NUM			0

#define MAX_SERVICES	10
#define MAX_URI_LEN		64

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

#define SERVICE_URI_MAX_LEN		50

#define LINBER_IOCTL_NOT_IMPLEMENTED -32

#define LINBER_SERVICE_REGISTRATION_SUCCESS			0
#define LINBER_SERVICE_REGISTRATION_ALREADY_EXISTS	-1

#define LINBER_REQUEST_SUCCESS						0
#define LINBER_ABORT_REQUEST						-1
#define LINBER_REQUEST_FAILED						-2

#define LINBER_SERVICE_SKIP_JOB		1
#define LINBER_KILL_WORKER			-1

#define LINBER_REQUEST_ACCEPTED 	1
#define LINBER_SERVICE_NOT_EXISTS	-3


typedef struct linber_service_struct {
	char *service_uri;
	unsigned int service_uri_len;
	union linber_params{
		struct registration {
			unsigned int exec_time;
			unsigned int max_concurrent_workers;
			unsigned long *ret_service_token;
		} registration;

		struct register_worker {
			unsigned long service_token;
			unsigned int *ret_worker_id;
		} register_worker;

		struct destroy_service {
			unsigned long service_token;
		} destroy_service;

		struct request {
			unsigned int rel_deadline;
			unsigned long *ptr_token;
			char *service_request;
			int service_request_len;
			char *ptr_service_response;
			int *ptr_service_response_len;
		} request;

		struct start_job{
			unsigned int worker_id;	// start job
			unsigned long service_token;
			unsigned int *ptr_slot_id;
			char *ptr_service_request;
			int *ptr_service_request_len;
		} start_job;

		struct end_job{
			unsigned int worker_id;
			unsigned long service_token;
			unsigned int slot_id;
			char *service_response;
			int service_response_len;
		} end_job;
	} linber_params;
} linber_service_struct;

typedef struct service_status{
	char * uri;
	unsigned int uri_len;
	unsigned int exec_time;
	unsigned int requests_count;
	unsigned int serving_time;
	unsigned int serving_requests_count;
	unsigned int max_concurrent_workers;
} service_status;

typedef struct system_status {
	unsigned int requests_count;
	unsigned int serving_requests_count;
	unsigned int max_concurrent_workers;
	unsigned int services_count;
	service_status *services;
}system_status;

#endif