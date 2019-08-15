#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../libs/linber_service_api.h"

#define PERIOD	1

int main(int argc,char* argv[]){
	system_status system;
	linber_init();
	while(1){
		sleep(PERIOD);
		linber_system_status(&system);
		int num_services = system.services_count;
		int num_concurent_workers = system.system.max_concurrent_workers;
		int num_requests = system.system.requests_count;
		int load = system.system.serving_load;
		int serving_count = system.system.serving_requests_count;
		printf("---------------------\n");
		printf("Services: %d, Concurrent Workers: %d, Requests: %d, Load: %d, Serving: %d\n", num_services, num_concurent_workers, num_requests, load, serving_count);
	}
	linber_exit();
}

