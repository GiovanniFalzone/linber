#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../../libs/linber_service_api.h"

#define DEFAULT_PERIOD	0.5*1000000	// 0.5 second in microseconds


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

	if(linber_init() < 0){
		printf("Linber Module not initialized\n");
		return 0;
	}
	while(1){
		usleep(period);
		linber_system_status(&system);
		int num_services = system.services_count;
		int num_concurent_workers = system.Max_Working;
		int load = 0;
		printf("---------------------\n");
		printf("Linber: Services: %d, Max Concurrent Workers: %d\n", \
						num_services, num_concurent_workers);
		for(int i = 0; i< num_services; i++){
			service_status service = system.services[i];
			load = 0;
			if(service.requests_count >0 && service.Max_Working >0){
				load = ((service.requests_count*100) / service.Max_Working);
				if(load > 100) load = 100;
			}
			printf("Service %s, ExecTime: %lu, Max Concurrent Workers %d Requests:%d, Load: %d\n", \
					service.uri, service.exec_time_ms, service.Max_Working, service.requests_count, load);
		}
	}
	linber_exit();
}

