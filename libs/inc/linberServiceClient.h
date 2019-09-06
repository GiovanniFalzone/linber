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

class linberServiceClient {
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
	linberServiceClient(char * service_uri, int service_uri_len);

	~linberServiceClient();
/*-------------------------------------------------------------------------------------------------------
	Blocking or non Blocking method to send the request passed as pointer.
	The result is returned as a pointer in the first argument.
	Relative deadline is expressed as milliseconds.
--------------------------------------------------------------------------------------------------------*/
	void linber_sendRequest(char *req, int req_len, char **res, int *res_len, bool blocking, int rel_deadline);

/*-------------------------------------------------------------------------------------------------------
	Use this method to create a shared memory for the request and then send it using then
	send request for shared memory.
--------------------------------------------------------------------------------------------------------*/
	void linber_create_shm(char **req, int req_len);

/*-------------------------------------------------------------------------------------------------------
	Use this method to send a request created with the linber_create_shm.
	The result is returned as a pointer in the first argument.
	Relative deadline is expressed as milliseconds.
--------------------------------------------------------------------------------------------------------*/
	void linber_sendRequest_shm(char **res, int *res_len, bool blocking, int rel_deadline);

/*-------------------------------------------------------------------------------------------------------
	Use this method to retrieve a response after a non-blocking send request operation
--------------------------------------------------------------------------------------------------------*/
	char* linber_get_response();

/*-------------------------------------------------------------------------------------------------------
	Always call this operation after have comsumed the message.
--------------------------------------------------------------------------------------------------------*/
	void linber_end_operation();
};
