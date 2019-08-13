#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "../libs/linber_service_api.h"

#define SERVICE_NAME	"org.server.func.1\0"
#define SERVICE_EXECTIME	5

int main(){
	printf("Running Server test\n");
	linber_init();
	linber_register_service(SERVICE_NAME, sizeof(SERVICE_NAME), SERVICE_EXECTIME);

	for(int i=0; i<8; i++){
		linber_start_job_service(SERVICE_NAME, sizeof(SERVICE_NAME));
		linber_end_job_service(SERVICE_NAME, sizeof(SERVICE_NAME));
	}

	linber_exit();
}