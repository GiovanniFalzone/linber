/*-------------------------------------------------------------------------------------------------------
	Extend this class to realize a linber worker, the class need the service uri and  service token.
	Implement the virtual method execute job that will be executed every request.
	The request is available as protected member of the class, don't free or manipulate it.
	this class is valid for a Worker that use dynamic memory for the buffer, put your buffer address
	in the response protected member and don't free it, it'll be free automatically.

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

class linberServiceWorker {
	char *service_uri;
	char *file_str;
	int uri_len;
	unsigned long service_token;
	unsigned int exec_time;
	unsigned int worker_id;
	unsigned int job_num;
	std::thread thread_worker;
	bool worker_alive;

	void worker_job();
	
protected:
		boolean request_shm_mode;
		char *request;
		int request_len;
		char *response;
		int response_len;

public:
	linberServiceWorker(char * service_uri, unsigned long service_token);
	virtual ~linberServiceWorker();
	void join_worker();
	void terminate_worker();
	virtual void execute_job();
};
