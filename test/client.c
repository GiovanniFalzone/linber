#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../libs/linber_service_api.h"

#define SERVICE_NAME	"org.server.func.1\0"
#define REL_DEADLINE		10

#define CONCURRENT_REQUEST	10

typedef struct{
	pthread_t tid;
	int id;
	char *service_uri;
	int uri_len;
} thread_info;


void *thread_job(void *args){
	thread_info worker = *(thread_info*)args;
	printf("sending request id:%d, service:%s\n", worker.id, worker.service_uri);
	linber_request_service(SERVICE_NAME, sizeof(SERVICE_NAME), REL_DEADLINE);
}


int main(){
	printf("Running client test\n");
	linber_init();

	thread_info worker[CONCURRENT_REQUEST];
	for(int i=0; i<CONCURRENT_REQUEST; i++){
		worker[i].uri_len = sizeof(SERVICE_NAME);
		worker[i].service_uri = malloc(worker[i].uri_len);
		worker[i].id = i;
		strcpy(worker[i].service_uri, SERVICE_NAME);
		int terr = pthread_create(&worker[i].tid, NULL, thread_job, (void*)&worker[i]);
		if (terr != 0){
			printf("Thread creation error: %d\n", terr);
			exit(1);
		}
	}

	for(int i=0; i<CONCURRENT_REQUEST; i++){
		pthread_join(worker[i].tid, NULL);
	}

	linber_exit();
}

