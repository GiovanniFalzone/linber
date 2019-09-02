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
#define DEFAULT_CONCURRENT_REQUESTS	16
#define DEFAULT_MAX_INTER_PERIOD	0
#define DEFAULT_REL_DEADLINE		0	// 0 for best effor

int abort_request = 0;
char *service_uri = DEFAULT_SERVICE_URI;
int uri_len = sizeof(DEFAULT_SERVICE_URI);
int concurrent_requests = DEFAULT_CONCURRENT_REQUESTS;
int max_inter_request = DEFAULT_MAX_INTER_PERIOD;
int max_rel_Deadline_ms = DEFAULT_REL_DEADLINE;

void sig_handler(int signo){
  if (signo == SIGINT){
    printf("received SIGINT\n");
	abort_request = 1;
  }
}

typedef struct{
	pthread_t tid;
	int id;
	int rel_deadline_ms;
} thread_info;

void *thread_job(void *args){
	thread_info worker = *(thread_info*)args;
	struct timeval start, end;
	unsigned long passed_millis = 0;
	int ret = 0;
	int request_len = strlen("ciao\0") + 1;
	char *request = malloc(request_len);	// request will be free by linber_request_service_clean
	char *response;
	int response_len;
	boolean response_shm_mode;

	strcpy(request, "ciao\0");
	request[request_len-1] = '\0';

	if(abort_request == 0){
		gettimeofday(&start, NULL);
		printf("sending Blocking request id:%d, service:%s\n", worker.id, service_uri);

		ret = linber_request_service(	service_uri, uri_len,	\
										worker.rel_deadline_ms, request, request_len,	\
										&response, &response_len, &response_shm_mode);
		if(ret < 0){
			printf("request aborted\n");
		} else {
			gettimeofday(&end, NULL);
			passed_millis = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000;
			printf("Request %d Response: %d %s served in %lu ms\n", worker.id, response_len, response, passed_millis);
		}
		linber_request_service_clean(request, FALSE, response, response_shm_mode);
	}
	return NULL;
}


int main(int argc,char* argv[]){
	int n;
	if(argc >= 2){		// SERVICE URI
		service_uri = malloc(strlen(argv[1])+1);
		strcpy(service_uri, argv[1]);
		uri_len = strlen(service_uri);
	}
	if(argc >= 3){		// CONCURRENT REQUESTS
		n = atoi(argv[2]);
		if(n > 0){
			concurrent_requests = n;
		}
	}
	if(argc >= 4){		// INTER REQUEST TIME
		n = atoi(argv[3]);
		if(n >= 0){
			max_inter_request = n;
		}
	}
	if(argc >= 5){		// MAX REL DEADLINE
		n = atoi(argv[4]);
		if(n >= 0){
			max_rel_Deadline_ms = n;
		}
	}

	printf("Running client test with %d concurrent requests on service %s\n", concurrent_requests, service_uri);
	linber_init();
	thread_info worker[concurrent_requests];
	for(int i=0; i<concurrent_requests; i++){
		worker[i].id = i;
		if(max_inter_request > 0){
			usleep(1000*(rand()%max_inter_request));
		}
		if(max_rel_Deadline_ms > 0){
			worker[i].rel_deadline_ms = rand()%max_rel_Deadline_ms;
		} else {
			worker[i].rel_deadline_ms = 0;
		}
		int terr = pthread_create(&worker[i].tid, NULL, thread_job, (void*)&worker[i]);
		if (terr != 0){
			printf("Thread creation error: %d\n", terr);
			exit(1);
		}
	}

	for(int i=0; i<concurrent_requests; i++){
		pthread_join(worker[i].tid, NULL);
	}
	if(argc >= 2){
		free(service_uri);
	}
	linber_exit();
}

