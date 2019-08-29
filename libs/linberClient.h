#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <pthread.h>
#include "linber_service_api.h"

class linberClient {
	char *service_uri;
	int service_uri_len;
	int request_len;
	int response_len;
	char* request = NULL;
	char * response = NULL;
	key_t request_shm_key;
	boolean response_shm_mode = FALSE;
	boolean request_shm_mode = FALSE;
	unsigned long token;
	int request_state = 0, request_shm_state = -1;

	public:
	linberClient(char * service_uri, int service_uri_len){
		this->service_uri = service_uri;
		this->service_uri_len = service_uri_len;
		linber_init();
	}

	~linberClient(){
		linber_exit();
	}

	void linber_sendRequest(char *req, int req_len, char **res, int *res_len, bool blocking){
		int ret;
		if((request_state == 0) && (request_shm_state == -1)){
			this->request_shm_mode = FALSE;
			this->request = req;
			this->request_len = req_len;
			if(blocking){
				ret = linber_request_service(	service_uri, service_uri_len,		\
										10, request, request_len,					\
										&response, &response_len, &response_shm_mode);
				*res = response;
				*res_len = response_len;
				request_state = 2;
			} else {
				ret = linber_request_service_no_blocking(	service_uri, service_uri_len,	\
															10, request, request_len,		\
															&token);
				request_state = 1;
			}
			if(ret < 0){
				printf("request failed\n");
				return;
			}
		} else {
			printf("Only one request at time\n");
		}
	}


	void linber_create_shm(char **req, int req_len){
		key_t shm_key;
		int shm_id;

		if((request_state == 0) && (request_shm_state == -1)){
			*req = create_shm_from_filepath(".", req_len, &shm_key, &shm_id);
			if(*req == NULL){
				printf("Error in shared memory allocation\n");
				return;
			}
			this->request_shm_mode = TRUE;
			this->request = *req;
			this->request_len = req_len;
			this->request_shm_key = shm_key;
			request_shm_state = 0;
		}
	}

	void linber_sendRequest_shm(char **res, int *res_len, bool blocking){
		unsigned long token;
		int ret, response_len;
		char *response;
		boolean response_shm_mode;

		if(request_shm_state == 0){
			if(blocking){
				ret = linber_request_service_shm(	service_uri, service_uri_len,					\
												10, request_shm_key, request_len,					\
												&response, &response_len, &response_shm_mode);
				*res = response;
				*res_len = response_len;
				request_shm_state = 2;
			} else {
				ret = linber_request_service_no_blocking_shm(	service_uri, service_uri_len,		\
																10, request_shm_key, request_len,	\
																&token);
				request_shm_state = 1;
			}
		}
		if(ret < 0){
			printf("request failed\n");
		}
	}

	void linber_get_result(){
		int ret;
		if((request_state == 1) || (request_shm_state == 1)){
			ret = linber_request_service_get_response(	service_uri, service_uri_len,					\
														&response, &response_len, &response_shm_mode,	\
														&token);
			if(ret < 0){
				printf("get result failed\n");
				return;
			}
			if(request_state == 1){
				request_state = 2;
			} else {
				request_shm_state = 2;
			}
		} else {
			printf("no request have been sent\n");
		}
	}

	void linber_end_operation(){
		if((request_state == 1) || (request_shm_state == 0 || request_shm_state == 1)){	// only request
			response = NULL;
			linber_request_service_clean(request, request_shm_mode, response, response_shm_mode);
			request = NULL;
			request_state = 0;
			request_shm_state = -1;
		} else if((request_state == 2) || (request_shm_state == 2)){	// completed
			linber_request_service_clean(request, request_shm_mode, response, response_shm_mode);
			response = NULL;
			request = NULL;
			request_state = 0;
			request_shm_state = -1;
		}
	}
};
