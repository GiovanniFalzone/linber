#ifndef LINBER_SERVICE_API_H
#define LINBER_SERVICE_API_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "linber_ioctl.h"

#define LINBER_ERROR_DEVICE_FILE	-1
#define LINBER_ERROR_URI			-2

int linber_fd = -1;

int linber_init(){
	linber_fd = open(DEVICE_FILE_FULL_PATH, 0);
	if(linber_fd < 0){
		printf("Init: Can't open %s\n", DEVICE_FILE_FULL_PATH);
		return LINBER_ERROR_DEVICE_FILE;
	}
	return 0;
}

int linber_exit(){
	if(linber_fd < 0){
		printf("Exit: device file error\n");
		return LINBER_ERROR_DEVICE_FILE;
	}
	close(linber_fd);
	return 0;
}

int ioctl_send(unsigned int cmd, void* param){
	boolean close_it = FALSE;
	if(linber_fd < 0){
		close_it = TRUE;
		linber_init();
		if(linber_fd < 0){
			printf("Device file error\n");
			return LINBER_ERROR_DEVICE_FILE;
		}
	}
	long int ret = ioctl(linber_fd, cmd, param);
	if(close_it){
		linber_exit();
	}
	if(ret < 0){
		printf("------------Linber err %li fd:%i----------------\n", ret, linber_fd);
	}
	switch(ret){
		case LINBER_IOCTL_NOT_IMPLEMENTED:
		printf("IOCTL Operation %d not implemented\n", cmd);
		break;
	}
	return ret;
}

//------------------------------------------------
int linber_register_service(char *service_uri, unsigned int uri_len, int *service_id, unsigned int exec_time, unsigned int max_workers, unsigned long *service_token){
	int ret = 0;
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.registration.exec_time = exec_time;
	param.op_params.registration.max_concurrent_workers = max_workers;
	param.op_params.registration.ptr_service_id = service_id;
	param.op_params.registration.ptr_service_token = service_token;
	ret = ioctl_send(IOCTL_REGISTER_SERVICE, &param);
	switch(ret){
		case 0:
			printf("Successfull Registration %s, token %lu\n", service_uri, *service_token);
			break;

		case LINBER_SERVICE_REGISTRATION_ALREADY_EXISTS:
			printf("Register: Service %s already exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_register_service_worker(char *service_uri, unsigned int uri_len, unsigned long service_token, unsigned int * worker_id, char **file_str){
	int ret = 0, fd, file_str_len;
	char worker_id_str[4];
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.register_worker.ptr_worker_id = worker_id;
	param.op_params.register_worker.service_token = service_token;
	ret = ioctl_send(IOCTL_REGISTER_SERVICE_WORKER, &param);

	if(ret >= 0){
		if(*worker_id > 99){
			return -1;
		}
		sprintf(worker_id_str, "%d", *worker_id);
		file_str_len = uri_len + strlen("/tmp/_99\0");
		*file_str = (char*)malloc(file_str_len);
		sprintf(*file_str, "%s", "/tmp/\0");
		strcat(*file_str, service_uri);
		strcat(*file_str, "_\0");
		strcat(*file_str, worker_id_str);
		printf("new service worker file %s\n", *file_str);
		fd = open(*file_str, O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);
		close(fd);
	}

	switch(ret){
		case 0:
			break;
		case LINBER_SERVICE_NOT_EXISTS:
			printf("RegisterWorker: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_request_service_shm(char *service_uri, unsigned int uri_len, unsigned int rel_deadline,\
							char *request, int request_len,\
							char **response, int *response_len){
	int ret = 0;
	unsigned long token;
	linber_service_struct param;
	int shmid;
	key_t key;
	char *shm_mode;

	param.op_params.request.shm_mode = TRUE;
	param.op_params.request.blocking = TRUE;;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.request.data.mem.request = request;
	param.op_params.request.request_len = request_len;
	param.op_params.request.ptr_response_len = response_len;
	param.op_params.request.rel_deadline = rel_deadline;
	param.op_params.request.ptr_token = &token;
	param.op_params.request.status = LINBER_REQUEST_INIT;
	param.op_params.request.data.shm.ptr_shm_response_key = &key;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);

	if ((shmid = shmget(key, *response_len, 0666)) < 0) {
		printf("shmget\n");
		return -1;
	}
	if ((shm_mode = (char*)shmat(shmid, NULL, 0)) == (char *) -1) {
		printf("shmat\n");
		return -1;
	}

	if(ret == LINBER_REQUEST_SUCCESS){
		*response = (char*)malloc(*response_len);
		if(response != NULL){
			memcpy(*response, shm_mode, *response_len);
		} else {
			ret = LINBER_REQUEST_FAILED;
		}
	}
	if(shmdt(shm_mode)) {
		perror("shmop: shmdt failed");
	}


	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}


// blocking
int linber_request_service(char *service_uri, unsigned int uri_len, unsigned int rel_deadline,\
							char *request, int request_len,\
							char **response, int *response_len){
	int ret = 0;
	unsigned long token;
	linber_service_struct param;

	param.op_params.request.shm_mode = FALSE;
	param.op_params.request.blocking = TRUE;;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.request.data.mem.request = request;
	param.op_params.request.request_len = request_len;
	param.op_params.request.ptr_response_len = response_len;
	param.op_params.request.rel_deadline = rel_deadline;
	param.op_params.request.ptr_token = &token;
	param.op_params.request.status = LINBER_REQUEST_INIT;
	param.op_params.request.data.shm.ptr_shm_response_key = NULL;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);

	if(ret == LINBER_REQUEST_SUCCESS){
		*response = (char*)malloc(*response_len);
		if(response != NULL){
			param.op_params.request.data.mem.ptr_response = *response;
			ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param);
		} else {
			ret = LINBER_REQUEST_FAILED;
		}
	}
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_request_service_no_blocking(char *service_uri, unsigned int uri_len, unsigned int rel_deadline,\
										char *request, int request_len,\
										unsigned long *ptr_token){
	int ret = 0;
	linber_service_struct param;

	param.op_params.request.shm_mode = FALSE;
	param.op_params.request.blocking = FALSE;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.request.data.mem.request = request;
	param.op_params.request.request_len = request_len;
	param.op_params.request.rel_deadline = rel_deadline;
	param.op_params.request.ptr_token = ptr_token;
	param.op_params.request.status = LINBER_REQUEST_INIT;
	param.op_params.request.data.shm.ptr_shm_response_key = NULL;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_request_service_get_response(char *service_uri, unsigned int uri_len,\
										char **response, int *response_len,\
										unsigned long *ptr_token){
	int ret = 0;
	linber_service_struct param;

	param.op_params.request.shm_mode = FALSE;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.request.ptr_response_len = response_len;
	param.op_params.request.ptr_token = ptr_token;
	param.op_params.request.status = LINBER_REQUEST_WAITING;
	param.op_params.request.data.shm.ptr_shm_response_key = NULL;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);
	if(ret == LINBER_REQUEST_SUCCESS){
		*response = (char*)malloc(*response_len);
		if(response != NULL){
			param.op_params.request.data.mem.ptr_response = *response;
			ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param);
		} else {
			ret = LINBER_REQUEST_FAILED;
		}
	}
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}


int linber_start_job_service(char *service_uri, unsigned int uri_len, int service_id, unsigned long service_token, unsigned int worker_id,\
								 unsigned int *slot_id, char **request, int *request_len){
	int ret = 0;
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.start_job.service_id = service_id;
	param.op_params.start_job.worker_id = worker_id;
	param.op_params.start_job.service_token = service_token;
	param.op_params.start_job.ptr_slot_id = slot_id;
	param.op_params.start_job.ptr_request_len = request_len;
	ret = ioctl_send(IOCTL_START_JOB_SERVICE, &param);
	if(ret >= 0){
		*request = (char*)malloc(*request_len);
		param.op_params.start_job.ptr_request = *request;
		ret = ioctl_send(IOCTL_START_JOB_GET_REQUEST_SERVICE, &param);
	}
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("StartJob: Service %s does not exists\n", service_uri);
			break;
		case LINBER_KILL_WORKER:
			printf("StartJob: fatal error, pls kill the worker %d token:%lu\n", worker_id, service_token);
			break;
		case LINBER_SERVICE_SKIP_JOB:
			printf("StartJob: Skip job, Spurious wakeup for workerd %d\n", worker_id);
			break;
	}
	return ret;
}

int linber_end_job_service(char *service_uri, unsigned int uri_len, int service_id, unsigned long service_token, unsigned int worker_id,\
							unsigned int slot_id,
							char *request, char *response, int response_len){
	int ret = 0;
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.end_job.service_id = service_id;
	param.op_params.end_job.worker_id = worker_id;
	param.op_params.end_job.service_token = service_token;
	param.op_params.end_job.slot_id = slot_id;
	param.op_params.end_job.response = response;
	param.op_params.end_job.response_len = response_len;
	param.op_params.end_job.shm_response_key = -1;

	ret = ioctl_send(IOCTL_END_JOB_SERVICE, &param);
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("EndJob: %s does not exists\n", service_uri);
			break;
		case LINBER_KILL_WORKER:
			printf("EndJob: fatal error, pls kill the worker %d\n", worker_id);
			break;
		case LINBER_SERVICE_SKIP_JOB:
			printf("EndJob: Skip job, Spurious wakeup for workerd %d\n", worker_id);
			break;
	}
	free(request);
	free(response);
	return ret;
}

int linber_end_job_service_shm(char *service_uri, unsigned int uri_len, int service_id, unsigned long service_token, unsigned int worker_id,\
							unsigned int slot_id,
							char *request, char *response, int response_len, char *file_str){
	int ret = 0, shmid;
	static int low_id = 0;
	key_t key;
	linber_service_struct param;
	char *shm_mode;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.end_job.service_id = service_id;
	param.op_params.end_job.worker_id = worker_id;
	param.op_params.end_job.service_token = service_token;
	param.op_params.end_job.slot_id = slot_id;
	param.op_params.end_job.response = response;
	param.op_params.end_job.response_len = response_len;
	if((key = ftok(file_str, low_id++)) == (key_t) -1) {
		printf("IPC error: ftok");
		return -1;
	}
	param.op_params.end_job.shm_response_key = key;

	if((shmid = shmget(key, 1<<22, IPC_CREAT | 0666)) < 0) {
		printf("shmget");
		return -1;
	}

	printf("key: %d, id: %d \n" , key, shmid);

	if((shm_mode = (char*)shmat(shmid, NULL, 0)) == (char *) -1) {
		printf("shmat");
		return -1;
	}
//	memcpy(shm_mode, response, response_len);

	ret = ioctl_send(IOCTL_END_JOB_SERVICE, &param);

	if(shmdt(shm_mode)) {
		perror("shmop: shmdt failed");
	}

	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("EndJob: %s does not exists\n", service_uri);
			break;
		case LINBER_KILL_WORKER:
			printf("EndJob: fatal error, pls kill the worker %d\n", worker_id);
			break;
		case LINBER_SERVICE_SKIP_JOB:
			printf("EndJob: Skip job, Spurious wakeup for workerd %d\n", worker_id);
			break;
	}
	free(request);
	free(response);
	return ret;
}

int linber_destroy_service(char *service_uri, unsigned int uri_len, unsigned long service_token){
	int ret = 0;
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.destroy_service.service_token = service_token;
	ret = ioctl_send(IOCTL_DESTROY_SERVICE, &param);
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Destroy: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_system_status(system_status *system){
	int ret = 0;
	ret = ioctl_send(IOCTL_SYSTEM_STATUS, system);
	return ret;
}


#endif