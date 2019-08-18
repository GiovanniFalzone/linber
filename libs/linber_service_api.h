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

int ioctl_send(int file_desc, unsigned int cmd, void* param){
	return ioctl(file_desc, cmd, param);
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

int linber_register_service(char * service_uri, unsigned int uri_len, unsigned int exec_time, unsigned int max_workers, unsigned long * service_token){
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
	param.linber_params.registration.exec_time = exec_time;
	param.linber_params.registration.max_concurrent_workers = max_workers;
	param.linber_params.registration.ret_service_token = service_token;
	return ioctl_send(linber_device_file_desc, IOCTL_REGISTER_SERVICE, &param);
}

int linber_register_service_worker(char * service_uri, unsigned int uri_len, unsigned long service_token, unsigned int * worker_id){
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
	param.linber_params.register_worker.ret_worker_id = worker_id;
	param.linber_params.register_worker.service_token;
	return ioctl_send(linber_device_file_desc, IOCTL_REGISTER_SERVICE_WORKER, &param);
}

int linber_request_service(char * service_uri, unsigned int uri_len, unsigned int rel_deadline,\
							char *service_params, int service_params_len, char *service_result, int *service_result_len){
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
	param.linber_params.request.service_params = service_params;
	param.linber_params.request.service_params_len = service_params_len;
	param.linber_params.request.ptr_service_result = service_result;
	param.linber_params.request.ptr_service_result_len = service_result_len;
	param.linber_params.request.rel_deadline = rel_deadline;
	return ioctl_send(linber_device_file_desc, IOCTL_REQUEST_SERVICE, &param);
}

int linber_start_job_service(char * service_uri, unsigned int uri_len, unsigned long service_token, unsigned int worker_id,\
								 unsigned int *slot_id, char *service_params, int *service_params_len){
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
	param.linber_params.start_job.worker_id = worker_id;
	param.linber_params.start_job.service_token = service_token;
	param.linber_params.start_job.ptr_slot_id = slot_id;
	param.linber_params.start_job.ptr_service_params = service_params;
	param.linber_params.start_job.ptr_service_params_len = service_params_len;
	return ioctl_send(linber_device_file_desc, IOCTL_START_JOB_SERVICE, &param);
}

int linber_end_job_service(char * service_uri, unsigned int uri_len, unsigned long service_token, unsigned int worker_id,\
								 unsigned int slot_id, char *service_result, int service_result_len){
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
	param.linber_params.end_job.worker_id = worker_id;
	param.linber_params.end_job.service_token = service_token;
	param.linber_params.end_job.slot_id = slot_id;
	param.linber_params.end_job.service_result = service_result;
	param.linber_params.end_job.service_result_len = service_result_len;
	return ioctl_send(linber_device_file_desc, IOCTL_END_JOB_SERVICE, &param);
}

int linber_destroy_service(char * service_uri, unsigned int uri_len, unsigned long service_token){
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
	param.linber_params.destroy_service.service_token = service_token;
	return ioctl_send(linber_device_file_desc, IOCTL_DESTROY_SERVICE, &param);
}

int linber_system_status(system_status *system){
	if(linber_device_file_desc < 0){
		printf("Linber start job: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	return ioctl_send(linber_device_file_desc, IOCTL_SYSTEM_STATUS, system);
}


#endif