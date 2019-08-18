#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../libs/linber_service_api.h"

#define DEFAULT_PERIOD	1000000	// 1 second in microseconds


int main(int argc,char* argv[]){
	unsigned int period = DEFAULT_PERIOD;
	system_status system;
	if(argc >= 2){
		int n = atoi(argv[1]);
		if(n > 0){
			period = n * 1000;	// input in milliseconds
		}
	}

	system.services = malloc(MAX_SERVICES * sizeof(service_status));
	for(int i = 0; i< MAX_SERVICES; i++){
		system.services[i].uri = malloc(MAX_URI_LEN);
	}

	linber_init();
	while(1){
		usleep(period);
		linber_system_status(&system);
		int num_services = system.services_count;
		int num_concurent_workers = system.max_concurrent_workers;
		int num_requests = system.requests_count;
		int serving_count = system.serving_requests_count;
		int load = 0;
		if(serving_count >0 && num_concurent_workers >0){
			load = ((serving_count*100) / num_concurent_workers);
		}
		printf("---------------------\n");
		printf("Linber: Services: %d, Max Concurrent Workers: %d, Requests: %d Serving: %d, Load: %d\n", \
						num_services, num_concurent_workers, num_requests, serving_count, load);
		for(int i = 0; i< num_services; i++){
			service_status service = system.services[i];
			load = 0;
			if(service.serving_requests_count >0 && service.max_concurrent_workers >0){
				load = ((serving_count*100) / num_concurent_workers);
			}
			printf("Service %s, ExecTime: %d, Max Concurrent Workers %d Requests:%d, Serving: %d, Serving time:%d, Load: %d\n", \
					service.uri, service.exec_time, service.max_concurrent_workers, service.requests_count, service.serving_requests_count, service.serving_time, load);
		}
	}
	linber_exit();
}

