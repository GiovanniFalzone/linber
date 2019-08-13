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
		linber_start_job_service(worker.service_uri, worker.uri_len);
		printf("thread id:%d job#:%d, serving request for service:%s\n", worker.id, job_num++, worker.service_uri);
		sleep(THREAD_JOB_EXEC_TIME);
		linber_end_job_service(worker.service_uri, worker.uri_len);
	}
}

int main() {
	printf("Running Server test\n");
	linber_init();
	linber_register_service(SERVICE_NAME, sizeof(SERVICE_NAME), SERVICE_EXECTIME);

	thread_info worker;
	worker.id = 0;
	worker.uri_len = sizeof(SERVICE_NAME);
	worker.service_uri = malloc(worker.uri_len);
	strcpy(worker.service_uri, SERVICE_NAME);

	printf("starting thread pool\n");
	int terr = pthread_create(&worker.tid, NULL, thread_job, (void*)&worker);

	if (terr != 0){
		printf("Thread creation error: %d\n", terr);
		exit(1);
	}
	pthread_join(worker.tid, NULL);

	linber_exit();
}


