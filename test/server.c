#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "../libs/linber_service_api.h"

#define SERVICE_NAME	"org.server.func.1\0"
#define SERVICE_EXECTIME	5

#define DEFAULT_JOB_EXEC_TIME	100	//ms

#define DEFAULT_MAX_WORKERS		8

typedef struct{
	pthread_t tid;
	int id;
	char *service_uri;
	int uri_len;
	int exec_time;
} thread_info;


void *thread_job(void *args){
	thread_info worker = *(thread_info*)args;
	printf("started_thread id:%d, service:%s\n", worker.id, worker.service_uri);
	int job_num = 1;
	while(1){
		linber_start_job_service(worker.service_uri, worker.uri_len,worker.id);
		printf("thread id:%d job#:%d, serving request for service:%s\n", worker.id, job_num++, worker.service_uri);
		struct timeval start, end;
		gettimeofday(&start, NULL);
		unsigned long passed_millis = 0;
		do{
			gettimeofday(&end, NULL);
			passed_millis = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
		} while(passed_millis < worker.exec_time);
		linber_end_job_service(worker.service_uri, worker.uri_len,worker.id);
	}
}

int main(int argc,char* argv[]){
	int max_workers = DEFAULT_MAX_WORKERS;
	int job_exec_time = DEFAULT_JOB_EXEC_TIME;
	if(argc >= 2){
		int n = atoi(argv[1]);
		if(n > 0){
			max_workers = n;
		}
	}

	if(argc >= 3){
		int n = atoi(argv[2]);
		if(n > 0){
			job_exec_time = n;
		}
	}

	printf("Running Service Server %s with %d workers, job exec time:%d\n", SERVICE_NAME, max_workers, job_exec_time);

	linber_init();
	linber_register_service(SERVICE_NAME, sizeof(SERVICE_NAME), job_exec_time, max_workers);

	printf("starting thread pool\n");
	thread_info worker[max_workers];
	for(int i=0; i<max_workers; i++){
		worker[i].uri_len = sizeof(SERVICE_NAME);
		worker[i].service_uri = malloc(worker[i].uri_len);
		worker[i].id = i;
		worker[i].exec_time = job_exec_time;
		strcpy(worker[i].service_uri, SERVICE_NAME);
		int terr = pthread_create(&worker[i].tid, NULL, thread_job, (void*)&worker[i]);
		if (terr != 0){
			printf("Thread creation error: %d\n", terr);
			exit(1);
		}
	}

	for(int i=0; i<max_workers; i++){
		pthread_join(worker[i].tid, NULL);
	}

	linber_exit();
}


