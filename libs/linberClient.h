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
	boolean response_shm_mode = FALSE;
	boolean request_shm_mode = FALSE;
	unsigned long token;
	int request_step = 0;

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
		this->request = req;
		this->request_len = req_len;
		if(request_step == 0){
			if(blocking){
				ret = linber_request_service(	service_uri, service_uri_len,		\
										10, request, request_len,					\
										&response, &response_len, &response_shm_mode);
				*res = response;
				*res_len = response_len;
			} else {
				ret = linber_request_service_no_blocking(	service_uri, service_uri_len,	\
															10, request, request_len,		\
															&token);
			}
			if(ret < 0){
				printf("request failed\n");
				return;
			}
			request_step = 1;
		} else {
			printf("Only one request at time\n");
		}
	}

	void linber_get_result(){
		int ret;
		if(request_step == 1){
			ret = linber_request_service_get_response(	service_uri, service_uri_len,					\
														&response, &response_len, &response_shm_mode,	\
														&token);
			if(ret < 0){
				printf("get result failed\n");
				return;
			}
			request_step = 2;
		} else {
			printf("no request have been sent\n");
		}
	}

	void linber_end_operation(){
		if(request_step < 2){	// only request
			response = NULL;
			linber_request_service_clean(request, request_shm_mode, response, response_shm_mode);
			request_step = 0;
		} else if(request_step == 2){	// completed
			linber_request_service_clean(request, request_shm_mode, response, response_shm_mode);
			response = NULL;
			request = NULL;
			request_step = 0;

		}
	}
};
