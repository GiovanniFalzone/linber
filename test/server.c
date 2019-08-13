#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "../libs/linber_service_api.h"

#define SERVICE_NAME	"org.server.func.1\0"
#define SERVICE_EXECTIME	5

#define THREAD_JOB_EXEC_TIME	1

#define MAX_WORKERS	4

typedef struct{
	pthread_t tid;
	int id;
	char *service_uri;
	int uri_len;
} thread_info;


void *thread_job(void *args){
	thread_info worker = *(thread_info*)args;
	printf("started_thread id:%d, service:%s\n", worker.id, worker.service_uri);
	int job_num = 1;
	while(1){
		linber_start_job_service(worker.service_uri, worker.uri_len,worker.id);
		printf("thread id:%d job#:%d, serving request for service:%s\n", worker.id, job_num++, worker.service_uri);
		sleep(THREAD_JOB_EXEC_TIME);
		linber_end_job_service(worker.service_uri, worker.uri_len,worker.id);
	}
}

int main() {
	printf("Running Server test\n");
	linber_init();
	linber_register_service(SERVICE_NAME, sizeof(SERVICE_NAME), SERVICE_EXECTIME, MAX_WORKERS);

	printf("starting thread pool\n");
	thread_info worker[MAX_WORKERS];
	for(int i=0; i<MAX_WORKERS; i++){
		worker[i].uri_len = sizeof(SERVICE_NAME);
		worker[i].service_uri = malloc(worker[i].uri_len);
		worker[i].id = 0;
		strcpy(worker[i].service_uri, SERVICE_NAME);
		int terr = pthread_create(&worker[i].tid, NULL, thread_job, (void*)&worker);
		if (terr != 0){
			printf("Thread creation error: %d\n", terr);
			exit(1);
		}
	}

	for(int i=0; i<MAX_WORKERS; i++){
		pthread_join(worker[i].tid, NULL);
	}

	linber_exit();
}


