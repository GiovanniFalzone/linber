#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <pthread.h>
#include "linber_service_api.h"

class linberServiceWorker {
	char *service_uri;
	int uri_len;
	unsigned long service_token;
	unsigned int exec_time;
	unsigned int worker_id;
	unsigned int slot_id;
	unsigned int job_num;
	char *service_params;
	int service_params_len;
	char *service_result;
	int service_result_len;
	std::thread thread_worker;
	bool worker_alive;

	public:
	linberServiceWorker(char * service_uri, unsigned long service_token, int service_params_len, int service_result_len){
		this->uri_len = strlen(service_uri) + 1;
		this->service_uri = (char*)malloc(this->uri_len+1);
		this->service_uri[this->uri_len-1] = '\0';
		this->service_uri[this->uri_len] = '\0';
		strcpy(this->service_uri, service_uri);
		this->service_token = service_token;
		this->job_num = 0;
		this->service_params_len = service_params_len;
		this->service_result_len = service_result_len;
		service_params = (char*)malloc(service_params_len);
		service_result = (char*)malloc(service_result_len);

		if(linber_register_service_worker(service_uri, uri_len, service_token, &worker_id) == 0){
			printf("Started Worker id:%d, service:%s token:%lu\n", worker_id, service_uri, service_token);
		}
		this->worker_alive = true;
		thread_worker = std::thread(&linberServiceWorker::worker_job, this);
	}

	virtual ~linberServiceWorker(){
		worker_alive = false;
		if(thread_worker.joinable()){
			thread_worker.join();
		}
		free(service_uri);
		free(service_params);
		free(service_result);
	}

	void worker_job(){
		int ret;
		linber_init();
		while(worker_alive){
			ret = linber_start_job_service(service_uri, uri_len, service_token, worker_id, &slot_id, service_params, &service_params_len);
			if(ret < 0){
				printf("Job aborted ret %i\n", ret);
				return;
			}
			if(ret != LINBER_SERVICE_SKIP_JOB){
				printf("Worker id:%d job#:%d, serving request (%s)\n", worker_id, job_num++, service_params);
				execute_job();
			}
			ret = linber_end_job_service(service_uri, uri_len, service_token, worker_id, slot_id, service_result, service_result_len);
		}
	}

	virtual void execute_job(){}
};