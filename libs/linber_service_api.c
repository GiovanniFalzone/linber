#include <sys/time.h>
#include "linber_service_api.h"

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
	long ret;
	if(linber_fd < 0){
		close_it = TRUE;
		linber_init();
		if(linber_fd < 0){
			printf("Device file error\n");
			return LINBER_ERROR_DEVICE_FILE;
		}
	}
	ret = ioctl(linber_fd, cmd, param);
	if(close_it){
		linber_exit();
	}
	if(ret < 0){
		printf("------------Linber err %li----------------\n", ret);
		switch(ret){
			case LINBER_IOCTL_NOT_IMPLEMENTED:
				printf("IOCTL Operation %d not implemented\n", cmd);
				break;

			case LINBER_PAYLOAD_SIZE_ERROR:
				printf("Payload too large, use shared memory\n");
				break;

			default:
				printf("unspecified error\n");
				break;
		}
	}
	return ret;
}

//------------------------------------------------
int linber_register_service(char *service_uri, unsigned int uri_len, int *service_id, unsigned int exec_time, unsigned int max_workers, unsigned long *service_token){
	int ret = 0;
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.registration.exec_time_ns = exec_time*1000000;
	param.op_params.registration.Max_Working = max_workers;
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
			return LINBER_KILL_WORKER;
		}
		sprintf(worker_id_str, "%d", *worker_id);
		file_str_len = uri_len + strlen("/tmp/_99\0");
		*file_str = (char*)malloc(file_str_len);
		sprintf(*file_str, "%s", "/tmp/\0");
		strcat(*file_str, service_uri);
		strcat(*file_str, "_\0");
		strcat(*file_str, worker_id_str);
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

void linber_destroy_worker(char *file_str){
	if(remove(file_str) == 0){
		printf("worker destroyied\n"); 
	} else {
		printf("Error on worker destroy\n");
	}
	free(file_str);
}

//------------------------------------------------------
char *create_shm_from_key(key_t key, int len, int *id){
	char* shm;
	if((*id = shmget(key, len, IPC_CREAT | 0666)) < 0) {
		printf("shmget error %d\n", *id);
		switch(*id){
			case -EACCES:
				printf("The user does not have permission to access the shared memory segment, and does not have the CAP_IPC_OWNER capability in the user namespace that governs its IPC namespace.\n");
				break;

			case -EEXIST:
				printf("IPC_CREAT  and  IPC_EXCL  were  specified in shmflg, but a shared memory segment already exists for key.\n");
				break;

			case -EINVAL:
				printf("A new segment was to be created and size is less than SHMMIN or greater than SHMMAX.\n");
				printf("A segment for the given key exists, but size is greater than the size of that segment.\n");
				break;

			case -ENFILE:
				printf("The system-wide limit on the total number of open files has been reached.\n");
				break;

			case -ENOENT:
				printf("No segment exists for the given key, and IPC_CREAT was not specified.\n");
				break;

			case -ENOMEM:
				printf("No memory could be allocated for segment overhead.\n");
				break;

			case -ENOSPC:
				printf("All possible shared memory IDs have been taken (SHMMNI), or allocating a segment of  the requested  size  would cause the system to exceed the system-wide limit on shared memory (SHMALL).\n");
				break;

      		case -EPERM:
				printf("The SHM_HUGETLB flag was specified, but the caller was not privileged (did not have  the CAP_IPC_LOCK capability).\n");
				break;
			default:
				printf("undefined error\n");
				break;
		}
		return NULL;
	}

	#ifdef DEBUG
		printf("New shm key: %d, id: %d \n" , key, *id);
	#endif 

	if((shm = (char*)shmat(*id, NULL, 0)) == (char *) -1) {
		printf("shmat error\n");
		return NULL;
	}

	return shm;
}

char *create_shm_from_filepath(char* file_str, int len, key_t *key, int *id){
	static int low_id = 0;
	if((*key = ftok(file_str, low_id)) == (key_t) -1) {
		printf("IPC error: ftok\n");
		return NULL;
	}
	low_id = (low_id + 1)%255;
	return create_shm_from_key(*key, len, id);
}

void detach_shm(void *addr){
	if(shmdt(addr)) {
		perror("shmop: shmdt failed\n");
	}
}

char* attach_shm_from_key(key_t key, int len, int *id){
	char *shm;

	if ((*id = shmget(key, len, 0666)) < 0) {
		printf("shmget error\n");
		return NULL;
	}
	if ((shm = (char*)shmat(*id, NULL, 0)) == (char *) -1) {
		printf("shmat error\n");
		return NULL;
	}
	return shm;
}
//------------------------------------------------------
int linber_request_service(char *service_uri, unsigned int uri_len, unsigned int rel_deadline_ms,\
							char *request, int request_len,\
							char **response, int *response_len,
							boolean *response_shm_mode
							){
	int ret = 0, response_shm_id;
	unsigned long token;
	linber_service_struct param;
	key_t response_key;
	struct timeval now;
	gettimeofday(&now, NULL);

	param.op_params.request.blocking = TRUE;;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;

	if(rel_deadline_ms == 0){
		param.op_params.request.abs_deadline_ns = -1; // 2^64 - 1
	} else {
		param.op_params.request.abs_deadline_ns = (now.tv_usec + rel_deadline_ms*1000)*1000;
	}

	param.op_params.request.ptr_token = &token;
	param.op_params.request.status = LINBER_REQUEST_INIT;
	param.op_params.request.request_len = request_len;
	param.op_params.request.request_shm_mode = FALSE;
	param.op_params.request.request.data = request;

	param.op_params.request.ptr_response_len = response_len;
	param.op_params.request.ptr_response_shm_mode = response_shm_mode;
	param.op_params.request.ptr_shm_response_key = &response_key;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);

	if(ret == LINBER_REQUEST_SUCCESS){
		if(*response_shm_mode) {
			if((ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param) < 0)){
				return LINBER_REQUEST_FAILED;
			}
			#ifdef DEBUG
				printf("response in shared memory, key:%d\n", response_key);
			#endif
			*response = attach_shm_from_key(response_key, *response_len, &response_shm_id);
			shmctl(response_shm_id, IPC_RMID, NULL);	// self destroy

		} else {
			*response = (char*)malloc(*response_len);
			if(response != NULL){
				param.op_params.request.ptr_response = *response;
				ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param);
			} else {
				ret = LINBER_REQUEST_FAILED;
			}
		}
	}
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_request_service_shm(char *service_uri, unsigned int uri_len, unsigned int rel_deadline_ms,\
							key_t request_key, int request_len,\
							char **response, int *response_len,
							boolean *response_shm_mode	
							){
	int ret = 0, response_shm_id;
	unsigned long token;
	linber_service_struct param;
	key_t response_key;
	struct timeval now;
	gettimeofday(&now, NULL);

	param.op_params.request.blocking = TRUE;;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;

	if(rel_deadline_ms == 0){
		param.op_params.request.abs_deadline_ns = -1; // 2^64 - 1
	} else {
		param.op_params.request.abs_deadline_ns = now.tv_usec + rel_deadline_ms*1000;
	}

	param.op_params.request.ptr_token = &token;
	param.op_params.request.status = LINBER_REQUEST_INIT;

	param.op_params.request.request_len = request_len;
	param.op_params.request.request_shm_mode = TRUE;
	param.op_params.request.request.shm_key = request_key;

	param.op_params.request.ptr_response_len = response_len;
	param.op_params.request.ptr_response_shm_mode = response_shm_mode;
	param.op_params.request.ptr_shm_response_key = &response_key;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);

	if(ret == LINBER_REQUEST_SUCCESS){
		if(*response_shm_mode) {
			if((ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param) < 0)){
				return LINBER_REQUEST_FAILED;
			}
			#ifdef DEBUG
				printf("response in shared memory, key:%d\n", response_key);
			#endif
			*response = attach_shm_from_key(response_key, *response_len, &response_shm_id);
			shmctl(response_shm_id, IPC_RMID, NULL);	// self destroy
		} else {
			*response = (char*)malloc(*response_len);
			if(response != NULL){
				param.op_params.request.ptr_response = *response;
				ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param);
			} else {
				ret = LINBER_REQUEST_FAILED;
			}
		}
	}

	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_request_service_no_blocking(char *service_uri, unsigned int uri_len, unsigned int rel_deadline_ms,\
										char *request, int request_len,\
										unsigned long *ptr_token){
	int ret = 0;
	linber_service_struct param;
	struct timeval now;
	gettimeofday(&now, NULL);

	param.op_params.request.blocking = FALSE;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;

	if(rel_deadline_ms == 0){
		param.op_params.request.abs_deadline_ns = -1; // 2^64 - 1
	} else {
		param.op_params.request.abs_deadline_ns = now.tv_usec + rel_deadline_ms*1000;
	}

	param.op_params.request.ptr_token = ptr_token;
	param.op_params.request.status = LINBER_REQUEST_INIT;
	param.op_params.request.request_len = request_len;
	param.op_params.request.request_shm_mode = FALSE;
	param.op_params.request.request.data = request;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_request_service_no_blocking_shm(char *service_uri, unsigned int uri_len, unsigned int rel_deadline_ms,\
										key_t request_key, int request_len,\
										unsigned long *ptr_token){
	int ret = 0;
	linber_service_struct param;
	struct timeval now;
	gettimeofday(&now, NULL);

	param.op_params.request.blocking = FALSE;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;

	if(rel_deadline_ms == 0){
		param.op_params.request.abs_deadline_ns = -1; // 2^64 - 1
	} else {
		param.op_params.request.abs_deadline_ns = now.tv_usec + rel_deadline_ms*1000;
	}

	param.op_params.request.ptr_token = ptr_token;
	param.op_params.request.status = LINBER_REQUEST_INIT;

	param.op_params.request.request_len = request_len;
	param.op_params.request.request_shm_mode = TRUE;
	param.op_params.request.request.shm_key = request_key;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

int linber_request_service_get_response(char *service_uri, unsigned int uri_len,	\
										char **response, int *response_len,			\
										boolean *response_shm_mode,					\
										unsigned long *ptr_token
										){
	int ret = 0, response_shm_id;
	linber_service_struct param;
	key_t response_key;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.request.ptr_token = ptr_token;
	param.op_params.request.status = LINBER_REQUEST_WAITING;

	param.op_params.request.ptr_response_len = response_len;
	param.op_params.request.ptr_response_shm_mode = response_shm_mode;
	param.op_params.request.ptr_shm_response_key = &response_key;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);
	if(ret == LINBER_REQUEST_SUCCESS){
		if(*response_shm_mode) {
			if((ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param) < 0)){
				return LINBER_REQUEST_FAILED;
			}
			#ifdef DEBUG
				printf("response in shared memory, key:%d\n", response_key);
			#endif
			*response = attach_shm_from_key(response_key, *response_len, &response_shm_id);
			shmctl(response_shm_id, IPC_RMID, NULL);	// self destroy
		} else {
			*response = (char*)malloc(*response_len);
			if(response != NULL){
				param.op_params.request.ptr_response = *response;
				ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param);
			} else {
				ret = LINBER_REQUEST_FAILED;
			}
		}
	}
	switch(ret){
		case LINBER_SERVICE_NOT_EXISTS:
			printf("Request: Service %s does not exists\n", service_uri);
			break;
	}
	return ret;
}

void linber_request_service_clean(char *request, boolean shm_request_mode, char *response, boolean shm_response_mode ){
	if(shm_request_mode){
		detach_shm(request);
	} else {
		free(request);
	}

	if(shm_response_mode){
		detach_shm(response);
	} else {
		free(response);
	}
}

//-----------------------------------------------------------------------
int linber_start_job_service(	char *service_uri, unsigned int uri_len,		\
								int service_id, unsigned long service_token,	\
								unsigned int worker_id,	\
								char **request, int *request_len, 				\
								boolean *request_shm_mode){
	int ret = 0, request_shm_id;
	key_t request_key;
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.start_job.service_id = service_id;
	param.op_params.start_job.worker_id = worker_id;
	param.op_params.start_job.service_token = service_token;
	param.op_params.start_job.ptr_request_len = request_len;
	param.op_params.start_job.ptr_request_shm_mode = request_shm_mode;
	param.op_params.start_job.data.ptr_request_key = &request_key;
	ret = ioctl_send(IOCTL_START_JOB_SERVICE, &param);
	if(ret >= 0){
		if(*request_shm_mode){
			#ifdef DEBUG
				printf("request in shared memory, key:%d\n", request_key);
			#endif
			*request = attach_shm_from_key(request_key, *request_len, &request_shm_id);
			shmctl(request_shm_id, IPC_RMID, NULL);	// self destroy
		} else {
			*request = (char*)malloc(*request_len);
			param.op_params.start_job.data.ptr_request = *request;
			ret = ioctl_send(IOCTL_START_JOB_GET_REQUEST_SERVICE, &param);
		}
		if(*request == NULL){
			return LINBER_SERVICE_SKIP_JOB;
		}
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

int linber_end_job_service(	char *service_uri, unsigned int uri_len,			\
							int service_id, unsigned long service_token,		\
							unsigned int worker_id,		\
							char *request, boolean request_shm_mode,			\
							char *response, int response_len){
	int ret = 0;
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.end_job.service_id = service_id;
	param.op_params.end_job.worker_id = worker_id;
	param.op_params.end_job.service_token = service_token;
	param.op_params.end_job.response_len = response_len;
	param.op_params.end_job.response_shm_mode = FALSE;
	param.op_params.end_job.response.data = response;

	ret = ioctl_send(IOCTL_END_JOB_SERVICE, &param);
	if(request_shm_mode){
		detach_shm(request);
	} else {
		free(request);
	}
	free(response);

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

	return ret;
}

int linber_end_job_service_shm(	char *service_uri, unsigned int uri_len,		\
								int service_id, unsigned long service_token,	\
								unsigned int worker_id,	\
								char *request, boolean request_shm,				\
								char *response, int response_len, key_t response_key){
	int ret = 0;
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.end_job.service_id = service_id;
	param.op_params.end_job.worker_id = worker_id;
	param.op_params.end_job.service_token = service_token;
	param.op_params.end_job.response_len = response_len;
	param.op_params.end_job.response_shm_mode = TRUE;
	param.op_params.end_job.response.shm_key = response_key;

	ret = ioctl_send(IOCTL_END_JOB_SERVICE, &param);

	if(request_shm){
		detach_shm(request);
	} else {
		free(request);
	}
	detach_shm(response);

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
