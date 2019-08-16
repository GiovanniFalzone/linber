#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../libs/linber_service_api.h"

#define DEFAULT_SERVICE_URI	"org.service\0"
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
	int ret = linber_request_service(worker.service_uri, worker.uri_len, REL_DEADLINE);
	if(ret == LINBER_ABORT_REQUEST){
		printf("request aborted\n");
	}

	return NULL;
}


int main(int argc,char* argv[]){
	char *service_uri = DEFAULT_SERVICE_URI;
	int uri_len = strlen(service_uri);
	int concurrent_requests = DEFAULT_CONCURRENT_REQUESTS;
	if(argc >= 2){
		service_uri = malloc(strlen(argv[1])+1);
		strcpy(service_uri, argv[1]);
		uri_len = strlen(service_uri);
	}
	if(argc >= 3){
		int n = atoi(argv[2]);
		if(n > 0){
			concurrent_requests = n;
		}
	}

	printf("Running client test with %d concurrent requests on service %s\n", concurrent_requests, service_uri);
	linber_init();

	thread_info worker[concurrent_requests];
	for(int i=0; i<concurrent_requests; i++){
		worker[i].uri_len = uri_len;
		worker[i].service_uri = service_uri;
		worker[i].id = i;
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

