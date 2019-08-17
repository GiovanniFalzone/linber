#ifndef LINBER_SERVICE_API_H
#define LINBER_SERVICE_API_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "linber_ioctl.h"

#define LINBER_ERROR_DEVICE_FILE	-1
#define LINBER_ERROR_URI			-2

#define DEBUG

int linber_device_file_desc = -1;

int ioctl_send(int file_desc, unsigned int cmd, linber_service_struct param){
	return ioctl(file_desc, cmd, &param);
}

int ioctl_system_status(int file_desc, system_status *param){
	int ret;
	ret = ioctl(file_desc, IOCTL_SYSTEM_STATUS, param);
	if(ret < 0){
		return -1;
	}
	return 0;
}


//------------------------------------------
int linber_init(){
	linber_device_file_desc = open(DEVICE_FILE_FULL_PATH, 0);
	if(linber_device_file_desc < 0){
		printf("Can't open %s\n", DEVICE_FILE_FULL_PATH);
		return LINBER_ERROR_DEVICE_FILE;
	}
	return 0;
}

int linber_exit(){
	if(linber_device_file_desc < 0){
		printf("Linber Exit: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	close(linber_device_file_desc);
	return 0;
}

int linber_register_service(char * service_uri, unsigned int uri_len, unsigned int exec_time, unsigned int max_workers, unsigned long * service_id){
	if(linber_device_file_desc < 0){
		printf("Linber Register: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	if(uri_len > SERVICE_URI_MAX_LEN){
		printf("Linber Register: uri size too large\n");
		return LINBER_ERROR_URI;
	}
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.service_params.registration.exec_time = exec_time;
	param.service_params.registration.max_concurrent_workers = max_workers;
	param.service_params.registration.ret_service_id = service_id;
	return ioctl_send(linber_device_file_desc, IOCTL_REGISTER_SERVICE, param);
}

int linber_register_service_worker(char * service_uri, unsigned int uri_len, unsigned int * worker_id){
	if(linber_device_file_desc < 0){
		printf("Linber Register: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	if(uri_len > SERVICE_URI_MAX_LEN){
		printf("Linber Register: uri size too large\n");
		return LINBER_ERROR_URI;
	}
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.service_params.register_worker.ret_worker_id = worker_id;
	return ioctl_send(linber_device_file_desc, IOCTL_REGISTER_SERVICE_WORKER, param);
}

int linber_request_service(char * service_uri, unsigned int uri_len, unsigned int rel_deadline){
	if(linber_device_file_desc < 0){
		printf("Linber Request: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	if(uri_len > SERVICE_URI_MAX_LEN){
		printf("Linber Request: uri size too large\n");
		return LINBER_ERROR_URI;
	}
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
//	param.service_params.request.params;
//	param.service_params.request.params_len;
	param.service_params.request.rel_deadline = rel_deadline;
	return ioctl_send(linber_device_file_desc, IOCTL_REQUEST_SERVICE, param);
}

int linber_start_job_service(char * service_uri, unsigned int uri_len, unsigned long service_id, unsigned int worker_id, unsigned int * slot_id){
	if(linber_device_file_desc < 0){
		printf("Linber start job: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	if(uri_len > SERVICE_URI_MAX_LEN){
		printf("Linber start job: uri size too large\n");
		return LINBER_ERROR_URI;
	}
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.service_params.start_job.worker_id = worker_id;
	param.service_params.start_job.service_id = service_id;
	param.service_params.start_job.ret_slot_id = slot_id;
	return ioctl_send(linber_device_file_desc, IOCTL_START_JOB_SERVICE, param);
}

int linber_end_job_service(char * service_uri, unsigned int uri_len, unsigned long service_id, unsigned int worker_id, unsigned int slot_id){
	if(linber_device_file_desc < 0){
		printf("Linber end job: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	if(uri_len > SERVICE_URI_MAX_LEN){
		printf("Linber end job: uri size too large\n");
		return LINBER_ERROR_URI;
	}
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.service_params.end_job.worker_id = worker_id;
	param.service_params.end_job.service_id = service_id;
	param.service_params.end_job.slot_id = slot_id;
//	param.service_params.end_job.ret;
//	param.service_params.end_job.ret_len;
	return ioctl_send(linber_device_file_desc, IOCTL_END_JOB_SERVICE, param);
}

int linber_destroy_service(char * service_uri, unsigned int uri_len, unsigned long service_id){
	if(linber_device_file_desc < 0){
		printf("Linber start job: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	if(uri_len > SERVICE_URI_MAX_LEN){
		printf("Linber start job: uri size too large\n");
		return LINBER_ERROR_URI;
	}
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.service_params.destroy_service.service_id = service_id;
	return ioctl_send(linber_device_file_desc, IOCTL_DESTROY_SERVICE, param);
}

int linber_system_status(system_status *system){
	if(linber_device_file_desc < 0){
		printf("Linber start job: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	return ioctl_system_status(linber_device_file_desc, system);
}


#endif