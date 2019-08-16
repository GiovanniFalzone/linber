#ifndef LINBER_IOCTL_H
#define LINBER_IOCTL_H

#include <linux/ioctl.h>
#define MAJOR_NUM	239

#define MAX_SERVICES	10
#define MAX_URI_LEN		64

#define IOCTL_REQUEST_SERVICE			_IOW(MAJOR_NUM, 0, char*)
#define IOCTL_REGISTER_SERVICE			_IOR(MAJOR_NUM, 1, char*)
#define IOCTL_REGISTER_SERVICE_WORKER	_IOR(MAJOR_NUM, 2, char*)
#define IOCTL_START_JOB_SERVICE			_IOW(MAJOR_NUM, 3, char*)
#define IOCTL_END_JOB_SERVICE			_IOW(MAJOR_NUM, 4, char*)
#define IOCTL_DESTROY_SERVICE			_IOW(MAJOR_NUM, 5, char*)

#define IOCTL_SYSTEM_STATUS				_IOW(MAJOR_NUM, 6, char*)


#define DEVICE_FILE_NAME		"linber"
#define DEVICE_FILE_FULL_PATH	"/dev/" DEVICE_FILE_NAME

#define SERVICE_URI_MAX_LEN		50

#define LINBER_ABORT_REQUEST	-1
#define LINBER_SUCCESS_REQUEST	0

#define LINBER_SKIP_JOB			1
#define LINBER_KILL_WORKER		-1

typedef struct linber_service_struct {
	char *service_uri;
	unsigned int service_uri_len;
	union params{
		struct registration {
			unsigned int exec_time;
			unsigned int max_concurrent_workers;
			unsigned long *ret_service_id;
		} registration;

		struct register_worker {
			unsigned int *ret_worker_id;
		} register_worker;

		struct destroy_service {
			unsigned long service_id;
		} destroy_service;

		struct request {
			unsigned int rel_deadline;
			char *params;
			int params_len;
		} request;

		struct start_job{
			unsigned int worker_id;	// start job
			unsigned long service_id;
		} start_job;

		struct end_job{
			unsigned int worker_id;
			unsigned long service_id;
			char *ret;
			int ret_len;
		} end_job;
	} service_params;
} linber_service_struct;

typedef struct service_status{
	char * uri;
	unsigned int uri_len;
	unsigned long id;
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