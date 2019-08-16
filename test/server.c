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
unsigned long service_id;
int abort_and_Exit = 0;

typedef struct{
	pthread_t tid;
	int id;
	unsigned long service_id;
	int exec_time;
} thread_info;


void sig_handler(int signo){
  if (signo == SIGINT){
    printf("received SIGINT\n");
	abort_and_Exit = 1;
	linber_destroy_service(service_uri, uri_len, service_id);
  }
}

void *thread_job(void *args){
	int ret;
	thread_info worker = *(thread_info*)args;
	printf("started_thread id:%d, service:%s\n", worker.id, service_uri);
	int job_num = 1;
	while(1){
		ret = linber_start_job_service(service_uri, uri_len, worker.service_id, worker.id);
		if((ret <0) || abort_and_Exit){ // if spuroius
			break;
		}
		printf("thread id:%d job#:%d, serving request for service:%s\n", worker.id, job_num++, service_uri);
		struct timeval start, end;
		gettimeofday(&start, NULL);
		unsigned long passed_millis = 0;
		do{
			gettimeofday(&end, NULL);
			passed_millis = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
		} while(passed_millis < worker.exec_time);
		ret = linber_end_job_service(service_uri, uri_len, worker.service_id, worker.id);
		if((ret <0) || abort_and_Exit){ // if spuroius
			break;
		}
	}
	printf("Thread %d died\n", worker.id);
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
	linber_register_service(service_uri, uri_len, job_exec_time, max_concurrent_workers, &service_id);
	printf("%lu\n", service_id);
	printf("starting thread pool\n");
	int workers_num = max_concurrent_workers * DAFAULT_SPARE_WORKER;
	thread_info worker[workers_num];
	for(int i=0; i<workers_num; i++){
		unsigned int worker_id;
		linber_register_service_worker(service_uri, uri_len, &worker_id);
		worker[i].id = worker_id;
		worker[i].service_id = service_id;
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


