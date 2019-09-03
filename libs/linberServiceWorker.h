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

protected:
		boolean request_shm_mode;
		char *request;
		int request_len;
		char *response;
		int response_len;

public:
	linberServiceWorker(char * service_uri, unsigned long service_token){
		this->service_token = service_token;
		this->uri_len = strlen(service_uri) + 1;
		this->service_uri = (char*)malloc(this->uri_len+1);
		this->service_uri[this->uri_len-1] = '\0';
		this->service_uri[this->uri_len] = '\0';
		strcpy(this->service_uri, service_uri);
		this->job_num = 0;

		this->request = NULL;
		this->response = NULL;

		linber_init();
		this->worker_alive = true;

		thread_worker = std::thread(&linberServiceWorker::worker_job, this);
	}

	virtual ~linberServiceWorker(){
		free(service_uri);
		service_uri = NULL;
	}

	void join_worker(){
		if(thread_worker.joinable()){
			thread_worker.join();
		}
	}

	void terminate_worker(){
		worker_alive = false;
	}

	void worker_job(){
		int ret;
		if(linber_register_service_worker(service_uri, uri_len, service_token, &worker_id, &file_str) < 0){
			worker_alive = false;
		} else {
			printf("Started Worker id:%d, service:%s token:%lu\n", worker_id, service_uri, service_token);
		}
		while(worker_alive){
			request_shm_mode = false;
			ret = linber_start_job_service(	service_uri,		\
											uri_len,			\
											service_token,		\
											worker_id,			\
											&request,			\
											&request_len,		\
											&request_shm_mode	\
											);

			if(ret < 0){
				break;
			}
			if(ret != LINBER_SERVICE_SKIP_JOB){
				#ifdef DEBUG
					printf("Worker id:%d job#:%d, serving request\n", worker_id, job_num++);
				#endif

				execute_job();
			}

// always execute end job, also if it is a spurious job, needed to free the request memory
			ret = linber_end_job_service(	service_uri,			\
											uri_len,				\
											service_token,			\
											worker_id,				\
											request,				\
											request_shm_mode,		\
											response,				\
											response_len			\
										);
		}
		printf("Worker %d died\n", worker_id);
	}

	virtual void execute_job(){
		printf("old execute\n");
	}
};
