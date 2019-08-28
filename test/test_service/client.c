#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../../libs/linber_service_api.h"
#include <sys/time.h>
#include <signal.h>


#define DEFAULT_SERVICE_URI	"org.service\0"
#define REL_DEADLINE		10
#define DEFAULT_CONCURRENT_REQUESTS	10
#define DEFAULT_MAX_INTER_PERIOD	100

typedef struct{
	pthread_t tid;
	int id;
	char *service_uri;
	int uri_len;
	int blocking;
} thread_info;

int abort_request = 0;

void sig_handler(int signo){
  if (signo == SIGINT){
    printf("received SIGINT\n");
	abort_request = 1;
  }
}

void *thread_job(void *args){
	thread_info worker = *(thread_info*)args;
	struct timeval start, end;
	unsigned long passed_millis = 0;
	int ret = 0;
	unsigned long token;
	int request_len = strlen("ciao\0") + 1;
	char *request = malloc(request_len);
	char *response;
	int response_len;
	boolean response_shm_mode;

	strcpy(request, "ciao\0");
	request[request_len-1] = '\0';

	if(abort_request == 0){
		gettimeofday(&start, NULL);
		if(worker.blocking == 1){
			ret = linber_request_service(	worker.service_uri, worker.uri_len,	\
											REL_DEADLINE, request, request_len,	\
											&response, &response_len, &response_shm_mode);

			printf("sending Blocking request id:%d, service:%s\n", worker.id, worker.service_uri);
		} else {
			ret = linber_request_service_no_blocking(	worker.service_uri, worker.uri_len,	\
														REL_DEADLINE, request, request_len,	\
														&token);
			if(ret >= 0){
				printf("sending NON Blocking request id:%d, service:%s\n", worker.id, worker.service_uri);
				sleep(1);
				printf("Asking for response request id:%d, service:%s\n", worker.id, worker.service_uri);
				ret = linber_request_service_get_response(	worker.service_uri, worker.uri_len,	\
															&response, &response_len, &response_shm_mode,	\
															&token);
			}
		}
		if(ret < 0){
			printf("request aborted\n");
		} else {
			gettimeofday(&end, NULL);
			passed_millis = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
			printf("Request %d Response: %d %s served in %lu ms\n", worker.id, response_len, response, passed_millis);
		}
		linber_request_service_clean(request, FALSE, response, response_shm_mode);
	}
	return NULL;
}


int main(int argc,char* argv[]){
	char *service_uri = DEFAULT_SERVICE_URI;
	int uri_len = strlen(service_uri);
	int concurrent_requests = DEFAULT_CONCURRENT_REQUESTS;
	int max_inter_request = DEFAULT_MAX_INTER_PERIOD;
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
	if(argc >= 4){
		int n = atoi(argv[3]);
		if(n > 0){
			max_inter_request = n;
		}
	}
	printf("Running client test with %d concurrent requests on service %s\n", concurrent_requests, service_uri);
	linber_init();

	thread_info worker[concurrent_requests];
	for(int i=0; i<concurrent_requests; i++){
		worker[i].uri_len = uri_len;
		worker[i].service_uri = service_uri;
		worker[i].id = i;
		if(i < concurrent_requests*0.5){
			worker[i].blocking = 0;
		} else {
			worker[i].blocking = 1;
		}
		usleep(1000*(rand()%max_inter_request + 1));
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

