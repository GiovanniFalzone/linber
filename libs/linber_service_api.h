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
#include <errno.h>
#include "linber_ioctl.h"

#define LINBER_ERROR_DEVICE_FILE	-1
#define LINBER_ERROR_URI			-2
#define LINBER_SHM_ERROR			-3

//#define DEBUG

int linber_init();
int linber_exit();
int linber_register_service(char *service_uri, unsigned int uri_len, int *service_id, unsigned int exec_time, unsigned int max_workers, unsigned long *service_token);
int linber_register_service_worker(char *service_uri, unsigned int uri_len, unsigned long service_token, unsigned int * worker_id, char **file_str);
void linber_destroy_worker(char *file_str);

char *create_shm_from_key(key_t key, int len, int *id);
char *create_shm_from_filepath(char* file_str, int low_id, int len, key_t *key, int *id);
void detach_shm(void *addr);

char* attach_shm_from_key(key_t key, int len, int *id);

int linber_request_service(char *service_uri, unsigned int uri_len, unsigned int rel_deadline,\
							char *request, int request_len,\
							char **response, int *response_len,
							boolean *response_shm_mode
							);

int linber_request_service_shm(char *service_uri, unsigned int uri_len, unsigned int rel_deadline,\
							key_t request_key, int request_len,\
							char **response, int *response_len,
							boolean *response_shm_mode	
							);

int linber_request_service_no_blocking(char *service_uri, unsigned int uri_len, unsigned int rel_deadline,\
										char *request, int request_len,\
										unsigned long *ptr_token);

int linber_request_service_no_blocking_shm(char *service_uri, unsigned int uri_len, unsigned int rel_deadline,\
										key_t request_key, int request_len,\
										unsigned long *ptr_token);

int linber_request_service_get_response(char *service_uri, unsigned int uri_len,	\
										char **response, int *response_len,			\
										boolean *response_shm_mode,					\
										unsigned long *ptr_token
										);

void linber_request_service_clean(char *request, boolean shm_request_mode, char *response, boolean shm_response_mode );

int linber_start_job_service(	char *service_uri, unsigned int uri_len,		\
								int service_id, unsigned long service_token,	\
								unsigned int worker_id,							\
								char **request, int *request_len, 				\
								boolean *request_shm_mode);

int linber_end_job_service(	char *service_uri, unsigned int uri_len,			\
							int service_id, unsigned long service_token,		\
							unsigned int worker_id,								\
							char *request, boolean request_shm_mode,			\
							char *response, int response_len);

int linber_end_job_service_shm(	char *service_uri, unsigned int uri_len,		\
								int service_id, unsigned long service_token,	\
								unsigned int worker_id,							\
								char *request, boolean request_shm,				\
								char *response, int response_len, key_t response_key);

int linber_destroy_service(char *service_uri, unsigned int uri_len, unsigned long service_token);

int linber_system_status(system_status *system);

#endif