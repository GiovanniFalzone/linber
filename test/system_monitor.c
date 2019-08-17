#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../libs/linber_service_api.h"

#define PERIOD	100

int main(int argc,char* argv[]){
	system_status system;
	system.services = malloc(MAX_SERVICES * sizeof(service_status));
	for(int i = 0; i< MAX_SERVICES; i++){
		system.services[i].uri = malloc(MAX_URI_LEN);
	}

	linber_init();
	while(1){
		usleep(PERIOD);
		linber_system_status(&system);
		int num_services = system.services_count;
		int num_concurent_workers = system.max_concurrent_workers;
		int num_requests = system.requests_count;
		int serving_count = system.serving_requests_count;
		printf("---------------------\n");
		printf("Services: %d, Max Concurrent Workers: %d, Requests: %d Serving: %d\n", num_services, num_concurent_workers, num_requests, serving_count);
		for(int i = 0; i< num_services; i++){
			service_status service = system.services[i];
			printf("Service %s, ExecTime: %d, Max Concurrent Workers %d Requests:%d, Serving: %d, Serving time:%d\n", service.uri, service.exec_time, service.max_concurrent_workers, service.requests_count, service.serving_requests_count, service.serving_time);
		}
	}
	linber_exit();
}

