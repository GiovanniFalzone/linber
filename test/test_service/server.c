#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#include "../../libs/linber_service_api.h"

#define DEFAULT_SERVICE_URI					"org.service\0"
#define DEFAULT_JOB_EXEC_TIME				5	//ms
#define DEFAULT_MAX_CONCURRENT_WORKERS		4
#define DEFAULT_SERVICE_PERIOD				100
#define DEFAULT_REQUEST_PER_PERIOD			1

char *service_uri;
int uri_len;
unsigned long service_token;
int Max_Working = DEFAULT_MAX_CONCURRENT_WORKERS;
int Abort_and_Exit = 0;
int job_exec_time = DEFAULT_JOB_EXEC_TIME;
int Service_period = DEFAULT_SERVICE_PERIOD;
int Service_RPP = DEFAULT_REQUEST_PER_PERIOD;


typedef struct{
	pthread_t tid;
	unsigned long service_token;
	unsigned int exec_time;
} thread_info;

void sig_handler(int signo){
  if (signo == SIGINT){
    printf("received SIGINT\n");
	Abort_and_Exit = 1;
	linber_destroy_service(service_uri, uri_len, service_token);
  }
}

void *thread_job(void *args){
	int ret, job_num = 1, request_len, response_len;
	unsigned int worker_id;
	char *request, *response, *file_str;
	boolean request_shm_mode;
	thread_info worker = *(thread_info*)args;
	struct timeval start, end;
	unsigned long passed_millis = 0;

	if(linber_register_service_worker(service_uri, uri_len,worker.service_token, &worker_id, &file_str) == 0){
		printf("started_thread id:%d, service:%s\n", worker_id, service_uri);
		while(Abort_and_Exit == 0){
			request_shm_mode = FALSE;
			response = NULL;
			request = NULL;
			ret = linber_start_job_service(	service_uri, uri_len,		\
											worker.service_token,		\
											worker_id,					\
											&request,					\
											&request_len,				\
											&request_shm_mode			\
											);
			if(ret < 0){
				break;
			}

			if(ret != LINBER_SERVICE_SKIP_JOB){
				response_len = request_len;
				response = malloc(response_len);

				gettimeofday(&start, NULL);
				do{
					gettimeofday(&end, NULL);
					passed_millis = (end.tv_sec - start.tv_sec)*1000;
					passed_millis = (end.tv_usec - start.tv_usec)*0.001;
				} while(passed_millis < worker.exec_time);
				#ifdef DEBUG_MESSAGE
					printf("thread id:%d job#:%d, serving request, %s %d\n", worker_id, job_num++, request, request_len);
					memcpy(response, request, response_len);
					printf("response: %s\n", response);
				#endif

				ret = linber_end_job_service(	service_uri, uri_len,		\
												worker.service_token,		\
												worker_id,					\
												request,					\
												request_shm_mode,			\
												response,					\
												response_len				\
											);

				// free memory response
				free(response);
			}

		}
	}
	linber_destroy_worker(file_str);
	printf("Thread %d died\n", worker_id);
	return NULL;
}

int main(int argc,char* argv[]){
	if (signal(SIGINT, sig_handler) == SIG_ERR)
		printf("can't catch SIGINT\n");

	if(argc >= 2){		// SERVICE URI
		service_uri = malloc(strlen(argv[1])+1);
		strcpy(service_uri, argv[1]);
	} else {
		service_uri = malloc(strlen(DEFAULT_SERVICE_URI)+1);
		strcpy(service_uri, DEFAULT_SERVICE_URI);
	}

	if(argc >= 3){		// Number of Workers
		int n = atoi(argv[2]);
		if(n > 0){
			Max_Working = n;
		}
	}

	if(argc >= 4){		// Execution time
		int n = atoi(argv[3]);
		if(n >= 0){
			job_exec_time = n;
		}
	}

	if(argc >= 5){		// Period
		int n = atoi(argv[4]);
		if(n > 0){
			Service_period = n;
		}
	}

	if(argc >= 6){		// Request per period
		int n = atoi(argv[5]);
		if(n > 0){
			Service_RPP = n;
		}
	}

	uri_len = strlen(service_uri);

	printf("Running Service Server %s with %d workers, job exec time:%d\n", service_uri, Max_Working, job_exec_time);

	linber_init();
	linber_register_service(service_uri, uri_len, job_exec_time, Max_Working, &service_token, Service_period, Service_RPP);
	printf("registered service, starting thread pool\n");
	int workers_num = Max_Working;
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
	free(service_uri);
}


