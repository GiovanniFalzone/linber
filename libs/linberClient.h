/*-------------------------------------------------------------------------------------------------------
	Extend this class to implement a Service Client using the linber framework, it needs just the service uri
	and service len.
	For a practical example see the Calculator example in the test directory
--------------------------------------------------------------------------------------------------------*/

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
	unsigned long token, abs_deadline;
	int request_state = 0, request_shm_state = -1;
	int rel_deadline;

	public:
	linberClient(char * service_uri, int service_uri_len){
		this->service_uri = service_uri;
		this->service_uri_len = service_uri_len;
		linber_init();
	}

	~linberClient(){
		linber_exit();
	}
/*-------------------------------------------------------------------------------------------------------
	Blocking or non Blocking method to send the request passed as pointer.
	The result is returned as a pointer in the first argument.
	Relative deadline is expressed as milliseconds.
--------------------------------------------------------------------------------------------------------*/
	void linber_sendRequest(char *req, int req_len, char **res, int *res_len, bool blocking, int rel_deadline){
		int ret;
		if((request_state == 0) && (request_shm_state == -1)){
			this->request_shm_mode = FALSE;
			this->request = req;
			this->request_len = req_len;
			this->rel_deadline = rel_deadline;
			if(blocking){
				ret = linber_request_service(	service_uri, 			\
												service_uri_len,		\
												rel_deadline,			\
												request,				\
												request_len,			\
												&response,				\
												&response_len,			\
												&response_shm_mode		\
											);
				*res = response;
				*res_len = response_len;
				request_state = 2;
			} else {
				ret = linber_request_service_no_blocking(	service_uri,		\
															service_uri_len,	\
															rel_deadline,		\
															request,			\
															request_len,		\
															&token,				\
															&abs_deadline		\
														);
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

/*-------------------------------------------------------------------------------------------------------
	Use this method to create a shared memory for the request and then send it using then
	send request for shared memory.
--------------------------------------------------------------------------------------------------------*/
	void linber_create_shm(char **req, int req_len){
		key_t shm_key;
		int shm_id;
		static int low_id = 0;
		if((request_state == 0) && (request_shm_state == -1)){
			*req = create_shm_from_filepath(".", low_id++, req_len, &shm_key, &shm_id);
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

/*-------------------------------------------------------------------------------------------------------
	Use this method to send a request created with the linber_create_shm.
	The result is returned as a pointer in the first argument.
	Relative deadline is expressed as milliseconds.
--------------------------------------------------------------------------------------------------------*/
	void linber_sendRequest_shm(char **res, int *res_len, bool blocking, int rel_deadline){
		unsigned long token;
		int ret, response_len;
		char *response;
		boolean response_shm_mode;

		if(request_shm_state == 0){
			this->rel_deadline = rel_deadline;
			if(blocking){
				ret = linber_request_service_shm(	service_uri,						\
													service_uri_len,					\
													rel_deadline,						\
													request_shm_key,					\
													request_len,						\
													&response,							\
													&response_len,						\
													&response_shm_mode					\
												);
				*res = response;
				*res_len = response_len;
				request_shm_state = 2;
			} else {
				ret = linber_request_service_no_blocking_shm(	service_uri,			\
																service_uri_len,		\
																rel_deadline,			\
																request_shm_key,		\
																request_len,			\
																&token,					\
																&abs_deadline			\
															);
				request_shm_state = 1;
			}
		}
		if(ret < 0){
			printf("request failed\n");
		}
	}

/*-------------------------------------------------------------------------------------------------------
	Use this method to retrieve a response after a non-blocking send request operation
--------------------------------------------------------------------------------------------------------*/
	char* linber_get_response(){
		int ret;
		if((request_state == 1) || (request_shm_state == 1)){
			ret = linber_request_service_get_response(	service_uri,		\
														service_uri_len,	\
														&response,			\
														&response_len,		\
														&response_shm_mode,	\
														&token,				\
														abs_deadline		\
													);
			if(ret < 0){
				printf("get result failed\n");
				return NULL;
			}
			if(request_state == 1){
				request_state = 2;
			} else {
				request_shm_state = 2;
			}
			return request;
		} else {
			printf("no request have been sent\n");
			return NULL;
		}
	}

/*-------------------------------------------------------------------------------------------------------
	Always call this operation after have comsumed the message.
--------------------------------------------------------------------------------------------------------*/
	void linber_end_operation(){
		if((request_state == 1) || (request_shm_state == 0 || request_shm_state == 1)){	// only request
			response = NULL;
			linber_request_service_clean(response, response_shm_mode);
			request_state = 0;
			request_shm_state = -1;
		} else if((request_state == 2) || (request_shm_state == 2)){	// completed
			linber_request_service_clean(response, response_shm_mode);
			response = NULL;
			request_state = 0;
			request_shm_state = -1;
		}
	}
};
