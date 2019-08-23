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
	int service_id;
	int uri_len;
	unsigned long service_token;
	unsigned int exec_time;
	unsigned int worker_id;
	unsigned int slot_id;
	unsigned int job_num;
	std::thread thread_worker;
	bool worker_alive;

protected:
		char *request;
		int request_len;
		char *response;
		int response_len;

public:
	linberServiceWorker(char * service_uri, int service_id, unsigned long service_token){
		this->service_id = service_id;
		this->service_token = service_token;
		this->uri_len = strlen(service_uri) + 1;
		this->service_uri = (char*)malloc(this->uri_len+1);
		this->service_uri[this->uri_len-1] = '\0';
		this->service_uri[this->uri_len] = '\0';
		strcpy(this->service_uri, service_uri);
		this->job_num = 0;

		this->request = NULL;
		this->response = NULL;

		if(linber_register_service_worker(service_uri, uri_len, service_token, &worker_id) == 0){
			printf("Started Worker id:%d, service:%s token:%lu\n", worker_id, service_uri, service_token);
		}
		this->worker_alive = true;
		thread_worker = std::thread(&linberServiceWorker::worker_job, this);
	}

	void join_worker(){
		if(thread_worker.joinable()){
			thread_worker.join();
		}
	}

	void terminate_worker(){
		worker_alive = false;
	}

	virtual ~linberServiceWorker(){
		free(service_uri);
	}

	void worker_job(){
		int ret;
		linber_init();
		while(worker_alive){
			ret = linber_start_job_service(service_uri, uri_len, service_id, service_token, worker_id, &slot_id, &request, &request_len);

			if(ret < 0){
				printf("Job aborted ret %i\n", ret);
				return;
			}
			if(ret != LINBER_SERVICE_SKIP_JOB){
				printf("Worker id:%d job#:%d, serving request\n", worker_id, job_num++);
				execute_job();
			}

			if(request != NULL){
				free(request);
			}
			ret = linber_end_job_service(service_uri, uri_len, service_id, service_token, worker_id, slot_id, response, response_len);
		}
	}

	virtual void execute_job(){
		printf("old execute\n");
	}
};
