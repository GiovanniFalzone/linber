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
		ret = -errno;
		switch(ret){
			case LINBER_IOCTL_NOT_IMPLEMENTED:
				printf("IOCTL Operation %d not implemented\n", cmd);
				break;

			case LINBER_PAYLOAD_SIZE_ERROR:
				printf("Payload too large, use shared memory\n");
				break;

			case LINBER_USER_MEMORY_ERROR:
				printf("Error on copy between Kernel and User space\n");
				break;
			case LINBER_SERVICE_NOT_EXISTS:
				printf("RegisterWorker: Service %s does not exists\n", ((linber_service_struct*)param)->service_uri);
				break;

		}
	}
	return ret;
}

/*---------------------------------------------------------------------------------------------
	Server operations
---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------
	Create a service that uses kernel buffer memory to store the response.
	With a maximum number of workers.
	For Real Time requests will use the period and budget passed here as parameters.
	To operate on the service this function store in the token pointer the token of the service
	each worker have to use this token.
	The time parameters are considered in milliseconds and converted in nanoseconds
---------------------------------------------------------------------------------------------*/
int linber_register_service(	char *service_uri,					\
								unsigned int uri_len,				\
								unsigned int exec_time,				\
								unsigned int max_workers,			\
								unsigned long *service_token,		\
								unsigned int service_period,		\
								unsigned int Request_Per_Period
							){
	int ret = 0;
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.registration.Max_Working = max_workers;
	param.op_params.registration.ptr_service_token = service_token;

	param.op_params.registration.exec_time_ns = mSEC_TO_nSEX(exec_time);
	param.op_params.registration.exec_time_ns += (param.op_params.registration.exec_time_ns*5)/100;	// add 5% to declared exec time
	param.op_params.registration.service_period_ns = mSEC_TO_nSEX(service_period);
	param.op_params.registration.service_budget_ns = param.op_params.registration.exec_time_ns * Request_Per_Period;

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

/*---------------------------------------------------------------------------------------------
	Register a worker for the service identified by the uri string, needs to pass the
	service token received during service registration.
	Create a file in /tmp associated with the service and the worker in order to create a 
	unique key in case of response in shared memory.
---------------------------------------------------------------------------------------------*/
int linber_register_service_worker(	char *service_uri,				\
									unsigned int uri_len,			\
									unsigned long service_token,	\
									unsigned int * worker_id,		\
									char **file_str){
	int ret = 0, fd, file_str_len;
	char worker_id_str[4];
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.register_worker.ptr_worker_id = worker_id;
	param.op_params.register_worker.service_token = service_token;
	ret = ioctl_send(IOCTL_REGISTER_SERVICE_WORKER, &param);

	if(ret == 0){
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
	return ret;
}

/*---------------------------------------------------------------------------------------------
	destroy procedure when the worker ends
---------------------------------------------------------------------------------------------*/
void linber_destroy_worker(char *file_str){
	if(remove(file_str) == 0){
		printf("worker destroyied\n"); 
	} else {
		printf("Error on worker destroy\n");
	}
	free(file_str);
}

/*---------------------------------------------------------------------------------------------
	Create a shared memory, return the address and id of the shared memory
---------------------------------------------------------------------------------------------*/
inline char *create_shm_from_key(key_t key, int len, int *id){
//	struct shmid_ds shmid_ds;
	char* shm;
	if((*id = shmget(key, len, IPC_CREAT | 0666)) >= 0) {
/*
	I can set the permission to the user that will use the shared memory.
	Passing the uid and gid of the Destination user
	Service's workers are running as root so no problem
	Users instead must pass the uid and gid using the request
	or I have to change them inside the kernel
	Another way could be that every service create a group for the service and
	each Client user and root must be part of the group
*/
//		shmid_ds.shm_perm.uid
//		shmid_ds.shm_perm.gid
//		shmid_ds.shm_perm.mode = 0660;
//		shmctl(*id, IPC_SET, &shmid_ds);
} else {
		perror("shmget error\n");
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

/*---------------------------------------------------------------------------------------------
	Used to create a shared memory starting from a dedicated worker file in /tmp and a low id
---------------------------------------------------------------------------------------------*/
char *create_shm_from_filepath(char* file_str, int low_id, int len, key_t *key, int *id){
	low_id = (low_id + 1)%255;
	if((*key = ftok(file_str, low_id)) == (key_t) -1) {
		printf("IPC error: ftok\n");
		return NULL;
	}
	return create_shm_from_key(*key, len, id);
}

void detach_shm(void *addr){
	if(shmdt(addr)) {
		perror("shmop: shmdt failed\n");
	}
}

char* attach_shm_from_key(key_t key, int len, int *id){
	char *shm;

	if((*id = shmget(key, len, 0666)) < 0) {
		printf("shmget error key:%d len:%d, id:%d\n", key, len, *id);
		perror("Errno:\n");
		return NULL;
	} else {
		if ((shm = (char*)shmat(*id, NULL, 0)) == (char *) -1) {
			printf("shmat error key:%d len:%d, id:%d\n", key, len, *id);
			perror("Errno:\n");
			return NULL;
		}
	}
	return shm;
}

/*---------------------------------------------------------------------------------------------
	Send a request to the service identified by uri, pass a request that will be copied in a 
	kernel buffer.
	Is a blocking procedure and will return the response in shared memory or in the response
	buffer.
	The relative deadline is considered in milliseconds and then converted in nanoseconds,
	if 0 then a best effort policy will be applied for the request.
---------------------------------------------------------------------------------------------*/
int linber_request_service(	char *service_uri,				\
							unsigned int uri_len,			\
							unsigned int rel_deadline_ms,	\
							char *request,					\
							int request_len,				\
							char **response,				\
							int *response_len,				\
							boolean *response_shm_mode		\
							){
	int ret = 0, response_shm_id;
	unsigned long token;
	linber_service_struct param;
	key_t response_key;
	struct timespec now;

	// we consider the boot time and 64bit are enough to express the abs deadline in ns
	if(clock_gettime(CLOCK_MONOTONIC, &now) == -1){
		perror("Errno: clock gettime error\n");
		rel_deadline_ms = 0;
	}

	param.op_params.request.blocking = TRUE;;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;

	if(rel_deadline_ms == 0){
		param.op_params.request.abs_deadline_ns = -1; // -1 for an unsigned is converted to the maximum value
	} else {
		param.op_params.request.abs_deadline_ns = SEC_TO_nSEC(now.tv_sec);
		param.op_params.request.abs_deadline_ns += now.tv_nsec;
		param.op_params.request.abs_deadline_ns += mSEC_TO_nSEX(rel_deadline_ms);
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
			#ifdef DEBUG
				printf("response in shared memory, key:%d\n", response_key);
			#endif
			*response = attach_shm_from_key(response_key, *response_len, &response_shm_id);
			if(response == NULL){
				return LINBER_REQUEST_FAILED;				
			}
// self destroy when the client will detach the response the shared memory will be destroied
			shmctl(response_shm_id, IPC_RMID, NULL);
		} else {
			*response = (char*)malloc(*response_len);
			if(response != NULL){
				param.op_params.request.ptr_response = *response;
				ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param);
			} else {
				ret = LINBER_REQUEST_FAILED;
			}
		}
	} else {
		*response = NULL;
	}
	return ret;
}

/*---------------------------------------------------------------------------------------------
	Send a request to the service identified by uri, pass a request in shared memory
	Is a blocking procedure and will return the response in shared memory or in the response
	buffer.
	The relative deadline is considered in milliseconds and then converted in nanoseconds,
	if 0 then a best effort policy will be applied for the request.
---------------------------------------------------------------------------------------------*/
int linber_request_service_shm(	char *service_uri,				\
								unsigned int uri_len,			\
								unsigned int rel_deadline_ms,	\
								key_t request_key, 				\
								int request_len,				\
								char **response,				\
								int *response_len,				\
								boolean *response_shm_mode		\
							){
	int ret = 0, response_shm_id;
	unsigned long token;
	linber_service_struct param;
	key_t response_key;
	struct timespec now;

	// we consider the boot time and 64bit are enough to express the abs deadline in ns
	if(clock_gettime(CLOCK_MONOTONIC, &now) == -1){
		perror("Errno: clock gettime error\n");
		rel_deadline_ms = 0;
	}

	param.op_params.request.blocking = TRUE;;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;

	if(rel_deadline_ms == 0){
		param.op_params.request.abs_deadline_ns = -1; // -1 for an unsigned is converted to the maximum value
	} else {
		param.op_params.request.abs_deadline_ns = SEC_TO_nSEC(now.tv_sec);
		param.op_params.request.abs_deadline_ns += now.tv_nsec;
		param.op_params.request.abs_deadline_ns += mSEC_TO_nSEX(rel_deadline_ms);;
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
			#ifdef DEBUG
				printf("response in shared memory, key:%d\n", response_key);
			#endif
			*response = attach_shm_from_key(response_key, *response_len, &response_shm_id);
			if(response == NULL){
				return LINBER_REQUEST_FAILED;				
			}
// self destroy when the client will detach the response the shared memory will be destroied
			shmctl(response_shm_id, IPC_RMID, NULL);
		} else {
			*response = (char*)malloc(*response_len);
			if(response != NULL){
				param.op_params.request.ptr_response = *response;
				ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param);
			} else {
				ret = LINBER_REQUEST_FAILED;
			}
		}
	} else {
		*response = NULL;
	}
	return ret;
}

/*---------------------------------------------------------------------------------------------
	Send a request to the service identified by uri, pass a request that will be copied in a 
	kernel buffer.
	Is a NON blocking procedure and return the token associated with the request in the token_ptr.
	The relative deadline is considered in milliseconds and then converted in nanoseconds,
	if 0 then a best effort policy will be applied for the request.
	Call the linber_request_service_get_response to retrieve the response
---------------------------------------------------------------------------------------------*/
int linber_request_service_no_blocking(	char *service_uri, 				\
										unsigned int uri_len, 			\
										unsigned int rel_deadline_ms,	\
										char *request, 					\
										int request_len,				\
										unsigned long *ptr_token,		\
										unsigned long *abs_deadline		\
									){
	int ret = 0;
	linber_service_struct param;
	struct timespec now;

	// we consider the boot time and 64bit are enough to express the abs deadline in ns
	if(clock_gettime(CLOCK_MONOTONIC, &now) == -1){
		perror("Errno: clock gettime error\n");
		rel_deadline_ms = 0;
	}

	param.op_params.request.blocking = FALSE;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;

	if(rel_deadline_ms == 0){
		*abs_deadline = -1; // -1 for an unsigned is converted to the maximum value
	} else {
		*abs_deadline = SEC_TO_nSEC(now.tv_sec);
		*abs_deadline += now.tv_nsec;
		*abs_deadline += mSEC_TO_nSEX(rel_deadline_ms);;
	}
	param.op_params.request.abs_deadline_ns = *abs_deadline;

	param.op_params.request.ptr_token = ptr_token;
	param.op_params.request.status = LINBER_REQUEST_INIT;
	param.op_params.request.request_len = request_len;
	param.op_params.request.request_shm_mode = FALSE;
	param.op_params.request.request.data = request;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);
	return ret;
}

/*---------------------------------------------------------------------------------------------
	Send a request to the service identified by uri, pass a request in shared memory.
	Is a NON blocking procedure and return the token associated with the request in the token_ptr.
	The relative deadline is considered in milliseconds and then converted in nanoseconds,
	if 0 then a best effort policy will be applied for the request.
	Call the linber_request_service_get_response to retrieve the response.
---------------------------------------------------------------------------------------------*/
int linber_request_service_no_blocking_shm(	char *service_uri, 				\
											unsigned int uri_len, 			\
											unsigned int rel_deadline_ms,	\
											key_t request_key, 				\
											int request_len,				\
											unsigned long *ptr_token,		\
											unsigned long *abs_deadline		\
										){
	int ret = 0;
	linber_service_struct param;
	struct timespec now;

	// we consider the boot time and 64bit are enough to express the abs deadline in ns
	if(clock_gettime(CLOCK_MONOTONIC, &now) == -1){
		perror("Errno: clock gettime error\n");
		rel_deadline_ms = 0;
	}

	param.op_params.request.blocking = FALSE;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;

	if(rel_deadline_ms == 0){
		*abs_deadline = -1; // -1 for an unsigned is converted to the maximum value
	} else {
		*abs_deadline = SEC_TO_nSEC(now.tv_sec);
		*abs_deadline += now.tv_nsec;
		*abs_deadline += mSEC_TO_nSEX(rel_deadline_ms);;
	}
	param.op_params.request.abs_deadline_ns = *abs_deadline;


	param.op_params.request.ptr_token = ptr_token;
	param.op_params.request.status = LINBER_REQUEST_INIT;

	param.op_params.request.request_len = request_len;
	param.op_params.request.request_shm_mode = TRUE;
	param.op_params.request.request.shm_key = request_key;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);
	return ret;
}

/*---------------------------------------------------------------------------------------------
	Retrieve the response associated to the request with the passed token and deadline.
	It is a blocking procedure.
	Will return a response in shared memory or in a user space memory buffer.
---------------------------------------------------------------------------------------------*/
int linber_request_service_get_response(	char *service_uri, 					\
											unsigned int uri_len,				\
											char **response, 					\
											int *response_len,					\
											boolean *response_shm_mode,			\
											unsigned long *ptr_token,			\
											unsigned long abs_deadline			\
										){
	int ret = 0, response_shm_id;
	linber_service_struct param;
	key_t response_key;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.request.ptr_token = ptr_token;
	param.op_params.request.abs_deadline_ns = abs_deadline;
	param.op_params.request.status = LINBER_REQUEST_WAITING;

	param.op_params.request.ptr_response_len = response_len;
	param.op_params.request.ptr_response_shm_mode = response_shm_mode;
	param.op_params.request.ptr_shm_response_key = &response_key;

	ret = ioctl_send(IOCTL_REQUEST_SERVICE, &param);
	if(ret == LINBER_REQUEST_SUCCESS){
		if(*response_shm_mode) {
			#ifdef DEBUG
				printf("response in shared memory, key:%d\n", response_key);
			#endif
			*response = attach_shm_from_key(response_key, *response_len, &response_shm_id);
			if(response == NULL){
				return LINBER_REQUEST_FAILED;				
			}
// self destroy when the client will detach the response the shared memory will be destroied
			shmctl(response_shm_id, IPC_RMID, NULL);
		} else {
			*response = (char*)malloc(*response_len);
			if(response != NULL){
				param.op_params.request.ptr_response = *response;
				ret = ioctl_send(IOCTL_REQUEST_SERVICE_GET_RESPONSE, &param);
			} else {
				ret = LINBER_REQUEST_FAILED;
			}
		}
	} else {
		*response = NULL;
	}
	return ret;
}

/*---------------------------------------------------------------------------------------------
	Call this after using the response. It will clean the created buffers or the shared
	memory associated with the response.
---------------------------------------------------------------------------------------------*/
void linber_request_service_clean(char *response, boolean shm_response_mode){
	if(shm_response_mode){
		#ifdef DEBUG
			printf("response in shared memory detached\n");
		#endif
		detach_shm(response);
	} else {
		free(response);
	}
}

/*---------------------------------------------------------------------------------------------
	It is a blocking procedure, blocks the worker waiting for requests.
	Return a negative value in case of errors, pls kill the worker in that case.
	Will pass the request in the request pointer as user space memory buffer or shared memory 
---------------------------------------------------------------------------------------------*/
int linber_start_job_service(	char *service_uri, 								\
								unsigned int uri_len,							\
								unsigned long service_token,					\
								unsigned int worker_id,							\
								char **request, 								\
								int *request_len, 								\
								boolean *request_shm_mode						\
							){
	int ret = 0, request_shm_id;
	key_t request_key;
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.start_job.worker_id = worker_id;
	param.op_params.start_job.service_token = service_token;
	param.op_params.start_job.ptr_request_len = request_len;
	param.op_params.start_job.ptr_request_shm_mode = request_shm_mode;
	param.op_params.start_job.data.ptr_request_key = &request_key;
	ret = ioctl_send(IOCTL_START_JOB_SERVICE, &param);
	if(ret == 0){
		if(*request_shm_mode){
			#ifdef DEBUG
				printf("request in shared memory, key:%d\n", request_key);
			#endif
			*request = attach_shm_from_key(request_key, *request_len, &request_shm_id);
			if(request == NULL){
				return LINBER_SERVICE_SKIP_JOB;
			}
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
		case LINBER_KILL_WORKER:
			printf("StartJob: fatal error, pls kill the worker %d token:%lu\n", worker_id, service_token);
			break;
		case LINBER_SERVICE_SKIP_JOB:
			printf("StartJob: Skip job, Spurious wakeup for workerd %d\n", worker_id);
			break;
	}
	return ret;
}

/*---------------------------------------------------------------------------------------------
	End Job, pass the response in user space memory buffer to the module.
	Call always after a Start job and after have used the request, will clean the memory used
	to store the request.
---------------------------------------------------------------------------------------------*/
int linber_end_job_service(	char *service_uri, 				\
							unsigned int uri_len,			\
							unsigned long service_token,	\
							unsigned int worker_id,			\
							char *request, 					\
							boolean request_shm_mode,		\
							char *response, 				\
							int response_len				\
						){
	int ret = 0;
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
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

	switch(ret){
		case LINBER_KILL_WORKER:
			printf("EndJob: fatal error, pls kill the worker %d\n", worker_id);
			break;
		case LINBER_SERVICE_SKIP_JOB:
			printf("EndJob: Skip job, Spurious wakeup for workerd %d\n", worker_id);
			break;
	}

	return ret;
}

/*---------------------------------------------------------------------------------------------
	End Job, pass the response in shared memory to the module.
	Call always after a Start job and after have used the request, will clean the memory used
	to store the request.
---------------------------------------------------------------------------------------------*/
int linber_end_job_service_shm(	char *service_uri, 				\
								unsigned int uri_len,			\
								unsigned long service_token,	\
								unsigned int worker_id,			\
								char *request, 					\
								boolean request_shm,			\
								int response_len, 				\
								key_t response_key				\
							){
	int ret = 0;
	linber_service_struct param;

	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
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

	switch(ret){
		case LINBER_KILL_WORKER:
			printf("EndJob: fatal error, pls kill the worker %d\n", worker_id);
			break;
		case LINBER_SERVICE_SKIP_JOB:
			printf("EndJob: Skip job, Spurious wakeup for workerd %d\n", worker_id);
			break;
	}
	return ret;
}

/*---------------------------------------------------------------------------------------------
	Start the destroying procedure
---------------------------------------------------------------------------------------------*/
int linber_destroy_service(char *service_uri, unsigned int uri_len, unsigned long service_token){
	int ret = 0;
	linber_service_struct param;
	param.service_uri = service_uri;
	param.service_uri_len = uri_len;
	param.op_params.destroy_service.service_token = service_token;
	ret = ioctl_send(IOCTL_DESTROY_SERVICE, &param);
	return ret;
}

/*---------------------------------------------------------------------------------------------
	Receive the system status and the status of each service in the system_status structure pointer
---------------------------------------------------------------------------------------------*/
int linber_system_status(system_status *system){
	int ret = 0;
	ret = ioctl_send(IOCTL_SYSTEM_STATUS, system);
	return ret;
}
