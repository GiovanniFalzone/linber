#include "linberServiceWorker.h"

linberServiceWorker::linberServiceWorker(char * service_uri, unsigned long service_token){
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

linberServiceWorker::~linberServiceWorker(){
	linber_destroy_worker(file_str);
	free(service_uri);
	service_uri = NULL;
}

void linberServiceWorker::join_worker(){
	if(thread_worker.joinable()){
		thread_worker.join();
	}
}

void linberServiceWorker::terminate_worker(){
	worker_alive = false;
}

void linberServiceWorker::worker_job(){
	int ret;
	if(linber_register_service_worker(service_uri, uri_len, service_token, &worker_id, &file_str) < 0){
		worker_alive = false;
	} else {
		printf("Started Worker id:%d, service:%s token:%lu\n", worker_id, service_uri, service_token);
	}
	while(worker_alive){
		request_shm_mode = false;
		response = NULL;
		request = NULL;
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

		ret = linber_end_job_service(	service_uri,			\
										uri_len,				\
										service_token,			\
										worker_id,				\
										request,				\
										request_shm_mode,		\
										response,				\
										response_len			\
									);
		free(response);
	}
	printf("Worker %d died\n", worker_id);
}

void linberServiceWorker::execute_job(){
	printf("old execute\n");
}
