#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "../libs/linber_service_api.h"

#define SERVICE_NAME	"org.server.func.1\0"
#define REL_DEADLINE		10

int main(){
	printf("Running client test\n");
	linber_init();
	for(int i=0; i<2; i++){
		linber_request_service(SERVICE_NAME, sizeof(SERVICE_NAME), REL_DEADLINE);
	}
	linber_exit();
}