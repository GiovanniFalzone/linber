#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#include "../libs/linber_service_api.h"

#define DEFAULT_SERVICE_URI	"org.service\0"

#define DEFAULT_JOB_EXEC_TIME	100	//ms

#define DEFAULT_MAX_CONCURRENT_WORKERS		4
#define DAFAULT_SPARE_WORKER				2

char *service_uri = DEFAULT_SERVICE_URI;
int uri_len;
unsigned long service_token;
int abort_and_Exit = 0;

typedef struct{
	pthread_t tid;
	unsigned long service_token;
	int exec_time;
} thread_info;


void sig_handler(int signo){
  if (signo == SIGINT){
    printf("received SIGINT\n");
	abort_and_Exit = 1;
	linber_destroy_service(service_uri, uri_len, service_token);
  }
}

void *thread_job(void *args){
	int ret, job_num = 1, service_params_len, service_result_len;
	unsigned int worker_id, slot_id;
	char *service_params, *service_result;
	thread_info worker = *(thread_info*)args;
//-----------------------------
	service_params = malloc(32);
	service_result = "bye\0";
	service_result_len = strlen(service_result) + 1;
//-----------------------------
	if(linber_register_service_worker(service_uri, uri_len,worker.service_token, &worker_id) == 0){
		printf("started_thread id:%d, service:%s\n", worker_id, service_uri);
		while(1){
			ret = linber_start_job_service(service_uri, uri_len, worker.service_token, worker_id, &slot_id, service_params, &service_params_len);
			if(ret < 0){
				break;
			}
			if(ret != LINBER_SKIP_JOB){
				printf("thread id:%d job#:%d, serving request (%s)\n", worker_id, job_num++, service_params);
				struct timeval start, end;
				gettimeofday(&start, NULL);
				unsigned long passed_millis = 0;
				do{
					gettimeofday(&end, NULL);
					passed_millis = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
				} while(passed_millis < worker.exec_time);
			}
			ret = linber_end_job_service(service_uri, uri_len, worker.service_token, worker_id, slot_id, service_result, service_result_len);
		}
	}
	printf("Thread %d died\n", worker_id);
	return NULL;
}

int main(int argc,char* argv[]){
	int max_concurrent_workers = DEFAULT_MAX_CONCURRENT_WORKERS;
	int job_exec_time = DEFAULT_JOB_EXEC_TIME;
	if (signal(SIGINT, sig_handler) == SIG_ERR)
		printf("can't catch SIGINT\n");

	if(argc >= 2){
		service_uri = malloc(strlen(argv[1])+1);
		strcpy(service_uri, argv[1]);
	}

	if(argc >= 3){
		int n = atoi(argv[2]);
		if(n > 0){
			max_concurrent_workers = n;
		}
	}

	if(argc >= 4){
		int n = atoi(argv[3]);
		if(n > 0){
			job_exec_time = n;
		}
	}

	uri_len = strlen(service_uri);

	printf("Running Service Server %s with %d workers, job exec time:%d\n", service_uri, max_concurrent_workers, job_exec_time);

	linber_init();
	linber_register_service(service_uri, uri_len, job_exec_time, max_concurrent_workers, &service_token);
	printf("starting thread pool\n");
	int workers_num = max_concurrent_workers + DAFAULT_SPARE_WORKER;
	thread_info worker[workers_num];
	for(int i=0; i<workers_num; i++){
		worker[i].service_token = service_token;
		worker[i].exec_time = job_exec_time;
		int terr = pthread_create(&worker[i].tid, NULL, thread_job, (void*)&worker[i]);
		if (terr != 0){
			printf("Thread creation error: %d\n", terr);
			exit(1);
		}
	}

	for(int i=0; i<workers_num; i++){
		pthread_join(worker[i].tid, NULL);
	}

	linber_exit();
}


