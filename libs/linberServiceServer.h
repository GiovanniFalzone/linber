#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "linber_service_api.h"
#include "linberServiceWorker.h"

class linberServiceServer {
	char *service_uri;
	int uri_len;
	unsigned long service_token;
	unsigned int exec_time;
	unsigned int workers_num;
	char *service_params;
	int service_params_len;
	char *service_result;
	int service_result_len;
	pthread_t *worker_tid;

	public:
	linberServiceServer(char * service_uri, int service_params_len, int service_result_len, int workers_num){
		this->uri_len = strlen(service_uri) + 1;
		this->service_uri = (char*)malloc(this->uri_len+1);
		this->service_uri[this->uri_len-1] = '\0';
		this->service_uri[this->uri_len] = '\0';
		strcpy(this->service_uri, service_uri);
		this->service_token = service_token;
		this->exec_time = compute_exec_time();
		this->workers_num = workers_num;
		this->service_params_len = service_params_len;
		this->service_result_len = service_result_len;
		service_params = (char*)malloc(service_params_len);
		service_result = (char*)malloc(service_result_len);
		linber_init();
		linber_register_service(this->service_uri, this->uri_len, this->exec_time, workers_num, &service_token);

		worker_tid = (pthread_t*)malloc(sizeof(pthread_t)*this->workers_num);
		for(int i=0; i<workers_num; i++){
			int terr = pthread_create(&worker_tid[i], NULL, worker_job, (void*)&worker[i]);
			if (terr != 0){
				printf("Thread creation error: %d\n", terr);
				exit(1);
			}
		}

		for(int i=0; i<workers_num; i++){
			pthread_join(worker[i].tid, NULL);
		}

	}

	unsigned int compute_exec_time(){
		return 100;
	}

	~linberServiceServer(){
		linber_destroy_service(this->service_uri, this->uri_len, this->service_token);
		linber_exit();
		free(this->service_uri);
		free(this->service_params);
		free(this->service_result);
	}

	virtual void execute_job(){}
};

