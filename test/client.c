#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../libs/linber_service_api.h"

#define SERVICE_NAME	"org.server.func.1\0"
#define REL_DEADLINE		10
#define DEFAULT_CONCURRENT_REQUESTS	10

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


int main(int argc,char* argv[]){
	int concurrent_requests = DEFAULT_CONCURRENT_REQUESTS;
	if(argc == 2){
		int n = atoi(argv[1]);
		if(n > 0){
			concurrent_requests = n;
		}
	}

	printf("Running client test with %d concurrent requests on service %s\n", concurrent_requests, SERVICE_NAME);
	linber_init();

	thread_info worker[concurrent_requests];
	for(int i=0; i<concurrent_requests; i++){
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

	for(int i=0; i<concurrent_requests; i++){
		pthread_join(worker[i].tid, NULL);
	}

	linber_exit();
}

